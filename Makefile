CXX = g++
CXXFLAGS = -std=c++20 -pthread -O2

# Цели по умолчанию
all: Task1 Task2

LinkedList.o: LinkedList.cpp LinkedList.h
	$(CXX) $(CXXFLAGS) -c LinkedList.cpp -o LinkedList.o

LiveCounter.o: LiveCounter.cpp LiveCounter.h
	$(CXX) $(CXXFLAGS) -c LiveCounter.cpp -o LiveCounter.o

Task1: Task1.cpp LiveCounter.o
	$(CXX) $(CXXFLAGS) Task1.cpp LiveCounter.o -o Task1

Task2: Task2.cpp LiveCounter.o
	$(CXX) $(CXXFLAGS) Task2.cpp LiveCounter.o -o Task2

Task3: Task3.cpp LiveCounter.o
	$(CXX) $(CXXFLAGS) Task3.cpp LiveCounter.o -o Task3

Task4: Task4.cpp LinkedList.o
	$(CXX) $(CXXFLAGS) Task4.cpp LinkedList.o -o Task4

Task5: Task5.cpp 
	$(CXX) $(CXXFLAGS) Task5.cpp -o Task5

Task6: Task6.cpp 
	$(CXX) $(CXXFLAGS) Task6.cpp -o Task6

	
Task8: Task8.cpp 
	$(CXX) $(CXXFLAGS) Task8.cpp -o Task8

Task9: Task9.cpp 
	$(CXX) -fopenmp -O3 -march=native $(CXXFLAGS) Task9.cpp -o Task9

run1: Task1
	./Task1

run2: Task2
	./Task2

run3: Task3
	./Task3

run4: Task4
	./Task4

run5: Task5
	./Task5

run6: Task6
	./Task6

run8: Task8
	./Task8

run9: Task9
	./Task9

clean:
	rm -f *.o Task1 Task2 Task3 Task4 Task5  Task6  Task8 Task9 LiveCounter.o snapshot_log.txt LinkedList.o

# Псевдонимы
build_LiveCounter: LiveCounter.o

build_Task1: Task1

build_Task2: Task2

build_Task3: Task3

build_Task4: Task4

build_Task5: Task5

build_Task6: Task6

build_Task8: Task8

build_Task9: Task9

.PHONY: all clean run1 run2 run3 run4 run5 run6 run8 run9 build_LiveCounter build_Task1 build_Task2 build_Task3 build_Task4 build_Task5 build_Task6 build_Task8