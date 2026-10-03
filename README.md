# Multithreaded Graph Algorithms Server (C++)

A TCP server, written in C++, that lets clients build graphs over the network and run
classic graph algorithms on them. The project was built iteratively: it starts as a
plain graph library, grows into a single-threaded network server, and ends as a
multithreaded, pipelined service — with each stage profiled and memory/thread-safety
tested along the way.

## Core Graph Library (`Part_1`)

- `Node` — a graph vertex: an id plus a list of `(neighbor_id, edge_weight)` pairs.
- `Graph` — stores the vertex list, supports add/remove vertex & edge, tracks a
  directed/undirected flag, and exposes helpers (`degree`, `indegree`,
  `contains_edge`, a stream `operator<<`) reused by every later stage.

## Algorithms

Implemented via a **Factory pattern** (`AlgorithmFactory`), each returning a
printable result for the current graph:

| Algorithm | What it computes |
|---|---|
| **Euler Circuit** (`EulerCircuit`) | Finds an Eulerian circuit using **Hierholzer's algorithm**; validates it exists first (even degree / balanced in-out degree + connectivity checks) |
| **Hamiltonian Circuit** (`HamCircuit`) | Finds a Hamiltonian circuit |
| **MST** (`MSTW`) | Minimum spanning tree / weight |
| **SCC** (`SCC`) | Strongly connected components |
| **Max Clique** (`MaxClique`) | Largest clique in the graph |

The server tracks which algorithms are valid for the current graph (some only apply
to directed graphs, others only to undirected) and reports back accordingly.

## Server Evolution

Each `Part_x` is a complete, runnable milestone of the same server:

| Stage | Architecture |
|---|---|
| `Part_2`, `Part_3` | Standalone CLI: builds/generates a graph (`random_graph`, `getopt`-based CLI flags for vertex/edge count, density, direction) and runs the Euler algorithm on it |
| `Part_6` | First network version: a **`select()` event loop** multiplexing stdin + all client sockets; each client can build a graph, add edges, and request `EULER`; per-client state tracked in `unordered_map`s keyed by socket fd |
| `Part_7` | Same `select()` loop, extended with `HAM`, `MSTW`, `SCC`, `CLIQUE` commands dispatched through the new `AlgorithmFactory` |
| `Part_8` | Swapped `select()` for a **leader-follower thread pool** (pthreads): workers coordinate with a mutex + condition variable so only one thread accepts a new connection at a time, while all workers process requests concurrently; added a `RandomGraph` command |
| `Part_9` | Final concurrency model: a **Pipeline / Active-Object architecture**. `ActiveGraph` is an active object with its own worker thread and job queue; a `Pipeline` chains five of them (Euler → Hamiltonian → MST → SCC → Max Clique) so each client's graph request flows through dedicated per-stage workers, with thread-safe output via `safe_send()` |

## Testing & Profiling

Every server iteration (`Part_2`, `3`, `6`, `7`, `8`, `9`) was validated with:

- **valgrind memcheck** — memory-leak detection
- **valgrind helgrind** — data-race / thread-safety detection (critical once the
  leader-follower pool and pipeline were introduced)
- **callgrind** — call-graph profiling
- **gprof** — function-level CPU profiling (e.g. running the Euler algorithm
  1,000,000 times to find hotspots)
- **Coverage reports** — multiple runs exercising invalid inputs, exceptions, and
  edge cases

Reports for each stage are included under their respective `Part_x/` folder.

## Build & Run

Each part is self-contained with its own `makefile`:

```bash
cd Part_9
make
./graph_server
```

Connect with any TCP client and send commands such as `Newgraph`, `AddEdge`,
`EULER`, `HAM`, `MSTW`, `SCC`, `CLIQUE`, `RandomGraph`, `QUIT`.

## Tech Stack

C++ · POSIX sockets · pthreads · `select()` · leader-follower thread pool ·
Active Object / Pipeline pattern · Factory pattern · valgrind · gprof · callgrind
