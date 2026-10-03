CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
COVERAGE_FLAGS = --coverage
PROFILING_FLAGS = -pg

TARGET = Euler_circle_algo
PROF_TARGET = Euler_circle_algo_Prof

SRC = Euler_circle_algo.cpp ../Part_1/Graph.cpp
OBJS = $(SRC:.cpp=.o)

all: $(TARGET)

# Link final binary with coverage
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) -o $(TARGET) $(OBJS)

# Compile each .cpp into .o with coverage
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) -I../Part_1 -c $< -o $@

# Profiling build (single step)
$(PROF_TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(PROFILING_FLAGS) -I../Part_1 -o $(PROF_TARGET) $(SRC)

valgrind-memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) -v 4 -e 4 -u < input.txt > valgrind_output.txt 2>&1

valgrind-callgrind: $(TARGET)
	valgrind --tool=callgrind ./$(TARGET) -v 4 -e 4 -u < input.txt > valgrind_call_output.txt 2>&1

profile: $(PROF_TARGET)
	./$(PROF_TARGET) -v 4 -e 4 -u < input.txt > profile_output.txt
	gprof $(PROF_TARGET) gmon.out > profile_report.txt

coverage: $(TARGET)
	@gcov -b -c $(SRC) > coverage_report.txt

clean:
	rm -f $(TARGET) $(PROF_TARGET) $(OBJS) ../Part_1/*.o ../Part_1/*.gcno ../Part_1/*.gcda ../Part_1/*.gcov *.gcno *.gcda *.gcov \
	      gmon.out callgrind.out.* 
