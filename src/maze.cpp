#include "generator/mazegenerator.hpp"
#include "solver/mazesolver.hpp"
#include <string>
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cout << "Usage: " << argv[0] << " -g [bfs/kruskal] -s [dfs/dijkstra]" << endl;
        return 1;
    }
    string st = argv[1];
    string algo1_str;
    string algo2_str;
    int algo1 = 1;
    int algo2 = 1;
    if(st=="-g"){
        algo1_str = argv[2];
        algo2_str = argv[4];
    }
    else{
        algo2_str = argv[2];
        algo1_str = argv[4];
    }
    if(algo1_str=="bfs")
        algo1 = 0;
    if(algo2_str=="dfs")
        algo2 = 0;
    mazegenerator(algo1);
    mazesolver(algo2);
}