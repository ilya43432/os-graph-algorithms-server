#include "../Part_1/Graph.hpp"
#include "../Part_7/Algorithms/AlgorithmFactory.hpp"
#include "random_graph.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <limits>
#include <netdb.h>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <cctype>
#include <pthread.h>
#include <stdexcept>
#include <vector>

#define BUFFER_SIZE 1024

using namespace std;

// global server socket fd
// used by the worker threads inside the worker loop
// for accept()
static int g_server_fd = -1;

/// leader follower synchronization
// mutex protecting the lf state
static pthread_mutex_t lf_mx = PTHREAD_MUTEX_INITIALIZER;

// conditional pthread  var for transfering leadership from one worker to another
static pthread_cond_t lf_cv = PTHREAD_COND_INITIALIZER;

// mutex for cout usage
// this is so multiple therads dont interleave output
// and cause race conditions as we got from helgrind checks
static pthread_mutex_t cout_mx = PTHREAD_MUTEX_INITIALIZER;

// boolean to indicate whether theres currently a leader thread
static bool has_leader = false;

// a pthread mutex for protection in handling all the clients that are connected to the server
static pthread_mutex_t clients_mx = PTHREAD_MUTEX_INITIALIZER;

// we use a vector to follow all the active clients in case the server shuts down
static std::vector<int> active_clients;

// a pthread mutex for protecting the shutdown mechanism for the server
static pthread_mutex_t shutdown_mx = PTHREAD_MUTEX_INITIALIZER;

// a bool flag for the server, incase of a shutdown request using QUIT
// this tells the worker threads that the server "requested" a shut down
static bool shutdown_requested = false;

// number of workers
#define num_workers 4

/// @brief  usage and introduction for the client.
std::string introduction_for_client()
{
    std::string intro =
        "Welcome to Our Graph Algorithms server - \n"
        "Commands Usage: \n"
        "Newgraph <num of vertices> <D|U>\n"
        "RandomGraph <edges> <vertices> <seed> <D|U>\n"
        "EDGES <num of edges> \n"
        "*Note* : After creating a graph there will be solutions\n"
        "to the 4 algorithms which are - \n"
        "EULER (Directed & Undirected) \n"
        "HamiltonianCircuit (Directed & Undirected) \n"
        "Minimum spanning tree weight (Undirected) \n"
        "Max clique found in the Graph (Undirected)\n"
        "Strongly connected components in the Graph (Directed).\n"
        "Depending on the graph direction, some algorithms may or may not work.\n"
        "therfore it is stated for each Algorithm, the direction it needs.\n"
        "\n";
    return intro;
}

/// @brief
/// @param port
void usage_for_server(int port)
{
    std::cout << "Graph Magician started on port " << port << std::endl;
    std::cout << "type QUIT to shut down the server" << "\n";
}

/// @brief
/// @param input_graph
/// @return
std::vector<std::string> graph_algo_calculations(Graph &input_graph, bool d_u)
{
    std::vector<std::string> ans;

    if (!d_u) // graph chosen is undirected
    {
        auto algo_E = AlgorithmFactory::create_algo("EULER");
        ans.push_back("Euler: " + algo_E->execute(input_graph));

        auto algo_HAM = AlgorithmFactory::create_algo("HAM");
        ans.push_back("Hamiltonian Circuit: " + algo_HAM->execute(input_graph));

        auto algo_MSTW = AlgorithmFactory::create_algo("MSTW");
        ans.push_back("MSTW: " + algo_MSTW->execute(input_graph));

        auto algo_Clique = AlgorithmFactory::create_algo("CLIQUE");
        ans.push_back("MAX CLIQUE: " + algo_Clique->execute(input_graph));

        ans.push_back("SCC: No SCC because the graph is Undirected!\n");
    }
    else // directed graph
    {
        auto algo_E = AlgorithmFactory::create_algo("EULER");
        ans.push_back("Euler: " + algo_E->execute(input_graph));

        auto algo_HAM = AlgorithmFactory::create_algo("HAM");
        ans.push_back("Hamiltonian Circuit: " + algo_HAM->execute(input_graph));

        ans.push_back("MSTW: No MSTW because the graph is directed!\n");

        ans.push_back("MAX CLIQUE: No Clique because the graph is directed!\n");

        auto algo_SCC = AlgorithmFactory::create_algo("SCC");
        ans.push_back("SCC:" + algo_SCC->execute(input_graph) + "\n");
    }

    return ans;
}

