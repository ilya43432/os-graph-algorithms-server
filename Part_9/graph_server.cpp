#include "../Part_1/Graph.hpp"
#include "../Part_7/Algorithms/AlgorithmFactory.hpp"
#include "../Part_8/random_graph.hpp"
#include "Pipeline_Pattern.hpp"
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
#include <stdexcept>

#define BUFFER_SIZE 1024

using namespace std;

// mutex for cout usage
// this is so multiple therads dont interleave output
// and cause race conditions as we got from helgrind checks
static pthread_mutex_t cout_mx = PTHREAD_MUTEX_INITIALIZER;

// mutex for shutdown flag
// protects the shutdown_requested bool so threads don’t read or write it at the same time
static pthread_mutex_t shutdown_mx = PTHREAD_MUTEX_INITIALIZER;

// a bool flag for the server, incase of a shutdown request using QUIT
// this tells the worker threads that the server "requested" a shut down
static bool shutdown_requested = false;

// global server socket fd
// used by the worker threads inside the worker loop
// for accept()
static int g_server_fd = -1;

// a pthread mutex for protection in handling all the clients that are connected to the server
static pthread_mutex_t clients_mx = PTHREAD_MUTEX_INITIALIZER;

// we use a vector to follow all the active clients in case the server shuts down
static std::vector<int> active_clients;

// number of stages for the pipeline
#define NUM_STAGES 5

/// @brief  usage and introduction for the client.
std::string introduction_for_client()
{
    std::string intro =
        "Welcome to Our Graph Algorithms server - \n"
        "Commands Usage: \n"
        "Newgraph <num of vertices> <D|U> (commas also accepted)\n"
        "RandomGraph <edges> <vertices> <seed> <D|U>\n"
        "EDGES <num of edges> \n"
        "*Note* : After creating a graph there will be solutions\n"
        "to the 4 algorithms chosen which are - \n"
        "EULER (Directed & Undirected) \n"
        "HamiltonianCircuit (Directed & Undirected) \n"
        "Minimum spanning tree weight (Undirected) \n"
        "Max clique found in the Graph (Undirected)\n"
        "Strongly connected components in the Graph (Directed).\n"
        "Depending on the graph direction, some algorithms may or may not work.\n"
        "therfore it is stated for each Algorithm, the direction it needs.\n";
    return intro;
}

/*
    Pipeline Pattern -
    each stage is one graph algorithm

    Our chosen Algorithms from the list given in the assignment -
    Euler cycle (We know it was not given but we have already implemented an algo for it so why not..)
    Hamiltonian Cycle
    MST weight (only for Undirected graphs)
    Strongly Connected Components (only for Directed graphs)
    Maximum Clique (only for Undirected graphs)

    the pipeline flow is -

    Stage 1 - Euler cycle
    Stage 2 - Hamiltonian Cycle
    Stage 3 - MST Weight
    Stage 4 - Strongly Connected Components
    Stage 5 - Maximum Clique

    After completing a stage, the pipeline pushes the graph into the next stage
    using p->push_to(the next stage id , we can use to transfer to the next stage using std::bind(the next stage), client fd, client's graph)

    Each stage checks graph type (directed / undirected) and skips unsupported algos,
    sending a message back to the client.
    if exceptions are thrown they are also sent to the client.

    the pipeline grants the once a client triggers the graphs creation,
    they will recieve the results of all supported algorithms in a sequence.

    Exmaple for Unirected graphs -
    EULER -> HAM -> MSTW -> **SKIPS SCC* -> CLIQUE

    Example for Directed garphs -
    EULER -> HAM -> **SKIPS MSTW** -> SCC -> **SKIPS Clique**

    Stage 1 - run_euler

    Runs the euler cycle algortihm (if exists), after sending the result
    it calls session->pipe->push_to(1, std::bind(run_ham, session, g)) to move to stage 2

    Stage 2 - run_ham

    runs the hamiltonian cycle algo. after sending the result it calls
    session->pipe->push_to(2, std::bind(run_mstw, session, g)) to move to stage 3

    Stage 3 - run_mstw
    runs the minimum spanning tree algo (if undirected).
    if the graph is directed it sends a message that the current algo is not available and skips,
    and pushes forward to stage 4 which is the strongly connected components algorithm.

    Stage 4 - run_SCC
    runs the strongly connected components algo (if the graph is directed)
    if graph is undirected it sends a message not available to the client and skips
    and pushes forward to the max clique algo, to the final stage.

    Stage 5 - run_clique (FINAL STAGE)
    runs the maximum clique (if graph is undirected)
    if graph is directed, sends not available to the client.

    THIS IS THE FINAL STAGE. Which means that the pipeline ends here.

*/

