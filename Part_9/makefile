CXX = g++
CXXFLAGS_COVERAGE = -std=c++17 -Wall -Wextra -g -fprofile-arcs -ftest-coverage
CXXFLAGS_NO_COVERAGE = -std=c++17 -Wall -Wextra -g
CXXFLAGS_PROFILE = -std=c++17 -Wall -Wextra -pg -g

INCLUDES = -I. -I../Part_7/Algorithms -I../Part_1

TARGET_COV = graph_server
TARGET_TEST = graph_server_test
TARGET_PROF = graph_server_prof

SOURCES = graph_server.cpp \
          random_graph.cpp \
		  Pipeline_Pattern.cpp \
		  ActiveGraph.cpp \
          ../Part_1/Graph.cpp \
          ../Part_7/Algorithms/EulerCircuit.cpp \
          ../Part_7/Algorithms/MSTW.cpp \
          ../Part_7/Algorithms/HamCircuit.cpp \
          ../Part_7/Algorithms/MaxClique.cpp \
          ../Part_7/Algorithms/SCC.cpp \
          ../Part_7/Algorithms/AlgorithmFactory.cpp

OBJECTS_COVERAGE = $(SOURCES:.cpp=.o)
OBJECTS_NO_COVERAGE = $(SOURCES:.cpp=.test.o)
OBJECTS_PROFILE = $(SOURCES:.cpp=.prof.o)

all: $(TARGET_COV)

test: $(TARGET_TEST)

# coverage
$(TARGET_COV): $(OBJECTS_COVERAGE)
	$(CXX) $(CXXFLAGS_COVERAGE) $(INCLUDES) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS_COVERAGE) $(INCLUDES) -c $< -o $@

../Part_1/%.o: ../Part_1/%.cpp
	$(CXX) $(CXXFLAGS_COVERAGE) $(INCLUDES) -c $< -o $@

../Part_7/Algorithms/%.o: ../Part_7/Algorithms/%.cpp
	$(CXX) $(CXXFLAGS_COVERAGE) $(INCLUDES) -c $< -o $@

# test the program
$(TARGET_TEST): $(OBJECTS_NO_COVERAGE)
	$(CXX) $(CXXFLAGS_NO_COVERAGE) $(INCLUDES) -o $@ $^

%.test.o: %.cpp
	$(CXX) $(CXXFLAGS_NO_COVERAGE) $(INCLUDES) -c $< -o $@

../Part_1/%.test.o: ../Part_1/%.cpp
	$(CXX) $(CXXFLAGS_NO_COVERAGE) $(INCLUDES) -c $< -o $@

../Part_7/Algorithms/%.test.o: ../Part_7/Algorithms/%.cpp
	$(CXX) $(CXXFLAGS_NO_COVERAGE) $(INCLUDES) -c $< -o $@

## profiling
$(TARGET_PROF): $(OBJECTS_PROFILE)
	$(CXX) $(CXXFLAGS_PROFILE) $(INCLUDES) -o $@ $^

%.prof.o: %.cpp
	$(CXX) $(CXXFLAGS_PROFILE) $(INCLUDES) -c $< -o $@

../Part_1/%.prof.o: ../Part_1/%.cpp
	$(CXX) $(CXXFLAGS_PROFILE) $(INCLUDES) -c $< -o $@

../Part_7/Algorithms/%.prof.o: ../Part_7/Algorithms/%.cpp
	$(CXX) $(CXXFLAGS_PROFILE) $(INCLUDES) -c $< -o $@


coverage: $(TARGET_COV)
	@gcov -b -c -o . graph_server.cpp >> coverage_report.txt
	@gcov -b -c -o . random_graph.cpp >> coverage_report.txt
	@gcov -b -c -o . Pipeline_Pattern.cpp >> coverage_report.txt
	@gcov -b -c -o .  ActiveGraph.cpp >> coverage_report.txt
	@gcov -b -c -o ../Part_1 ../Part_1/Graph.cpp >> coverage_report.txt
	@gcov -b -c -o ../Part_7/Algorithms ../Part_7/Algorithms/MSTW.cpp >> coverage_report.txt
	@gcov -b -c -o ../Part_7/Algorithms ../Part_7/Algorithms/EulerCircuit.cpp >> coverage_report.txt
	@gcov -b -c -o ../Part_7/Algorithms ../Part_7/Algorithms/HamCircuit.cpp >> coverage_report.txt
	@gcov -b -c -o ../Part_7/Algorithms ../Part_7/Algorithms/MaxClique.cpp >> coverage_report.txt
	@gcov -b -c -o ../Part_7/Algorithms ../Part_7/Algorithms/SCC.cpp >> coverage_report.txt
	@gcov -b -c -o ../Part_7/Algorithms ../Part_7/Algorithms/AlgorithmFactory.cpp >> coverage_report.txt

valgrind-memcheck: $(TARGET_COV)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET_COV) 127.0.0.1 1234 2>&1 | tee -a valgrind_memcheck.txt

valgrind-callgrind: $(TARGET_COV)
	valgrind --tool=callgrind ./$(TARGET_COV) 127.0.0.1 1234
	callgrind_annotate callgrind.out.* > callgrind_report.txt

valgrind-hellgrind: $(TARGET_TEST)
	valgrind --tool=helgrind ./$(TARGET_TEST) 127.0.0.1 1234 2>&1 | tee helgrind_report.txt

profile: $(TARGET_PROF)
	./$(TARGET_PROF) 127.0.0.1 1234
	@gprof $(TARGET_PROF) gmon.out > gprof_report.txt

test-client:
	while read -r line; do \
		echo $$line; \
		sleep 0.2; \
	done < text.txt | nc 127.0.0.1 1234

clean:
	rm -f $(TARGET_COV) $(TARGET_TEST) $(OBJECTS_COVERAGE) $(OBJECTS_NO_COVERAGE) \
	      *.o ../Part_1/*.o ../Part_7/Algorithms/*.o \
	      *.gcno *.gcda *.gcov ../Part_1/*.gcno ../Part_1/*.gcda ../Part_1/*.gcov \
	      ../Part_7/Algorithms/*.gcno ../Part_7/Algorithms/*.gcda ../Part_7/Algorithms/*.gcov \
	      *.out callgrind.out.*

.PHONY: all test clean coverage valgrind-memcheck valgrind-callgrind valgrind-hellgrind test-client