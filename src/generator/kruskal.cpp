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
auto rd_kruskal = std::random_device {}; 
auto rng_kruskal = default_random_engine {rd_kruskal()};

int dx_kruskal[4] = {-1, 0, 1, 0};
int dy_kruskal[4] = {0, 1, 0, -1};

map<int, int> parent;
map<int, int> dsrank;

// initialise DSU
void make_set_kruskal (int v) {
	parent.insert({v, v});
	dsrank.insert({v, 0});
}

// find representative of node
int find_set_kruskal (int v) {
	if (v == parent[v]) 
		return v;
	return parent[v] = find_set_kruskal(parent[v]);
}

// merge two sets
void union_sets_kruskal (int a, int b) {
	a = find_set_kruskal(a);
	b = find_set_kruskal(b);
	if (a != b) {
		if (dsrank[a] < dsrank[b])
			swap(a, b);
		parent[b] = a;
		if (dsrank[a] == dsrank[b])
			dsrank[a]++;
	}
}

// check if (x, y) is inside_kruskal r x c
bool inside_kruskal(int x, int y, int r, int c) {
	return ((x >= 0) && (x < r) && (y >= 0) && (y < c));
}

void itr_kruskal (bool A[], int x, int y, int px, int py, int r, int c, vector<bool> &vis, bool &detect) {
	if (vis[x*c + y])
		return;
	vis[x*c + y] = true;
	for (int d = 0; d < 4; d++){
		int nx = x + dx_kruskal[d], ny = y + dy_kruskal[d];
		if (inside_kruskal(nx, ny, r, c) && A[nx*c + ny]) {
			if (vis[nx*c + ny] && (nx!=px || ny != py)) {
				//cout << "Prev " << nx << " " << ny << " with cur " << x << " " << y << endl;
				detect = true;
			}
			else
				itr_kruskal(A, nx, ny, x, y, r, c, vis, detect);
		}
	}
}

// check if A has any cycles on adding (x, y) as a passable cell
bool no_cycle_kruskal (bool A[], int x, int y, int r, int c) {
	bool detect = false;
	vector<bool> vis(r*c, false);
	itr_kruskal(A, x, y, -1, -1, r, c, vis, detect);
	return !detect;
}

void format_kruskal(bool A[], int r, int c, int n) {
	for (int i = r/n - 1; i < r - 1; i+= r/n) {
		for (int j = 0; j < c; j++) {
			//cout << i << " " << j << " " << no_cycle_kruskal(A, i, j, r, c) << endl;
			if (!A[i*c + j] && !A[(i+1)*c + j]) {
				bool connected = false;
				for (int t = 0; t < 4; t++) {
					int nx = i + dx_kruskal[t], ny = j + dy_kruskal[t];
					if (inside_kruskal(nx, ny, r, c) && A[nx*c + ny]) 
						connected = true;
				}
				if (connected && no_cycle_kruskal(A, i, j, r, c))
					A[i*c + j] = true;
			}
		}
	}
}

// sample path uniformly at random
// (dy_kruskal - ey) horizontal steps
// c vertical steps
void sample_path_kruskal (bool A[], int sy, int ey, int r, int c, int dir) {
	vector<char> shifts;
	int i;
	for (i = 0; i < abs(sy - ey); i++) {
		shifts.push_back('H');
	}
	for (i = 0; i < r - 1; i++){
		shifts.push_back('V');
	}
	// shuffle the sequence
	shuffle(shifts.begin(), shifts.end(), rng_kruskal);
	int cur_y = sy, cur_x = 0;
	for (auto x : shifts) {
		if (x == 'H')
			cur_y += dir;
		else
			cur_x += 1;
		A[cur_x*c + cur_y] = true;
	}
}

