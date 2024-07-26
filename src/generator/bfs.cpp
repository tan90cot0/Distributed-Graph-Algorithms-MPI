#include <mpi.h>
#include <iostream>
#include <random>
#include <fstream>

using namespace std;

// uncomment below for unseeded
auto rd_bfs = std::random_device {}; 
auto rng_bfs = default_random_engine {rd_bfs()};

int dx_bfs[4] = {-1, 0, 1, 0};
int dy_bfs[4] = {0, 1, 0, -1};

// check if (x, y) is inside_bfs r x c
bool inside_bfs(int x, int y, int r, int c) {
	return ((x >= 0) && (x < r) && (y >= 0) && (y < c));
}

// iteratively look for back edges in the tree space
// if we see a vertex not the parent
// then there is a cycle in the graph
void itr_bfs (bool A[], int x, int y, int px, int py, int r, int c, vector<bool> &vis, bool &detect) {
	if (vis[x*c + y])
		return;
	vis[x*c + y] = true;
	for (int d = 0; d < 4; d++){
		int nx = x + dx_bfs[d], ny = y + dy_bfs[d];
		if (inside_bfs(nx, ny, r, c) && A[nx*c + ny]) {
			if (vis[nx*c + ny] && (nx!=px || ny != py)) {
				//cout << "Prev " << nx << " " << ny << " with cur " << x << " " << y << endl;
				detect = true;
			}
			else
				itr_bfs(A, nx, ny, x, y, r, c, vis, detect);
		}
	}
}

// check if A has any cycles on adding (x, y) as a passable cell
bool no_cycle_bfs (bool A[], int x, int y, int r, int c) {
	bool detect = false;
	vector<bool> vis(r*c, false);
	itr_bfs(A, x, y, -1, -1, r, c, vis, detect);
	return !detect;
}

void format_bfs(bool A[], int r, int c, int n) {
	for (int i = r/n - 1; i < r - 1; i+= r/n) {
		for (int j = 0; j < c; j++) {
			//cout << i << " " << j << " " << no_cycle_bfs(A, i, j, r, c) << endl;
			if (!A[i*c + j] && !A[(i+1)*c + j]) {
				bool connected = false;
				for (int t = 0; t < 4; t++) {
					int nx = i + dx_bfs[t], ny = j + dy_bfs[t];
					if (inside_bfs(nx, ny, r, c) && A[nx*c + ny]) 
						connected = true;
				}
				if (connected && no_cycle_bfs(A, i, j, r, c))
					A[i*c + j] = true;
			}
		}
	}
}

// sample path uniformly at random
// (dy_bfs - ey) horizontal steps
// c vertical steps
void sample_path_bfs (bool A[], int sy, int ey, int r, int c, int dir) {
	vector<char> shifts;
	int i;
	for (i = 0; i < abs(sy - ey); i++) {
		shifts.push_back('H');
	}
	for (i = 0; i < r - 1; i++){
		shifts.push_back('V');
	}
	// shuffle the sequence
	shuffle(shifts.begin(), shifts.end(), rng_bfs);
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
void f_bfs(bool A[], int r, int c, pair<int, int> start, pair<int, int> end) {
	int sx = start.first, sy = start.second, ex = end.first, ey = end.second;
	int i, j;
	A[sx*c+sy] = true;
	if (sy >= ey)
		sample_path_bfs(A, sy, ey, r, c, -1);
	else
		sample_path_bfs(A, sy, ey, r, c, +1);
	
	// BFS from points on path to dead ends
	queue<pair<int, int>> frontier;
	vector<pair<int, int>> passable;
	vector<bool> vis(r*c, false);
	// compute passable points
	for (i = 0; i < r; i++) {
		for (j = 0; j < c; j++) {
			if (A[i*c + j])
				passable.push_back({i, j});
		}
	}
	// randomize the BFS orders
	shuffle(passable.begin(), passable.end(), rng_bfs);
	for (auto p : passable)
		frontier.push(p);
	while (!frontier.empty()) {
		auto u = frontier.front();
		frontier.pop();
		int ux = u.first, uy = u.second;
		if (vis[ux*c + uy])
			continue;
		// mark u as visited
		vis[ux*c + uy] = true;
		//cout << "Cur Vertex : " << ux << " " << uy << endl << endl;

		// iterate over neighbours of u
		for (int d = 0; d < 4; d++) {
			int vx = ux + dx_bfs[d], vy = uy + dy_bfs[d];
			if (inside_bfs(vx, vy, r, c) && !vis[vx*c + vy]) {
				//cout << "Neighbour " << vx << " " << vy << endl;
				// check if (vx, vy) can be added without making a cycle
				if (no_cycle_bfs(A, vx, vy, r, c)) {
					//cout << "No cycle with " << vx << " " << vy << endl;
					A[vx*c + vy] = true;
					frontier.push({vx, vy});
				}
			}
		}
	}
}

void bfs(){
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
			shuffle(indices, indices + c - 1, rng_bfs);
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
		f_bfs(A, endx[p] - startx[p], c, {0, starty[p]}, {endx[p] - 1 - startx[p], endy[p]});
		// resolve the below lines?!
		/*for (i = 0; i < c; i++) 
			A[endx[p]*c + i] = false;*/
		//cout << p << " " << endx[p] << " " << endy[p] << endl;
		A[(endx[p] - startx[p])*c + endy[p]] = true;
	}
	else{
		f_bfs(A, endx[p] - startx[p] + 1, c, {0, starty[p]}, {endx[p] - startx[p], endy[p]});
	}

	
	bool *B = NULL;
	if (p == 0) {
		B = (bool*)malloc(r*c * sizeof(bool));
	}

	MPI_Gather(A, (r/n)*c, MPI_C_BOOL, B, (r/n)*c, MPI_C_BOOL, 0, MPI_COMM_WORLD);

	if (p == 0) {
		format_bfs(B, r, c, n);
		ofstream outFile("aryan_harihar_divyansh.txt");

        // Check if the file opened successfully
        if (!outFile.is_open()) {
            cerr << "Error opening output file." << endl;
            return;
        }

        // Write the content to the file instead of printing it
        for (int i = 0; i < r; i++) {
            for (int j = 0; j < c; j++) {
                outFile << B[i * c + j] << " ";
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
