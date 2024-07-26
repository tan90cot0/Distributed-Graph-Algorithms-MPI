#include <mpi.h>
#include <random>
#include <fstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <map>
#include <iostream>

using namespace std;

// uncomment below for unseeded
auto rd_dijkstra = std::random_device {}; 
auto rng_dijkstra = default_random_engine {rd_dijkstra()};

int dx_dijkstra[4] = {-1, 0, 1, 0};
int dy_dijkstra[4] = {0, 1, 0, -1};

// check if (x, y) is inside_dijkstra r x c
bool inside_dijkstra(int x, int y, int r, int c) {
	return ((x >= 0) && (x < r) && (y >= 0) && (y < c));
}

// find the predecessor for each passable cell
// the tree is rooted at (x, y)
void g_dijkstra(bool A[], int pred[], int r, int c, int x, int y) {
	pred[x*c + y] = x*c + y;
	vector<bool> vis(r*c, false);
	vector<int> dis(r*c, 1e6);	// initialise distances to be inf = 1e6
	// distance of start vertex is 0
	dis[x*c + y] = 0;
	priority_queue<pair<int, pair<int, int>>> frontier;
	frontier.push({0, {x, y}});

	vector<bool> processed(r*c, false);

	while (!frontier.empty()) {
		pair<int, int> u = frontier.top().second;
		frontier.pop();
		if (processed[u.first*c + u.second])
			continue;
		processed[u.first*c + u.second] = true;
		for (int t = 0; t < 4; t++) {
			int nx = u.first + dx_dijkstra[t], ny = u.second + dy_dijkstra[t];
			if (inside_dijkstra(nx, ny, r, c) && A[nx*c + ny]) {
				if (dis[u.first*c + u.second] + 1 < dis[nx*c + ny]) {
					dis[nx*c + ny] = dis[u.first*c + u.second] + 1;
					frontier.push({-dis[nx*c+ny], {nx, ny}});
					pred[nx*c + ny] = u.first*c + u.second;
				}
			}
		}
	}
}

void dijkstra(){
	int n, p;
	MPI_Comm_size(MPI_COMM_WORLD, &n);
	MPI_Comm_rank(MPI_COMM_WORLD, &p);
	std::ifstream inFile("aryan_harihar_divyansh.txt");

	int r = 64, c = 64, i, j;

	// read in maze from the master process
	bool *maze = NULL;
	maze = (bool*)malloc((r*c) * sizeof(bool));
	if (p == 0) {
		for (i = 0; i < r; i++) {
			for (j = 0; j < c; j++) {
				inFile >> maze[i*c + j];
			}
		}
	}
	MPI_Bcast(maze, r*c, MPI_C_BOOL, 0, MPI_COMM_WORLD);

	int *startx = NULL, *starty = NULL;
	startx = (int*)malloc(n*sizeof(int));
	starty = (int*)malloc(n*sizeof(int));
	// compute start locations for each processor
	if (p == 0) {
		startx[0] = 0;
		starty[0] = r - 1;
		int idx = 1;
		for (i = r/n; i < r; i += r/n) {
			for (j = 0;j < c; j++) {
				if (maze[i*c+j] && maze[(i-1)*c + j]) {
					startx[idx] = r/n;
					starty[idx] = j;
					idx += 1;
					break;
				}
			}
		}
	}
	MPI_Bcast(startx, n, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(starty, n, MPI_INT, 0, MPI_COMM_WORLD);

	int pred[r*c];
	g_dijkstra(&maze[p*(r/n)], pred, r, c, startx[p], starty[p]);

	pred[startx[p]*c + starty[p]] -= c;

	int *par;
	if (p == 0) {
		par = (int*)malloc(r*c*sizeof(int));
	}
	MPI_Gather(pred, (r/n)*c, MPI_INT, par, (r/n)*c, MPI_INT, 0, MPI_COMM_WORLD);

	if (p == 0) {
		char path[r*c];
		for (i = 0; i < r; i++){
			for (j = 0; j < c; j++) {
				if(maze[i*c+j])
					path[i*c+j] = ' ';
				else
					path[i*c+j] = '*';
			}
		}
		int cur = (r - 1)*c;
		while (cur != c - 1) {
			path[cur] = 'P';
			cur = pred[cur];
		}
		path[c - 1] = 'S';
		path[(r - 1)*c] = 'E';
		for (i = 0; i < r; i++) {
			for (j = 0; j < c; j++) {
				cout << path[i*c + j];
			}
			cout << '\n';
		}
	}

	if (p == 0) {
		free(par);
	}

	free(maze);
	free(startx);
	free(starty);

	MPI_Finalize();
}