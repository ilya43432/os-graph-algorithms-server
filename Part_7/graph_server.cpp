#include "Algorithms/AlgorithmFactory.hpp"
#include "../Part_1/Graph.hpp"
#include "../Part_2/Euler_circle_algo.hpp"
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
#include <memory>
#include <unordered_map>
#include <vector>
#include <unordered_set>

#define BUFFER_SIZE 1024

using namespace std;

// introduction for the client.
// gives usage and info for our algorithms
std::string introduction_for_client()
{
    std::string intro =
        "Welcome to Our Graph Algorithms server - \n"
        "Commands Usage: \n"
        "Newgraph <num of vertices> <D for Directed | U for Undirected>\n"
        "EDGES <num of edges>\n"
        "EULER (Undirected Graph | Directed Graph)\n"
        "HAM (Undirected Graph | Directed Graph)\n"
        "MSTW (Undirected Graph)\n"
        "SCC (Directed Graph)\n"
        "CLIQUE (Find the Max Clique. Undirected Graph ONLY)\n";

    return intro;
}

// usage pop up when we run the server
void usage_for_server(int port)
{
    std::cout << "Graph Magician started on port " << port << std::endl;
    std::cout << "type QUIT to shut down the server" << "\n";
}

void process_client(int server_fd)
{
    // introduction msg for client
    std::string introduction = introduction_for_client();

    // holds the file descriptors (server + clients + stdin)
    fd_set mas_set;

    // temp copy passed to select()
    fd_set read_set;

    // initialize sets to 0
    FD_ZERO(&mas_set);
    FD_ZERO(&read_set);

    // add servers listening socket to the master set
    FD_SET(server_fd, &mas_set);

    // add stdin (standart input) to the master set
    // this will alow the server to accept commands
    // in the server
    FD_SET(STDIN_FILENO, &mas_set);

    // track the highest file descriptor
    int fd_max = server_fd;

    // check if stdin fd is greater than server_fd
    // update accordingly to that select() will check enough fds
    if (STDIN_FILENO > fd_max)
    {
        fd_max = STDIN_FILENO;
    }

    // map for the clients created graphs
    std::unordered_map<int, std::unique_ptr<Graph>> graphs;

    // buffer for clients input
    char buffer[BUFFER_SIZE];

    // bool map for each client
    // checks if the graph has been created to calculate algorithms on it
    std::unordered_map<int, bool> graph_created;

    // bool map for each client
    // we use this to check if the clients had already inserted edges
    // when he created a new graph, so that if so he wont be able to
    // add more edges to his graph. meaning that he has to create a new graph
    // to do so.
    std::unordered_map<int, bool> edges_inserted;

    // bool map for each graph's direction
    std::unordered_map<int, bool> is_directed;

    // set for the algorithms we chose to include for this assignment
    // we included EULER just because we have already implemented an algorithm for
    // it so why not..
    const std::unordered_set<std::string> algorithms = {"EULER", "HAM", "MSTW", "SCC", "CLIQUE"};

    while (1)
    {
        // each loop will start with a new copy of master set
        // mas set hols all active sockets.
        // read set is the one we pass to select
        read_set = mas_set;

        // wait for activity from clients or stdin
        if (select(fd_max + 1, &read_set, NULL, NULL, NULL) == -1)
        {
            perror("SELECT");
            exit(1);
        }

        // loop through all possible file descriptors up to fd max
        for (int i = 0; i <= fd_max; ++i)
        {
            // check if fd is ready
            if (FD_ISSET(i, &read_set))
            {
                // new connection on the server listening socket
                if (i == server_fd)
                {
                    // clients addr
                    // and addr length
                    sockaddr client_add;
                    socklen_t addr_len = sizeof(client_add);

                    // accept new connection
                    int accept_fd = accept(server_fd, (struct sockaddr *)&client_add, &addr_len);

                    // error during connection
                    if (accept_fd == -1)
                    {
                        perror("ACCEPT");
                        continue;
                    }

                    // add clients socket to the master set
                    FD_SET(accept_fd, &mas_set);

                    // update fd max if it is needed to track the highest
                    // file descriptor
                    if (accept_fd > fd_max)
                    {
                        fd_max = accept_fd;
                    }

                    // message for the server
                    std::cout << "New client has been connected" << std::endl;

                    // send introduction to the connected client
                    send(accept_fd, introduction.c_str(), introduction.size(), MSG_NOSIGNAL);
                }
                // stdin input
                else if (i == STDIN_FILENO)
                {
                    std::string input;
                    // read line from stdin
                    if (std::getline(std::cin, input))
                    {
                        // check if the input is QUIT
                        if (input == "QUIT")
                        {
                            std::cout << "Exiting the server!" << "\n";
                            return;
                        }
                        else
                        {
                            std::cout << "Unknown command! Please try again!" << "\n";
                        }
                    }
                    else
                    {
                        // if stdin is closed. we shut down the server
                        std::cout << "stdin is closed, shutting down the server!" << "\n";
                        return;
                    }
                }
                else
                {
                    // clear buffer
                    memset(buffer, 0, BUFFER_SIZE);

                    // recieve bytes
                    int bytes = recv(i, buffer, BUFFER_SIZE - 1, 0);

                    // check if theres an error with recieving the bytes
                    if (bytes <= 0)
                    {
                        // if client disconnected
                        if (bytes == 0)
                        {
                            std::cout << "A client has disconnected" << std::endl;
                        }
                        else
                        {
                            perror("bytes recv");
                        }
                        // close file descriptor
                        close(i);

                        // remove it from the list
                        FD_CLR(i, &mas_set);

                        // erase the clients graph
                        graphs.erase(i);

                        // erase bool
                        graph_created.erase(i);

                        // erase direction
                        is_directed.erase(i);

                        continue;
                    }

                    // convert the buffer to a string
                    std::string line(buffer);

                    // intput stringstream so we can parse
                    // the input like a string
                    std::istringstream iss(line);

                    // token vector to hold each word
                    // from the clients input
                    std::vector<std::string> tokens;

                    std::string token;

                    // extract the tokents
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

                    // Newgraph section
                    if (cmd == "Newgraph")
                    {
                        // check vector tokens size
                        // and also check its valditity later
                        // Newgraph 3 valid
                        // Newgraph a invalid
                        // Newgraph -3 invalid
                        // tokens size has to be 3
                        if (tokens.size() != 3)
                        {
                            std::string correction_usage = "usage Newgraph <num> <D|U>\n";
                            send(i, correction_usage.c_str(), correction_usage.size(), MSG_NOSIGNAL);
                            continue;
                        }

                        // num of vxs
                        int num_vxs;
                        // direction of the graph
                        char direction;

                        // wrap it as a string and get input stream
                        std::istringstream a(tokens[1]);

                        // same for direction
                        std::istringstream b(tokens[2]);

                        // check validity of num of vxs and the direction
                        if (!(a >> num_vxs) || !(b >> direction) || (direction != 'D' && direction != 'U'))
                        {
                            std::string usage = "usage is Newgraph <num vxs> <D|U>\n";
                            send(i, usage.c_str(), usage.size(), MSG_NOSIGNAL);
                            continue;
                        }

                        // number of vertices can't be 0 or negative
                        if (num_vxs <= 0)
                        {
                            std::string error_vxs = "Number of vertices must be greater than 0\n";
                            send(i, error_vxs.c_str(), error_vxs.size(), MSG_NOSIGNAL);
                            continue;
                        }

                        // is_directed flag is set to true whenever the direction input is of course, directed.
                        is_directed[i] = (direction == 'D');

                        // we use unique pointers for better memory management and to avoid
                        // unwanted memory losses
                        graphs[i] = std::make_unique<Graph>(num_vxs, is_directed[i]);

                        // set flags of graph created and edges inserted to false
                        // so that the client will have a chance now to insert his desired edges
                        graph_created[i] = true;
                        edges_inserted[i] = false;

                        // give info about the created graph so far
                        std::string msg = std::string("created a ") + (is_directed[i] ? "Directed " : "Undirected ") + "graph with " + std::to_string(num_vxs) + " vertices\n";
                        send(i, msg.c_str(), msg.size(), MSG_NOSIGNAL);
                    }

                    // EDGES section. after creating a graph using Newgraph
                    // the client can now use EDGES to insert edges to the graph
                    // weve decided to implement this like that because this is modular
                    // and simple. at first we thought about doing Newgraph <num vxs> <num edges> <direction>
                    // and then right away the client will have to option to give edge inputs.
                    // but we found it very problematic and divided the process so itll be clear for the both of us.
                    else if (cmd == "EDGES")
                    {
                        // check if graph has been created
                        if (!graph_created[i])
                        {
                            std::string err = "You have to create a graph first!\n";
                            send(i, err.c_str(), err.size(), MSG_NOSIGNAL);
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
                            std::string err = "ERROR: usage EDGES <num>\n";
                            send(i, err.c_str(), err.size(), MSG_NOSIGNAL);
                            continue;
                        }

                        // check if edges have been already inserted.
                        // adding edges after the first graph creation and edge insertion
                        // isnt allowed in our project
                        if (edges_inserted[i])
                        {
                            std::string error = "Edges have been already inserted.\n";
                            send(i, error.c_str(), error.size(), MSG_NOSIGNAL);
                            continue;
                        }

                        // num of edge pairs
                        int numPairs_;

                        try
                        {
                            // get the number as a string
                            std::string input = tokens[1];

                            // check empty
                            if (input.empty())
                            {
                                throw std::invalid_argument("input is EMPTY!\n");
                            }

                            // check if its a number
                            for (char ch : input)
                            {
                                if (!std::isdigit(ch))
                                {
                                    throw std::invalid_argument("Number of edges must be numeric!\n");
                                }
                            }

                            // use stoi to convert it into an integer
                            numPairs_ = std::stoi(tokens[1]);

                            // check if negative
                            if (numPairs_ < 0)
                            {
                                throw std::invalid_argument("Number of edges can't be negative\n");
                            }

                            // now we will check if the EDGES input of the client
                            // is <= of the maximum possible edges for his graph
                            // we can check it by the number of vertices
                            int v = graphs[i]->get_num_vertices();

                            int max_edges;

                            // for directed graphs
                            if (is_directed[i])
                            {
                                max_edges = v * (v - 1);
                            }
                            else // undirected graphs
                            {
                                max_edges = (v * (v - 1)) / 2;
                            }

                            // if greater.. this is indeed a logical error
                            if (numPairs_ > max_edges)
                            {
                                throw std::invalid_argument("number of edges cant be higher than " + std::to_string(max_edges));
                            }
                        }
                        // catch any exception within the process
                        catch (const std::exception &ex)
                        {
                            std::string error_usage = std::string("invalid input: ") + ex.what() + "\n";
                            send(i, error_usage.c_str(), error_usage.size(), MSG_NOSIGNAL);
                            continue;
                        }

                        // for loop for the amount of edge pairs specified
                        for (int j = 0; j < numPairs_; j++)
                        {
                            // clear buffer
                            memset(buffer, 0, BUFFER_SIZE);

                            // recieve input
                            int b = recv(i, buffer, BUFFER_SIZE - 1, 0);

                            if (b <= 0)
                            {
                                break;
                            }

                            //
                            std::string edge_weight = buffer;

                            std::replace(edge_weight.begin(), edge_weight.end(), ',', ' ');

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
                                send(i, err.c_str(), err.size(), MSG_NOSIGNAL);
                                j--;
                                continue;
                            }

                            try
                            {
                                // weve deciced to define the weights within [1,25] for simplicity
                                if (weight < 0 || weight > 25)
                                {
                                    std::string error = "edge weight must be in range [1,25]\n";
                                    send(i, error.c_str(), error.size(), MSG_NOSIGNAL);
                                    j--;
                                    continue;
                                }

                                // we dont allow self loops for simplicity
                                if (source == target)
                                {
                                    std::string err = "self loops not allowed for our graphs\n";
                                    send(i, err.c_str(), err.size(), MSG_NOSIGNAL);
                                    j--;
                                    continue;
                                }

                                // check if source and target are already connected
                                if (graphs[i]->is_connected(source, target))
                                {
                                    std::string err = "duplicate edge ignored\n";
                                    send(i, err.c_str(), err.size(), MSG_NOSIGNAL);
                                    j--;
                                    continue;
                                }

                                // check if the source and target are within the bounds
                                if (source < 0 || target < 0 || source >= graphs[i]->get_num_vertices() || target >= graphs[i]->get_num_vertices())
                                {
                                    std::string err = "vertex out of bounds\n";
                                    send(i, err.c_str(), err.size(), MSG_NOSIGNAL);
                                    j--;
                                    continue;
                                }

                                // add edge function implemented in Graph.cpp in part 1
                                // this function accounts for both directed and undirected graphs
                                graphs[i]->add_edge(source, target, weight);
                            }
                            // catch any execption
                            catch (std::exception &e)
                            {
                                std::string err = std::string("ERROR: ") + e.what() + "\n";
                                send(i, err.c_str(), err.size(), MSG_NOSIGNAL);
                            }
                        }

                        // after a successful edge insertion
                        // we give the client a confirmation message
                        // and set the bool edges_inserted[i] to true
                        // to avoid further additions to the graph.
                        std::string ok = "edges inserted\n";
                        edges_inserted[i] = true;
                        send(i, ok.c_str(), ok.size(), MSG_NOSIGNAL);
                    }

                    // find the clients algorithm name input, if not found then we go to the else condition
                    // which sends "Unknown command" back to the client.
                    else if (algorithms.count(cmd))
                    {
                        // we check if the graph has not been created yet
                        // or edges havent been inserted
                        if (!graph_created[i] || !edges_inserted[i])
                        {
                            std::string error_msg = "NO Graph defined yet!\n";
                            send(i, error_msg.c_str(), error_msg.size(), MSG_NOSIGNAL);
                            continue;
                        }

                        // if condition that checks whether the graph is directed and the algo chosen in one of
                        // the following: MSTW , HAM , CLIQUE then
                        if ((cmd == "MSTW" || cmd == "CLIQUE") && is_directed[i])
                        {
                            std::string error_msg = cmd + string(": only works for undirected graphs\n");
                            send(i, error_msg.c_str(), error_msg.size(), MSG_NOSIGNAL);
                            continue;
                        }

                        // scc algo. this algo cant be invoked on an undirected graph
                        if (cmd == "SCC" && !is_directed[i])
                        {
                            std::string error_msg = cmd + string(": only works for directed graphs\n");
                            send(i, error_msg.c_str(), error_msg.size(), MSG_NOSIGNAL);
                            continue;
                        }

                        try
                        {
                            // create the algo factory instance
                            auto algo = AlgorithmFactory::create_algo(cmd);

                            // invoke the specified algorithm
                            std::string ans = algo->execute(*graphs[i]);

                            // check if its empty
                            if (!ans.empty())
                            {
                                send(i, ans.c_str(), ans.size(), MSG_NOSIGNAL);
                            }
                        } // catch any exception within the process of calculation
                        catch (const std::exception &e)
                        {
                            std::string error_msg = std::string("ERROR: ") + e.what() + "\n";
                            send(i, error_msg.c_str(), error_msg.size(), MSG_NOSIGNAL);
                            continue;
                        }
                    }
                    else
                    {
                        // client gave an unknown command
                        std::string error = "Unknown command. Please try again!\n";
                        send(i, error.c_str(), error.size(), MSG_NOSIGNAL);
                        continue;
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{

    // Usage is - ./graph_server <host> <port>
    // for example ./graph_server 127.0.0.1 1234
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <host> <port>\n";
        return 1;
    }

    // create a socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // if socket fails
    if (server_fd == -1)
    {
        perror("socket failed");
        exit(1);
    }

    // allow reusing the port in case of restarts
    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // extarct host
    const char *host_name = argv[1];

    // extarct port
    int port = std::stoi(argv[2]);

    // check if the port is valid
    assert(port > 0 && port < 65535);

    // hints for getaddrinfo
    // in order to resolve th host and the port
    struct addrinfo hints{};
    struct addrinfo *res;
    struct addrinfo *rp;

    // clear hints
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

    // intro
    // usage for server
    // this is mainly to notify that you can type QUIT
    // to shut down the server
    usage_for_server(port);

    // call the main function
    // that processes the clients
    process_client(server_fd);

    // close  listening socket
    close(server_fd);

    return 0;
}