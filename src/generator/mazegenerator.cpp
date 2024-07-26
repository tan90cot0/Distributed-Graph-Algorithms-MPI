#include "bfs.hpp"
#include "kruskal.hpp"

void mazegenerator(int algo){
    if(algo==0)
        bfs();
    else
        kruskal();
}