#include "../Part_1/Graph.hpp"
#include "Euler_circle_algo.hpp"
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
#include <unordered_map>
#include <memory>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#define BUFFER_SIZE 1024

using namespace std;

/// @brief  usage and introduction for the client.
std::string introduction_for_client()
{
    std::string intro =
        "Welcome to Our Graph Algorithms server - \n"
        "Commands Usage: \n"
        "Newgraph <num of vertices> <D|U> (commas also accepted)\n"
        "EDGES <num of edges> (each edge: source,target,weight)\n"
        "EULER\n";
    return intro;
}

void usage_for_server(int port)
{
    std::cout << "Graph Magician started on port " << port << std::endl;
    std::cout << "type QUIT to shut down the server" << "\n";
}

/// @brief handle all communication with a client
/// @param client_sock client socket file descriptor
void process_client(int server_fd)
{
    // ensure server fd is part of the monitored set
    fd_set mas_set;
    fd_set read_set;

    FD_ZERO(&mas_set);
    FD_ZERO(&read_set);

    FD_SET(server_fd, &mas_set);
    int fd_max = server_fd;

    // build the introduction message for client
    std::string introduction = introduction_for_client();

    // map for client's graphs
    std::unordered_map<int, std::unique_ptr<Graph>> graphs;

    // map the bool graph_created for every client
    std::unordered_map<int, bool> graph_created;

    // map the direction of every graph created by each client
    std::unordered_map<int, bool> is_directed;

    std::unordered_map<int, bool> edges_inserted;

    // buffer for client
    char buffer[BUFFER_SIZE];

    // add stdin (standart input) to the master set
    // this will alow the server to accept commands
    // in the server
    FD_SET(STDIN_FILENO, &mas_set);

    // check if stdin fd is greater than server_fd
    // update accordingly to that select() will check enough fds
    if (STDIN_FILENO > fd_max)
    {
        fd_max = STDIN_FILENO;
    }

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

                    // add clients socket to the master socket
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

                    // receive bytes from client
                    int bytes = recv(i, buffer, BUFFER_SIZE - 1, 0);

                    // if bytes == 0 then the client has disconnected
                    // otherwise , there was a connection error
                    if (bytes <= 0)
                    {
                        if (bytes == 0)
                        {
                            // if client disconnected
                            std::cout << "A client has disconnected" << std::endl;
                        }
                        else
                        {
                            perror("bytes recv");
                        }

                        // close file descriptor
                        close(i);

                        // remove it from the master set
                        FD_CLR(i, &mas_set);

                        // erase the clients graph from the map
                        graphs.erase(i);

                        // erase the clients graph_created flag from the map
                        graph_created.erase(i);

                        // erase the client's bool flag is_directed from the map
                        is_directed.erase(i);

                        continue;
                    }

                    // convert the line buffer to a string
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
                            std::string correction_usage = "usage is Newgraph <num> <D|U>\n";
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

                        // number of vertrices can't be 0 or negative
                        if (num_vxs <= 0)
                        {
                            std::string usage = "number of vertices must be greater than 0!\n";
                            send(i, usage.c_str(), usage.size(), MSG_NOSIGNAL);
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

                        //// check the vector tokens size
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
                                throw std::invalid_argument("input is EMPTY!");
                            }

                            // check if its a number
                            for (char ch : input)
                            {
                                if (!std::isdigit(ch))
                                {
                                    throw std::invalid_argument("number must be numeric!");
                                }
                            }

                            // use stoi to convert it into an integer
                            numPairs_ = std::stoi(tokens[1]);

                            // check if negative
                            if (numPairs_ < 0)
                            {
                                throw std::invalid_argument("value can't be negative");
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
                            edges_inserted[i] = false;
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

                            // replace commas with blank
                            std::replace(edge_weight.begin(), edge_weight.end(), ',', ' ');

                            // wrap it as a stringstream and get input stream
                            std::istringstream eiss(edge_weight);

                            // source vx
                            int source;
                            // target vx
                            int target;
                            // weight of the edge
                            int weight;

                            // check if format is valid
                            if (!(eiss >> source >> target >> weight))
                            {
                                std::string err = "invalid edge format (use source,target,weight)\n";
                                send(i, err.c_str(), err.size(), MSG_NOSIGNAL);
                                j--;
                                continue;
                            }

                            try
                            {

                                // weights in are graph are defined to be between the values [1,25]
                                // for simplicity
                                if (weight < 0 || weight > 25)
                                {
                                    std::string error = "edge weight must be in range [1,25]\n";
                                    send(i, error.c_str(), error.size(), MSG_NOSIGNAL);
                                    j--;
                                    continue;
                                }

                                // self loops arent allowed. for simplicity of course
                                if (source == target)
                                {
                                    std::string err = "self loops not allowed for our graphs\n";
                                    send(i, err.c_str(), err.size(), MSG_NOSIGNAL);
                                    j--;
                                    continue;
                                }

                                // duplicate edge condition
                                if (graphs[i]->is_connected(source, target))
                                {
                                    std::string err = "duplicate edge ignored\n";
                                    send(i, err.c_str(), err.size(), MSG_NOSIGNAL);
                                    j--;
                                    continue;
                                }

                                // check for out of bound vertices
                                if (source < 0 || target < 0 || source >= graphs[i]->get_num_vertices() || target >= graphs[i]->get_num_vertices())
                                {
                                    std::string err = "vertex out of bounds\n";
                                    send(i, err.c_str(), err.size(), MSG_NOSIGNAL);
                                    j--;
                                    continue;
                                }

                                // add_edge functionallity can handle both directed and undirected graphs
                                // inputs from our client..

                                graphs[i]->add_edge(source, target, weight);
                            }
                            // catch error
                            catch (std::exception &e)
                            {
                                std::string err = std::string("ERROR: ") + e.what() + "\n";
                                send(i, err.c_str(), err.size(), MSG_NOSIGNAL);
                                edges_inserted[i] = false;
                            }
                        }

                        // validation message
                        std::string ok = "edges inserted\n";
                        // set bool flag to true to avoid further edge additions
                        edges_inserted[i] = true;
                        send(i, ok.c_str(), ok.size(), MSG_NOSIGNAL);
                    }
                    // EULER algo section
                    else if (cmd == "EULER")
                    {
                        // client wont be able to use EULER
                        // if:
                        // 1. graph hasnt been created
                        // 2. edges hasnt been inserted
                        if (!graph_created[i] || !edges_inserted[i])
                        {
                            std::string err = "You have to create a graph first!\n";
                            send(i, err.c_str(), err.size(), MSG_NOSIGNAL);
                            continue;
                        }

                        try
                        {
                            // invoke the calculation function inside Euler_Circle_algo.cpp
                            std::string ans = find_euler_circle_string(*graphs[i]);

                            // check if its empty
                            if (!ans.empty())
                            {
                                send(i, ans.c_str(), ans.size(), MSG_NOSIGNAL);
                            }
                        }
                        // catch any exception during calculation process
                        catch (const std::exception &ex)
                        {
                            std::string err = std::string("ERROR: ") + ex.what() + "\n";
                            send(i, err.c_str(), err.size(), MSG_NOSIGNAL);
                        }
                    }
                    else
                    {
                        // client gives unknown input
                        std::string unk = "Unknown command\n";
                        send(i, unk.c_str(), unk.size(), MSG_NOSIGNAL);
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <host> <port>\n";
        return 1;
    }

    // create the server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket failed");
        exit(1);
    }

    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // extract the host_name from the arguments
    const char *host_name = argv[1];

    // extract port num
    int port = std::stoi(argv[2]);

    // check if port is valid
    assert(port > 0 && port < 65535);

    // create server address
    struct addrinfo hints{};
    struct addrinfo *res;
    struct addrinfo *rp;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // ipv4
    hints.ai_socktype = SOCK_STREAM; // tcp
    hints.ai_flags = AI_PASSIVE;

    int err = getaddrinfo(host_name, argv[2], &hints, &res);
    if (err != 0)
    {
        std::cerr << "getaddrinfo: " << gai_strerror(err) << "\n";
        close(server_fd);
        return 1;
    }

    for (rp = res; rp != nullptr; rp = rp->ai_next)
    {
        if (bind(server_fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
    }

    if (!rp)
    {
        perror("bind failed");
        freeaddrinfo(res);
        exit(1);
    }

    freeaddrinfo(res);

    // listen to up to 10 clients
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

    // process all commands for this client
    process_client(server_fd);

    close(server_fd);

    return 0;
}