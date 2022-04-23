
COMPILE = clang++
FLAGS = -std=c++11 -Wall -O

benchmark : hashes.hpp benchmark.cpp
	${COMPILE} ${FLAGS} benchmark.cpp -o benchmark

grade : benchmark
	./benchmark naive    10000 || echo "\nNAIVE CRASHED"
	./benchmark chain  1000000 || echo "\nCHAIN CRASHED"
	./benchmark lp     1000000 || echo "\nLP CRASHED"
	./benchmark cuckoo 1000000 || echo "\nCUCKOO CRASHED"

clean:
	rm -f benchmark