/// @brief function to calculate all the algorithms on the specificed g graph and send the answer to the client.
void calculate_and_send(int c_fd, Graph &g, bool is_directed)
{
    // call the graph_algo_calculations function and
    // return each algo result
    std::vector<std::string> result = graph_algo_calculations(g, is_directed);

    for (auto &s : result)
    {
        send(c_fd, s.c_str(), s.size(), MSG_NOSIGNAL);
    }
}

/// @brief handle all communication with a client
void process_client(int c_fd)
{
    std::string introduction = introduction_for_client();
    send(c_fd, introduction.c_str(), introduction.size(), MSG_NOSIGNAL);

    // our graph to be initialized
    Graph input_graph;

    // buffer
    char buffer[BUFFER_SIZE];

    // bool flags
    bool is_directed = false;
    bool graph_created = false;
    bool edges_inserted = false;

    while (1)
    {
        // clear buffer
        memset(buffer, 0, BUFFER_SIZE);

        // recieve input
        int bytes = recv(c_fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes <= 0)
        {
            if (bytes == 0)
            {
                // lock cout to avoid race conditions between different threads
                // we use the pthread cout mutex for this
                pthread_mutex_lock(&cout_mx);
                std::cout << "client has disconnected." << std::endl;
                pthread_mutex_unlock(&cout_mx);
            }
            else
            {
                perror("bytes recv");
            }
            break;
        }

        std::string line(buffer);
        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;

        while (iss >> token)
        {
            tokens.push_back(token);
        }

        // check if token vector is empty
        if (tokens.empty())
        {
            continue;
        }

        // first word of client's input
        std::string cmd = tokens[0];

        if (cmd == "Newgraph")
        {
            /// check vector tokens size
            // and also check its valditity later
            // Newgraph 3 valid
            // Newgraph a invalid
            // Newgraph -3 invalid
            // tokens size has to be 3
            if (tokens.size() != 3)
            {
                std::string correction_usage = "usage Newgraph <num> <D|U>\n";
                send(c_fd, correction_usage.c_str(), correction_usage.size(), MSG_NOSIGNAL);
                continue;
            }

            // num of vxs
            int num_vxs;
            // direction of the graph
            char direction;

            // string stream objects from the tokens
            // tokens[1] num of vxs
            // tokens[2] direction
            std::istringstream a(tokens[1]);
            std::istringstream b(tokens[2]);

            // check valditity of num of vxs and the direction
            if (!(a >> num_vxs) || !(b >> direction) || (direction != 'D' && direction != 'U'))
            {
                std::string usage = "usage is Newgraph <num vxs> <D|U>\n";
                send(c_fd, usage.c_str(), usage.size(), MSG_NOSIGNAL);
                continue;
            }

            if (num_vxs <= 0)
            {
                std::string usage = "number of vertices must be greater than 0!\n";
                send(c_fd, usage.c_str(), usage.size(), MSG_NOSIGNAL);
                continue;
            }

            // is_directed flag is set to true whenever the direction input is of course, directed.
            is_directed = (direction == 'D');

            // initlize graph
            input_graph = Graph(num_vxs, is_directed);

            // set flags to true
            graph_created = true;
            edges_inserted = false;

            std::string msg = std::string("created a ") + (is_directed ? "Directed " : "Undirected ") + "graph with " + std::to_string(num_vxs) + " vertices\n";
            send(c_fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
        }
        // clients input "RandomGraph"
        // this will generate a random graph using the usage
        // RandomGraph <edges> <vertices> <seed> <D|U>
        // he will give the number of edges, vertrices, seed and a direction
        // we use the function "generate_graph" in the random_graph.cpp file.
        else if (cmd == "RandomGraph")
        {

            // vector tokens must be of size 5 !
            if (tokens.size() != 5)
            {
                std::string correction = "usage: RandomGraph <edges> <vertices> <seed> <D|U>\n";
                send(c_fd, correction.c_str(), correction.size(), MSG_NOSIGNAL);
                continue;
            }

            // check if the direction given is valid
            if (tokens[4].size() != 1 || (tokens[4][0] != 'D' && tokens[4][0] != 'U'))
            {
                std::string correction = "direction must be D or U\n";
                send(c_fd, correction.c_str(), correction.size(), MSG_NOSIGNAL);
                continue;
            }

            /*
                CHECKS FOR VALDITITY OF THE GRAPH INPUTS ARE LOCATED IN THE
                generate_graph function located

            */

            try
            {
                // strict to the format
                // num vertices
                int num_vxs = std::stoi(tokens[2]);
                // num edges
                int num_edges = std::stoi(tokens[1]);
                // seed
                int input_seed = std::stoi(tokens[3]);

                // direction
                bool random_directed = (tokens[4][0] == 'D');

                // use the generate_graph function located at random_graph.cpp file.
                input_graph = generate_graph(input_seed, num_edges, num_vxs, random_directed);

                // set the bool is_directed flag accordingly
                is_directed = random_directed;

                // set graph created flag to to true
                // since a random graph was created
                graph_created = true;
                edges_inserted = false;

                std::string ans =
                    std::string("A random ") + (is_directed ? "Directed " : "Undirected ") + " graph with " +
                    std::to_string(num_edges) + " edges and " + std::to_string(num_vxs) +
                    " vertices has been successfully created\n";

                // send the client information about the random created graph
                send(c_fd, ans.c_str(), ans.size(), MSG_NOSIGNAL);

                // calculate the 5 algorithms (since we added 4 of the required and also addedd EULER)
                calculate_and_send(c_fd, input_graph, is_directed);
            }
            catch (const std::exception &e)
            { // catch any exception within the random graph creation process
                std::string error = std::string("error creating random graph: ") + e.what() + "\n";
                send(c_fd, error.c_str(), error.size(), MSG_NOSIGNAL);
            }
        }

        // EDGES process
        else if (cmd == "EDGES")
        {
            // check if the graph is created
            if (!graph_created)
            {
                std::string err = "You have to create a graph first!\n";
                send(c_fd, err.c_str(), err.size(), MSG_NOSIGNAL);
                continue;
            }

            // check the vector tokens size
            // for example - EDGES 2 valid
            // EDGES   invalid
            // EDGES a invalid
            // we have to make sure the client specifies a valid number of edges
            // within the bounds of his num vxs specified - for both directed|undirected graphs
            if (tokens.size() != 2)
            {
                std::string err = "usage EDGES <num of edges>\n";
                send(c_fd, err.c_str(), err.size(), MSG_NOSIGNAL);
                continue;
            }

            // check if edges have been already inserted.
            // adding edges after the first graph creation and edge insertion
            // isnt allowed in our project
            if (edges_inserted)
            {
                std::string error = "Edges have been already inserted.\n";
                send(c_fd, error.c_str(), error.size(), MSG_NOSIGNAL);
                continue;
            }

            // num of edge pairs
            int numPairs_;
            try
            {
                // get the number as a string
                std::string input = tokens[1];

                if (input.empty())
                {
                    throw std::invalid_argument("input is empty!");
                }

                try
                {

                    numPairs_ = std::stoi(input);
                }
                catch (const std::exception &ec)
                {
                    throw std::invalid_argument("number must be numeric!");
                }

                // check if negative
                if (numPairs_ < 0)
                {
                    throw std::invalid_argument("value can't be negative!");
                }

                int v = input_graph.get_num_vertices();

                int max_edges;

                if (is_directed)
                {
                    max_edges = v * (v - 1);
                }
                else
                {
                    max_edges = (v * (v - 1)) / 2;
                }

                if (numPairs_ > max_edges)
                {
                    throw std::invalid_argument("number of edges cant be higher than " + std::to_string(max_edges));
                }
            }
            catch (const std::exception &ex)
            { // catch an exception within the EDGES process
                std::string error_usage = std::string("invalid input: ") + ex.what() + "\n";
                send(c_fd, error_usage.c_str(), error_usage.size(), MSG_NOSIGNAL);
                continue;
            }

            // for loop for the amount of edge pairs specified
            for (int i = 0; i < numPairs_; i++)
            {
                // clear buffer
                memset(buffer, 0, BUFFER_SIZE);

                // recieve input
                int b = recv(c_fd, buffer, BUFFER_SIZE - 1, 0);

                // check receiving errors
                if (b <= 0)
                {
                    break;
                }

                //  convert buffer into a string
                std::string edge_weight = buffer;

                // replace every "," with a blank
                std::replace(edge_weight.begin(), edge_weight.end(), ',', ' ');

                // input stringstream so we can parse the edge weight
                // as a string
                std::istringstream eiss(edge_weight);

                // source vx
                int source;
                // target vx
                int target;
                // weight of the edge
                int weight;

                // for example 1,2,3
                // source - 1
                // target - 2
                // weight 3

                // check if the format is valid
                if (!(eiss >> source >> target >> weight))
                {
                    std::string err = "invalid edge format (use source,target,weight)\n";
                    send(c_fd, err.c_str(), err.size(), MSG_NOSIGNAL);
                    i--;
                    continue;
                }

                try
                {
                    // weve deciced to define the weights within [1,25] for simplicity
                    if (weight < 1 || weight > 25)
                    {
                        std::string error = "edge weight must be in range [1,25]\n";
                        send(c_fd, error.c_str(), error.size(), MSG_NOSIGNAL);
                        i--;
                        continue;
                    }

                    // check if the source and target are within the bounds
                    if (source < 0 || target < 0 || source >= input_graph.get_num_vertices() || target >= input_graph.get_num_vertices())
                    {
                        std::string err = "vertex out of bounds\n";
                        send(c_fd, err.c_str(), err.size(), MSG_NOSIGNAL);
                        i--;
                        continue;
                    }

                    // we dont allow self loops for simplicity
                    if (source == target)
                    {
                        std::string err = "self loops not allowed for our graphs\n";
                        send(c_fd, err.c_str(), err.size(), MSG_NOSIGNAL);
                        i--;
                        continue;
                    }

                    // check if source and target are already connected
                    if (input_graph.is_connected(source, target))
                    {
                        std::string err = "duplicate edge ignored\n";
                        send(c_fd, err.c_str(), err.size(), MSG_NOSIGNAL);
                        i--;
                        continue;
                    }

                    // add edge function implemented in Graph.cpp in part 1
                    // this function accounts for both directed and undirected graphs
                    input_graph.add_edge(source, target, weight);
                }
                catch (std::exception &e)
                { // catch any exception
                    std::string err = std::string("ERROR: ") + e.what() + "\n";
                    send(c_fd, err.c_str(), err.size(), MSG_NOSIGNAL);
                }
            }

            // after a successful edge insertion
            // we give the client a confirmation message
            // and set the bool edges_inserted[i] to true
            // to avoid further additions to the graph.
            std::string ok = "edges inserted\n";
            send(c_fd, ok.c_str(), ok.size(), MSG_NOSIGNAL);
            edges_inserted = true;
            calculate_and_send(c_fd, input_graph, is_directed);
        }
        else
        {
            std::string error_msg = ("Unknown command\n");
            send(c_fd, error_msg.c_str(), error_msg.size(), MSG_NOSIGNAL);
            continue;
        }
    }
}

/// @brief leader follower loop
static void *worker_loop(void *)
{
    bool wait_shutdown = false;

    while (1)
    {
        int c_fd = -1;
        int local_server_fd = -1;

        // leader elect
        // lock mutex thread
        pthread_mutex_lock(&lf_mx);
        // if another worker is already the leader
        while (has_leader)
        {
            // wait for turn
            pthread_cond_wait(&lf_cv, &lf_mx);
        }

        // check if shutdown requested by the server
        pthread_mutex_lock(&shutdown_mx);
        wait_shutdown = shutdown_requested;
        if (!wait_shutdown)
        {
            local_server_fd = g_server_fd;
        }
        pthread_mutex_unlock(&shutdown_mx);

        //
        if (wait_shutdown)
        {
            pthread_mutex_unlock(&lf_mx);
            break; // if so, then break out of the loop
        }

        // worker becomes the leader
        has_leader = true;

        // unlock mutex thread
        pthread_mutex_unlock(&lf_mx);

        // leader accepts new client
        c_fd = accept(local_server_fd, nullptr, nullptr);

        // leader resigns and promotes the next follower
        // lock mutex
        pthread_mutex_lock(&lf_mx);

        // flag goes false
        has_leader = false;

        // signal the waitingf follower to become the leader
        pthread_cond_signal(&lf_cv);

        // unlock mutex thread
        pthread_mutex_unlock(&lf_mx);

        // if accept fails
        if (c_fd < 0)
        {
            pthread_mutex_lock(&shutdown_mx);
            wait_shutdown = shutdown_requested;
            pthread_mutex_unlock(&shutdown_mx);

            if (wait_shutdown)
            {
                break;
            }

            perror("accept");
            continue;
        }

        // print the new client
        pthread_mutex_lock(&cout_mx);
        std::cout << "New client has been connected" << std::endl;
        pthread_mutex_unlock(&cout_mx);

        // add the client to the active client vector
        pthread_mutex_lock(&clients_mx);
        active_clients.push_back(c_fd);
        pthread_mutex_unlock(&clients_mx);

        // hande the clients request
        process_client(c_fd);
        // close after client disconnects
        close(c_fd);

        // remove client from active list
        // after he disconnects
        pthread_mutex_lock(&clients_mx);

        active_clients.erase(std::remove(active_clients.begin(), active_clients.end(), c_fd), active_clients.end());

        pthread_mutex_unlock(&clients_mx);
    }
    return nullptr;
}

/// @brief this function will help monitor the server stdin
/// this is specifically for the keyword QUIT as to shut down
/// the server in a normal way
void *wait_for_quit(void *)
{
    std::string ans;

    // read from the stdin
    while (std::getline(std::cin, ans))
    {
        // if the input is QUIT then it is considered a shutdown command
        if (ans == "QUIT")
        {
            // lock the cout using the pthread mutex cout protection
            // to prevent race conditions
            pthread_mutex_lock(&cout_mx);
            std::cout << "shutting down the server." << std::endl;
            // unlock it
            pthread_mutex_unlock(&cout_mx);

            // we set the global shutdown flag
            // so that there will be coordination
            // between the workers and the server's QUIT
            pthread_mutex_lock(&shutdown_mx);
            shutdown_requested = true;
            

            // we close the server socket
            // so accept() will be disabled
            if (g_server_fd != -1)
            {
                // dont allow further read input or write
                shutdown(g_server_fd, SHUT_RDWR);
                // close server fd
                close(g_server_fd);
                // set it as closed
                g_server_fd = -1;
            }
            pthread_mutex_unlock(&shutdown_mx);

            // we need to wake up the threads that are waiting
            // in our lf condition var
            // lock using leader follower pthread mutex
            pthread_mutex_lock(&lf_mx);
            // wake up all the waiting workers
            pthread_cond_broadcast(&lf_cv);
            // unlock
            pthread_mutex_unlock(&lf_mx);

            // we then close all the active client connections
            // lock clients pthread mutex
            pthread_mutex_lock(&clients_mx);
            for (int fd : active_clients)
            {
                // close the client's socket
                shutdown(fd, SHUT_RDWR);
                close(fd);
            }
            // unlock clients pthread mutex
            pthread_mutex_unlock(&clients_mx);

            break;
        }
        else
        {
            pthread_mutex_lock(&cout_mx);
            std::cout << "Unknown command" << std::endl;
            pthread_mutex_unlock(&cout_mx);
        }
    }

    return nullptr;
}

int main(int argc, char *argv[])
{

    // usage. for example -
    // ./graph_server 127.0.0.1 1234
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <host> <port>\n";
        return 1;
    }

    // create a socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // if socket creation fails
    if (server_fd == -1)
    {
        perror("socket failed");
        exit(1);
    }

    // allow reusing the port in case of restarts
    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // extact host
    const char *host_name = argv[1];
    // extract port
    int port = std::stoi(argv[2]);

    // check if the port is valid
    assert(port > 0 && port < 65535);

    // hints for getaddrinfo
    // in order to resolve th host and the port
    struct addrinfo hints{};
    struct addrinfo *res;
    struct addrinfo *rp;

    // clear hints buffer
    memset(&hints, 0, sizeof(hints));
    // ipv4
    hints.ai_family = AF_INET;
    // tcp socket
    hints.ai_socktype = SOCK_STREAM;
    // for bind()
    hints.ai_flags = AI_PASSIVE;

    // resolve the hostname and port
    // into a list of possible addresses
    int err = getaddrinfo(host_name, argv[2], &hints, &res);
    if (err != 0)
    {
        std::cerr << "getaddrinfo: " << gai_strerror(err) << "\n";
        close(server_fd);
        return 1;
    }

    // for loop to try and bind the socket into one of the resolved
    // addresses
    for (rp = res; rp != nullptr; rp = rp->ai_next)
    {
        if (bind(server_fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
    }

    // if no valid address worked, then the binding process failed
    if (!rp)
    {
        perror("bind failed");
        freeaddrinfo(res);
        exit(1);
    }

    // free addrinfo list
    freeaddrinfo(res);

    // listen to incoming connections
    int opt = listen(server_fd, 10);
    if (opt < 0)
    {
        perror("listen");
        close(server_fd);
        exit(1);
    }

    // save the server socket as global so that
    // the workers can access it
    g_server_fd = server_fd;

    // call usage for server
    usage_for_server(port);

    // pthread for stdin
    pthread_t stdin_server;

    // create the stdin pthread to watch for shutdown ("QUIT")
    pthread_create(&stdin_server, nullptr, wait_for_quit, nullptr);

    // vector for the workers in the lf
    std::vector<pthread_t> tids(num_workers);

    for (int i = 0; i < num_workers; ++i)
    {
        pthread_create(&tids[i], nullptr, worker_loop, nullptr);
    }

    // wait till stdin thread finishes
    // which means when QUIT input is given
    pthread_join(stdin_server, nullptr);

    // wake all the workers that are waiting
    // on the leader follower condition
    pthread_mutex_lock(&lf_mx);
    pthread_cond_broadcast(&lf_cv);
    pthread_mutex_unlock(&lf_mx);

    /// wait for all the workers to finish
    for (pthread_t tid : tids)
    {
        pthread_join(tid, nullptr);
    }

    // close global sock
    // and reset
    if (g_server_fd != -1)
    {
        close(g_server_fd);
        g_server_fd = -1;
    }

    return 0;
}
