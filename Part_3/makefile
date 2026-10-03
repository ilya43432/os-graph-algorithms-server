CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
COVERAGE_FLAGS = --coverage
PROFILING_FLAGS = -pg

TARGET = random_graph
PROF_TARGET = random_graph_prof
SRC = random_graph.cpp ../Part_1/Graph.cpp Euler_circle_algo.cpp

TEST_FILE = text.txt     
TEST_FILE_PROF = profiler.txt     
TEST_FILE_MEM = memcheck.txt      

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) -I../Part_1 -o $(TARGET) $(SRC)

$(PROF_TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(PROFILING_FLAGS) -I../Part_1 -o $(PROF_TARGET) $(SRC)

run: $(TARGET)
	-while read line; do \
		echo "Running: $$line"; \
		$$line; \
	done < $(TEST_FILE)

reports: all run $(PROF_TARGET)
	# --- Coverage ---
	gcov -b -c --object-file random_graph-random_graph.gcda random_graph.cpp > coverage_report.txt
	gcov -b -c --object-file random_graph-Graph.gcda ../Part_1/Graph.cpp >> coverage_report.txt
	gcov -b -c --object-file random_graph-Euler_circle_algo.gcda Euler_circle_algo.cpp >> coverage_report.txt

	while read line; do \
		./$(PROF_TARGET) $$line > /dev/null; \
	done < $(TEST_FILE_PROF)
	gprof ./$(PROF_TARGET) gmon.out > gprof_report.txt

	while read line; do \
		valgrind --leak-check=full --show-leak-kinds=all $$line; \
	done < $(TEST_FILE_MEM) 2>&1 | tee valgrind_report.txt

	valgrind --tool=callgrind ./$(TARGET) -v 8 -e 12 -s 1 -u
	callgrind_annotate callgrind.out.* > callgraph_report.txt

clean:
	rm -f $(TARGET) $(PROF_TARGET) *.o ../Part_1/*.o ../Part_1/*.gcno ../Part_1/*.gcda ../Part_1/*.gcov *.gcno *.gcda *.gcov \
	      gmon.out callgrind.out.* 