/// @brief hold info for each client
/// fd - client socket
/// pipe - pointer to the client's pipeline
/// send_mx a mutex so that threads dont intervene when sending info to the client
struct client_session
{
    int fd;
    Pipeline *pipe;
    pthread_mutex_t send_mx;

    /// @brief constrcutor for client session
    /// @param socket_fd client sock
    /// @param pipeline client pipe
    client_session(int socket_fd, Pipeline *pipeline) : fd(socket_fd), pipe(pipeline)
    {
        // initialize send_mx (pthread mutex)
        pthread_mutex_init(&send_mx, nullptr);
    }

    /// @brief destroy the mutex when the client session is done
    ~client_session()
    {
        pthread_mutex_destroy(&send_mx);
    }
};

/// @brief // safe send function
// locks the client session mutex before sending
/// @param session the clients session
/// @param msg  messagge to the client
/// this makes sure multiple threads don’t mess up output to the same socket
static void safe_send(client_session *session, const std::string &msg)
{
    pthread_mutex_lock(&session->send_mx);
    send(session->fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
    pthread_mutex_unlock(&session->send_mx);
}

/// @brief usage for server - QUIT option..
/// @param port
void usage_for_server(int port)
{
    std::cout << "Graph Magician started on port " << port << std::endl;
    std::cout << "type QUIT to shut down the server" << "\n";
}

/// @brief Last stage.
/// STAGE 5 - MAX CLIQUE ALGO
/// @param c_fd
/// @param g
void run_clique(client_session *session, Graph g)
{

    try
    {
        if (g.directed())
        {
            std::string msg = "CLIQUE: Not available for directed graphs\n\n";
            safe_send(session, msg);
            return;
        }

        auto algo = AlgorithmFactory::create_algo("CLIQUE");
        std::string result = "CLIQUE: " + algo->execute(g) + "\n\n";
        safe_send(session, result);
    }
    catch (const std::exception &e)
    {
        std::string error = std::string("error with CLIQUE algo: ") + e.what() + "\n\n";
        safe_send(session, error);
    }
}

/// @brief STAGE 4 - STRONGLY CONNECTED COMPONENTS ALGO
/// @param c_fd
/// @param p
/// @param g
void run_SCC(client_session *session, Graph g)
{
    try
    {

        if (!g.directed())
        {
            std::string msg = "SCC: Not computed for undirected graphs\n\n";
            safe_send(session, msg);
            session->pipe->push_to(4, std::bind(run_clique, session, g));
            return;
        }

        auto algo = AlgorithmFactory::create_algo("SCC");
        std::string res = "SCC: " + algo->execute(g) + "\n\n";

        safe_send(session, res);
    }
    catch (const std::exception &e)
    {
        std::string err = std::string("error with SCC algo: ") + e.what() + "\n\n";
        safe_send(session, err);
    }

    // move to stage 4 (clique)
    session->pipe->push_to(4, std::bind(run_clique, session, g));
}

/// @brief STAGE 3 - MINIMUM SPANNING TREE ALGO
/// @param c_fd
/// @param p
/// @param g
void run_mstw(client_session *session, Graph g)
{
    try
    {
        if (g.directed())
        {
            std::string msg = "MSTW: No MSTW because the graph is directed!\n\n";
            safe_send(session, msg);
            // proceed to last stage
            session->pipe->push_to(3, std::bind(run_SCC, session, g));
            return;
        }

        auto algo = AlgorithmFactory::create_algo("MSTW");
        std::string result = "MSTW: " + algo->execute(g) + "\n\n";
        safe_send(session, result);
    }
    catch (const std::exception &e)
    {
        std::string error = std::string("error with MSTW algo: ") + e.what() + "\n\n";
        safe_send(session, error);
    }

    // move to stage 3 (clique)
    session->pipe->push_to(3, std::bind(run_SCC, session, g));
}

/// @brief STAGE 2 - HAMILTONIAN CYCLE ALGORITHM
void run_ham(client_session *session, Graph g)
{
    try
    {
        auto algo = AlgorithmFactory::create_algo("HAM");
        std::string result = "Hamilton: " + algo->execute(g) + "\n\n";
        safe_send(session, result);
    }
    catch (const std::exception &e)
    {
        std::string error = std::string("error with HAM algo: ") + e.what() + "\n\n";
        safe_send(session, error);
    }

    session->pipe->push_to(2, std::bind(run_mstw, session, g));
}

/// @brief STAGE 1 - EULER ALGO
void run_euler(client_session *session, Graph g)
{

    try
    {
        auto algo = AlgorithmFactory::create_algo("EULER");
        std::string result = "Euler: " + algo->execute(g) + "\n\n";
        safe_send(session, result);
    }
    catch (const std::exception &e)
    {
        std::string error = std::string("error with EULER algo: ") + e.what() + "\n\n";
        safe_send(session, error);
    }

    // move to stage 1 (ham)
    session->pipe->push_to(1, std::bind(run_ham, session, g));
}

// EULER->HAM->MSTW->SCC->CLIQUE

/// function to start the pipeline for the client.
/// we chose to start from euler algo for stage 0
/// after that , pipeline will handle moving forward to the next stages
void graph_algo_stages(client_session *session, Graph g)
{
    // start in stage 0 (euler)
    session->pipe->push_to(0, std::bind(run_euler, session, g));
}

/// @brief handle all communication with a client
/// @param arg pointer to socket fd
void *process_client(void *arg)
{

    // extract the client socket fd
    int c_fd = *reinterpret_cast<int *>(arg);

    // free memory to avoid leaks
    delete reinterpret_cast<int *>(arg);

    pthread_mutex_lock(&clients_mx);
    active_clients.push_back(c_fd);
    pthread_mutex_unlock(&clients_mx);

    std::string intro = introduction_for_client();

    // uninitialized graph for the client
    Graph input_graph;

    // buffer for input
    char buffer[BUFFER_SIZE];

    // bool flags
    bool is_directed = false;

    bool graph_created = false;

    bool edges_inserted = false;

    // pipeline object in order for us to handle the algorithm stages
    Pipeline client_p;

    client_session session(c_fd, &client_p);

    // vector of active object which are the stages in the pipeline
    std::vector<ActiveGraph> g_stages(NUM_STAGES);

    // well add each stage into the pipe
    for (ActiveGraph &g : g_stages)
    {
        client_p.add_stage(&g);
    }

    // start function
    // the worker threads begin to run
    client_p.start();

    safe_send(&session, intro);

    while (1)
    {
        // clear buffer
        memset(buffer, 0, BUFFER_SIZE);

        // recieve bytes
        int bytes = recv(c_fd, buffer, BUFFER_SIZE - 1, 0);

        // check if theres an error with recieving the bytes
        if (bytes <= 0)
        {
            // if client disconnected
            if (bytes == 0)
            {
                std::ostringstream info;
                info << "Client has disconnected\n";

                pthread_mutex_lock(&cout_mx);

                std::cout << info.str();

                pthread_mutex_unlock(&cout_mx);
            }
            else
            {
                perror("bytes recv");
            }
            break;
        }

        // convert buffer to a string
        std::string line(buffer);

        // intput stringstream so we can parse
        // the input like a string
        std::istringstream iss(line);

        // token vector to hold each word
        // from the clients input
        std::vector<std::string> tokens;
        std::string token;

        // extarct tokens
        while (iss >> token)
        {
            tokens.push_back(token);
        }

        // check if token vector is empty
        if (tokens.empty())
        {
            continue;
        }

        // extract the first word from the clients input
        std::string cmd = tokens[0];

        if (cmd == "Newgraph")
        {
            // Newgraph <num vxs> <D|U> OR Newgraph <num vxs>,<D|U>
            if (tokens.size() != 3)
            {
                std::string correction_usage = "usage Newgraph <num> <D|U>\n";
                safe_send(&session, correction_usage);
                continue;
            }

            // Newgraph <num vxs> <direction U|D>
            int num_vxs;
            char direction;

            // get the num of vxs
            std::istringstream a(tokens[1]);

            // direction
            std::istringstream b(tokens[2]);

            if (!(a >> num_vxs) || !(b >> direction) || (direction != 'D' && direction != 'U'))
            {
                std::string usage = "usage is Newgraph <num vxs> <D|U>\n";
                safe_send(&session, usage);
                continue;
            }

            if (num_vxs <= 0)
            {
                std::string usage = "number of vertices must be greater than 0!\n";
                safe_send(&session, usage);
                continue;
            }

            // if direction is D then we set the flag accordingly
            is_directed = (direction == 'D');

            // create new graph
            input_graph = Graph(num_vxs, is_directed);

            graph_created = true;

            edges_inserted = false;

            // send clarification msg to client if succesful
            std::string msg = std::string("created a ") + (is_directed ? "Directed " : "Undirected ") + "graph with " + std::to_string(num_vxs) + " vertices\n";
            safe_send(&session, msg);
        }

        // clients input "RandomGraph"
        // this will generate a random graph using the usage
        // RandomGraph <edges> <vertices> <seed> <D|U>
        // he will give the number of edges, vertrices, seed and a direction
        // we use the function "generate_graph" in the random_graph.cpp file.
        else if (cmd == "RandomGraph")
        {
            // vector tokens must be of size 5
            if (tokens.size() != 5)
            {
                std::string correction = "usage: RandomGraph <edges> <vertices> <seed> <D|U>\n";
                safe_send(&session, correction);
                continue;
            }

            // check if the direction given is valid
            if (tokens[4].size() != 1 || (tokens[4][0] != 'D' && tokens[4][0] != 'U'))
            {
                std::string correction = "direction must be D or U\n";
                safe_send(&session, correction);
                continue;
            }

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
                edges_inserted = true;

                std::string ans =
                    std::string("A random ") + (is_directed ? "Directed " : "Undirected ") + " graph with " +
                    std::to_string(num_edges) + " edges and " + std::to_string(num_vxs) +
                    " vertices has been successfully created\n";

                // send the client information about the random created graph
                safe_send(&session, ans);

                // invoke the pipeline pattern. we calculate each algo within its specified stage
                graph_algo_stages(&session, input_graph);
            }
            catch (const std::exception &e)
            { // catch any exception within the random graph creation process
                std::string error = std::string("error creating random graph: ") + e.what() + "\n";
                graph_created = false;
                edges_inserted = false;
                safe_send(&session, error);
                continue;
            }
        }
        // EDGES process
        else if (cmd == "EDGES")
        {
            // cant type EDGES without creating/generating a graph
            if (!graph_created)
            {
                std::string err = "You have to create a graph first!\n";
                safe_send(&session, err);
                continue;
            }

            // check if edges have been already inserted.
            // adding edges after the first graph creation and edge insertion
            // isnt allowed in our project
            if (edges_inserted)
            {
                std::string error = "Edges have been already inserted.\n";
                safe_send(&session, error);
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
                std::string err = "usage: EDGES <num of edges>\n";
                safe_send(&session, err);
                continue;
            }

            // client input for the num of edges
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
                    throw std::invalid_argument("number of edges cant be higher than " + std::to_string(max_edges) + "\n");
                }
            }
            catch (const std::exception &ex)
            { // catch an exception within the EDGES process
                std::string error_usage = std::string("invalid input: ") + ex.what() + "\n";

                safe_send(&session, error_usage);
                continue;
            }

            // for loop for the amount of edge pairs specified
            for (int i = 0; i < numPairs_; i++)
            {

                // clear buffer
                memset(buffer, 0, BUFFER_SIZE);

                // recieve input
                int b = recv(c_fd, buffer, BUFFER_SIZE - 1, 0);

                // check receving errors
                if (b <= 0)
                {
                    break;
                }
                //  convert buffer into a string
                std::string edge_ = buffer;

                // replace every "," with a blank
                std::replace(edge_.begin(), edge_.end(), ',', ' ');

                // input stringstream so we can parse the edge weight
                // as a string
                std::istringstream eiss(edge_);

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
                    std::string err = "invalid edge format (expected: u,v,w)\n";
                    safe_send(&session, err);
                    i--;
                    continue;
                }

                try
                {

                    // check bounds for the source & target
                    int g_size = input_graph.get_num_vertices();
                    if (source < 0 || target < 0 || source >= g_size || target >= g_size)
                    {
                        std::string error = "vertex is out of bounds\n";
                        safe_send(&session, error);
                        i--;
                        continue;
                    }

                    // weight bounds at [1,25] (we chose this for simplicity)
                    if (weight < 1 || weight > 25)
                    {
                        std::string err = "edge weight must be in range [1,25]\n";
                        safe_send(&session, err);
                        i--;
                        continue;
                    }

                    // self loops arent allowed in our graphs
                    if (source == target)
                    {
                        std::string err = "self-loops not allowed\n";
                        safe_send(&session, err);
                        i--;
                        continue;
                    }

                    // check for edge duplications
                    if (input_graph.is_connected(source, target))
                    {
                        std::string err = "duplicate edge ignored\n";
                        safe_send(&session, err);
                        i--;
                        continue;
                    }

                    // add the edge
                    input_graph.add_edge(source, target, weight);
                }
                catch (std::exception &e)
                { // error during EDGES process
                    std::string err = std::string("error adding edge: ") + e.what() + "\n";
                    safe_send(&session, err);
                }
            }

            // inform the client for success
            std::string ok = "edges inserted\n\n";
            safe_send(&session, ok);
            edges_inserted = true;
            // invoke the algorimths using stages adhering to the pipeline pattern
            graph_algo_stages(&session, input_graph);
        }
        else
        {
            // catch any exception within the EDGES process
            std::string error = "Unknown command. Please try again.\n";
            safe_send(&session, error);
        }
    }

    // stop the pipeline
    client_p.stop();
    // close after client disconnects
    close(c_fd);

    // remove client from active list
    // after he disconnects
    pthread_mutex_lock(&clients_mx);
    // we use the vector iterator
    active_clients.erase(std::remove(active_clients.begin(), active_clients.end(), c_fd), active_clients.end());
    pthread_mutex_unlock(&clients_mx);

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
            // unlock
            pthread_mutex_unlock(&cout_mx);

            //  we set the global shutdown flag
            // so that there will be coordination
            // between the workers and the server's QUIT
            pthread_mutex_lock(&shutdown_mx);
            shutdown_requested = true;
            pthread_mutex_unlock(&shutdown_mx);

            // // we close the server socket
            // so accept() will be disabled

            pthread_mutex_lock(&shutdown_mx);
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
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <host> <port>\n";
        return 1;
    }

    // create the servers socket
    // AFT_INET - ipv4
    // SOCK_STREAM - tcp socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket failed");
        exit(1);
    }

    // extract host
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

    // call usage server function
    usage_for_server(port);

    // global server file descritor.
    // this will help us so that the shutdown mechanism for the server will work
    // and will also help the working threads
    g_server_fd = server_fd;

    // we create a seperate thread to look for the stdin input
    // "QUIT"
    pthread_t stdin_server;
    pthread_create(&stdin_server, nullptr, wait_for_quit, nullptr);

    bool shutting_down = false;

    while (1)
    {
        // wait for clients
        int c_fd = accept(server_fd, nullptr, nullptr);

        // if accept fails
        if (c_fd < 0)
        {
            // check if shutdown requested by the server
            pthread_mutex_lock(&shutdown_mx);
            shutting_down = shutdown_requested;
            pthread_mutex_unlock(&shutdown_mx);

            if (shutting_down)
            {
                break; // if so, then break out of the loop
            }

            perror("accept");
            continue;
        }

        // each client gets his own thread
        pthread_t tid;

        // client socket descriptor allocation
        // so it can be passed to the pthread func
        int *arg = new int(c_fd);

        pthread_mutex_lock(&cout_mx);
        std::cout << "New client has been connected" << std::endl;
        pthread_mutex_unlock(&cout_mx);

        // pthread create a detached thread to run
        // the process client using c_fd
        pthread_create(&tid, nullptr, process_client, arg);

        // no need to join later
        pthread_detach(tid);
    }

    // close the server socket
    pthread_mutex_lock(&shutdown_mx);
    if (server_fd != -1)
    {
        close(server_fd);
        // global server socket
        // is closed now
        g_server_fd = -1;
    }
    pthread_mutex_unlock(&shutdown_mx);

    // if QUIT wasnt triggered by the server.
    // we still have to set the bool flag to true
    if (!shutting_down)
    {
        pthread_mutex_lock(&shutdown_mx);
        shutdown_requested = true;
        pthread_mutex_unlock(&shutdown_mx);
    }

    // wait for stdin server to finish
    // stdin "QUIT"
    pthread_join(stdin_server, nullptr);

    return 0;
}