#include <mpi.h>
#include <fstream>
#include <random>
#include <vector>
#include <queue>
#include <algorithm>
#include <map>
#include <iostream>

using namespace std;

// uncomment below for unseeded
auto rd_dfs = std::random_device {}; 
auto rng_dfs = default_random_engine {rd_dfs()};

int dx_dfs[4] = {-1, 0, 1, 0};
int dy_dfs[4] = {0, 1, 0, -1};

// check if (x, y) is inside_dfs r x c
bool inside_dfs(int x, int y, int r, int c) {
	return ((x >= 0) && (x < r) && (y >= 0) && (y < c));
}

void dfs (int ux, int uy, bool A[], vector<bool> &vis, int pred[], int r, int c) {
	if (vis[ux*c + uy])
		return;
	vis[ux*c + uy] = true;
	for (int t = 0; t < 4; t++) {
		int nx = ux + dx_dfs[t], ny = uy + dy_dfs[t];
		if (inside_dfs(nx, ny, r, c) && A[nx*c + ny] & !vis[nx*c + ny]) {
			pred[nx*c + ny] = ux*c + uy;
			dfs(nx, ny, A, vis, pred, r, c);
		}
	}
}

// find the predecessor for each passable cell
// the tree is rooted at (x, y)
void g_dfs(bool A[], int pred[], int r, int c, int x, int y) {
	pred[x*c + y] = x*c + y;
	vector<bool> vis(r*c, false);
	dfs(x, y, A, vis, pred, r, c);
}

void dfs(){
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
	g_dfs(&maze[p*(r/n)], pred, r, c, startx[p], starty[p]);

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