// find perfect maze from A[start] to A[end]
// start and end are relative positions
// A is also relative
void f_kruskal(bool A[], int r, int c, pair<int, int> start, pair<int, int> end) {
	int sx = start.first, sy = start.second, ex = end.first, ey = end.second;
	int i, j, t;
	A[sx*c+sy] = true;
	if (sy >= ey)
		sample_path_kruskal(A, sy, ey, r, c, -1);
	else
		sample_path_kruskal(A, sy, ey, r, c, +1);
	
	// Kruskal to add passable cells
	vector<pair<int, int>> edges;
	for (i = 0; i < r; i++) {
		for (j = 0; j < c; j++) {
			make_set_kruskal(i*c + j);	// create sets for each node
			edges.push_back({i, j});
			//cout << A[i*c+j] << " ";
		}
		//cout << "\n";
	}

	// remove the edges on path
	for (i = 0; i < r; i++) {
		for (j = 0; j < c; j++) {
			if (A[i*c + j]) {
				for (t = 0; t < 4; t++) {
					int nx = i + dx_kruskal[t], ny = j + dy_kruskal[t];
					if (inside_kruskal(nx, ny, r, c) && A[nx*c + ny]) {
						pair<int, int> cur = {nx, ny};
						auto itr_kruskal = find(edges.begin(), edges.end(), cur);
						if (itr_kruskal != edges.end())
							edges.erase(itr_kruskal);
						pair<int, int> par = {i, j};
						auto ptr = find(edges.begin(), edges.end(), par);
						if (ptr != edges.end())
							edges.erase(ptr);
					}
				}
			}
		}
	}

	// shuffle the edges
	shuffle(edges.begin(), edges.end(), rng_kruskal);
	bool exhausted = false;

	// while we have not exhausted all the edges in the graph
	while (!exhausted) {
		bool tmp = false;
		for (auto e : edges) {
			if (A[e.first*c + e.second])
				continue;
			bool is_connected = false;
			int count = 0;
			bool cycle = false;
			for (t = 0; t < 4; t++) {
				int nx = e.first + dx_kruskal[t], ny = e.second + dy_kruskal[t];
				if (inside_kruskal(nx, ny, r, c) && A[nx*c + ny]) {
					is_connected = true;
					count += 1;
				}
				if (count > 1) {
					cycle = true;
				}
			}
			//cout << "Num cycles = " << cycle << " and connected = " << is_connected << endl;
			if (cycle || !is_connected)
				continue;
			else {
				//cout << "Gonna add " << e.first*c + e.second << endl << endl;
				A[e.first*c + e.second] = true;
				tmp = true;
			}
		}
		if (tmp == false)
			exhausted = true;
	}
}

void kruskal(){
	MPI_Init(NULL, NULL);
	int n, p;
	MPI_Comm_size(MPI_COMM_WORLD, &n);
	MPI_Comm_rank(MPI_COMM_WORLD, &p);

	int r = 64, c = 64, i, j;
	// the board is initially filled with walls
	bool *A = NULL;
	A = (bool*)malloc((r/n)*c * sizeof(bool));
	for (i = 0; i < (r/n)*c; i++) {
		A[i] = false;
	}

	// define end points for each processor
	int *startx = NULL, *starty = NULL, *endx = NULL, *endy = NULL;
	startx = (int*)malloc(n*sizeof(int));
	starty = (int*)malloc(n*sizeof(int));
	endx = (int*)malloc(n*sizeof(int));
	endy = (int*)malloc(n*sizeof(int));
	int* indices = NULL;
	
	if (p == 0) {
		indices = (int*)malloc(c*sizeof(int));
		for (i = 0; i < c; i++)
			indices[i] = i;
		startx[0] = 0;
		starty[0] =  c - 1;
		endx[n - 1] = r - 1;
		endy[n - 1] =  0;
		for (i = 0; i < n - 1; i++) {
			// generate random number from 0 to c - 1
			shuffle(indices, indices + c - 1, rng_kruskal);
			endx[i] = (i+1)*(r/n) - 1;
			endy[i] = indices[0];
		}

		for (i = 1; i < n; i++) {
			startx[i] = endx[i-1] + 1;
			starty[i] = endy[i-1];
		}
	}
	MPI_Bcast(startx, n, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(starty, n, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(endx, n, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(endy, n, MPI_INT, 0, MPI_COMM_WORLD);

	// generate maze for each chunk assigned to a processor
	if (p < n - 1) {
		f_kruskal(A, endx[p] - startx[p], c, {0, starty[p]}, {endx[p] - 1 - startx[p], endy[p]});
		// resolve the below lines?!
		/*for (i = 0; i < c; i++) 
			A[endx[p]*c + i] = false;*/
		//cout << p << " " << endx[p] << " " << endy[p] << endl;
		A[(endx[p] - startx[p])*c + endy[p]] = true;
	}
	else{
		f_kruskal(A, endx[p] - startx[p] + 1, c, {0, starty[p]}, {endx[p] - startx[p], endy[p]});
	}

	
	bool *B = NULL;
	if (p == 0) {
		B = (bool*)malloc(r*c * sizeof(bool));
	}

	MPI_Gather(A, (r/n)*c, MPI_C_BOOL, B, (r/n)*c, MPI_C_BOOL, 0, MPI_COMM_WORLD);

	if (p == 0) {
		format_kruskal(B, r, c, n);
		
		ofstream outFile("aryan_harihar_divyansh.txt");

        // Check if the file opened successfully
        if (!outFile.is_open()) {
            cerr << "Error opening output file." << endl;
            return;
        }
		for (i = 0; i < r; i++) {
			for (j = 0; j < c; j++) {
				outFile << B[i*c + j] << " ";
			}
			outFile << "\n";
		}

        // Close the output file stream
        outFile.close();

		free(B);
		free(indices);
	}


	free(A);
	free(startx);
	free(starty);
	free(endx);
	free(endy);
}