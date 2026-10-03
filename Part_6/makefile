CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -fprofile-arcs -ftest-coverage
INCLUDES = -I../Part_1 -I../Part_2

SOURCES = graph_server.cpp ../Part_1/Graph.cpp Euler_circle_algo.cpp
OBJECTS = graph_server.o ../Part_1/Graph.o Euler_circle_algo.o
TARGET = graph_server

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

graph_server.o: graph_server.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

../Part_1/Graph.o: ../Part_1/Graph.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

Euler_circle_algo.o: Euler_circle_algo.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

coverage:
	@gcov -b -c graph_server.cpp ../Part_1/Graph.cpp Euler_circle_algo.cpp > coverage_report.txt

valgrind-memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) 127.0.0.1 1234 2>&1 | tee -a valgrind_memcheck.txt

# 3. Valgrind Callgrind
valgrind-callgrind: $(TARGET)
	valgrind --tool=callgrind ./$(TARGET) 127.0.0.1 1234
	callgrind_annotate callgrind.out.* > callgrind_report.txt

test-client:
	while read -r line; do \
		echo $$line; \
		sleep 0.3; \
	done < text.txt | nc 127.0.0.1 1234

clean:
	rm -f $(TARGET) *.o ../Part_1/*.o ../Part_1/*.gcno ../Part_1/*.gcda ../Part_1/*.gcov *.gcno *.gcda *.gcov 

.PHONY: all clean coverage valgrind-memcheck valgrind-callgrind