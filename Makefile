all:
	mpic++ src/generator/bfs.cpp src/generator/kruskal.cpp src/generator/mazegenerator.cpp src/solver/dfs.cpp src/solver/dijkstra.cpp src/solver/mazesolver.cpp src/maze.cpp -o maze.out -std=c++17
run: 
	mpirun -np 4 ./maze.out -g bfs -s dfs