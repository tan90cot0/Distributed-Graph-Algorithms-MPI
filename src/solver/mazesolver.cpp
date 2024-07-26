#include "dfs.hpp"
#include "dijkstra.hpp"

void mazesolver(int algo){
    if(algo==0)
        dfs();
    else
        dijkstra();
}