#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cstring>
#include <vector>
#include <algorithm>

#include "../../core/Worker.hpp"

using namespace std;

const int N = 1000;
const int MAX_SEED = 200;
const int RELAX = 4000;
const int MAX_LAST = 3;
const float INIT_TEMP = 99.0;
const float STOP_TEMP = 0.001;
const float RATIO = 0.999;
const float EPS = 1E-5;

class TSP {
	public:
		TSP() {
		}

		TSP(int _n): n(_n) {
			for (int i = 0; i < n; ++i) {
				tour.push_back(i);
			}
			random_shuffle(tour.begin(), tour.end());
		}

	private:
		float tourLen(const vector<int> &tour) {
			if (tour.size() != n) {
				fprintf(stderr, "The tour has wrong size!!!\n");
				exit(1);
			}
			float ret = 0.0;
			for (int i = 1; i < n; ++i) {
				ret += dist[tour[i - 1]][tour[i]];
			}
			ret += dist[tour.back()][tour[0]];
			return ret;
		}

	public:
		float solve() {
			double curLen = tourLen(tour), preLen = curLen;
			float temperature = INIT_TEMP;
			int contCnt = 0; // the continuous same length times
			while (temperature > STOP_TEMP) {
				temperature *= RATIO;
				/* stay in the same temperature for RELAX times */
				for (int i = 0; i < RELAX; ++i) {
					/* Proposal 1: Block Reverse between p and q */
					int p = rand() % n, q = rand() % n;
					// If will occur error if p=0 q=N-1...
					if (abs(p - q) == n - 1) {
						q = rand() % (n - 1);
						p = rand() % (n - 2);
					}
					if (p == q) {
						q = (q + 2) % n;
					}
					if (p > q) {
						swap(p, q);
					}
					int p1 = (p - 1 + n) % n, q1 = (q + 1) % n;
					int tp = tour[p], tq = tour[q], tp1 = tour[p1], tq1 = tour[q1];
					float delta = dist[tp][tq1] + dist[tp1][tq] - dist[tp][tp1] - dist[tq][tq1];
					/* whether to accept the change */
					if ((delta < 0) || ((delta > 0) && (exp(-delta / temperature) > (float)rand() / RAND_MAX))) {
						curLen = curLen + delta;
						int mid = (q - p) >> 1;
						int tmp;
						for (int k = 0; k <= mid; ++k) {
							swap(tour[p + k], tour[q - k]);
						}

						if (curLen != tourLen(tour)) {
							fprintf(stderr, "p q p1 q1 is: %d %d %d %d\n", p, q, p1, q1);
							fprintf(stderr, "wrong! delta %f, %f vs. %f\n", delta, tourLen(tour), curLen);
							exit(1);
						}
					}
				}

				if (fabs(curLen - preLen) < EPS) {
					++contCnt;
					if (contCnt == MAX_LAST) {
						break;
					}
				} else {
					contCnt = 0;
				}
				preLen = curLen;
			}

			return ret = curLen;
		}

		void output() {
			printf("The shortest length is: %f\nAnd the tour is:", ret);
			for (int i = 0; i < n; ++i) {
				printf(" %d", tour[i] + 1);
			}
			printf("\n");
		}

	public:
		static float dist[N][N];
		vector<int> tour;
		int n;
		float ret;
}; 
float TSP::dist[N][N];
int n;

/* load the data */
void loadFile(char* filename) {
	FILE *pf = fopen(filename, "r");
	if (pf == NULL) {
		printf("Cannot open the file!\n");
		exit(1);
	}
	char buff[200];
	fscanf(pf, "NAME: %[^\n]s", buff);
	fscanf(pf, "\nTYPE: TSP%[^\n]s", buff);
	fscanf(pf, "\nCOMMENT: %[^\n]s", buff);
	fscanf(pf, "\nDIMENSION: %d", &n);
	printf("The N is: %d\n", n);
	fscanf(pf, "\nEDGE_WEIGHT_TYPE: %[^\n]s", buff);
	printf("the type is: %s\n", buff);
	memset(TSP::dist, 0, sizeof(TSP::dist));
	if (strcmp(buff, "EUC_2D") == 0) {
		fscanf(pf, "\nNODE_COORD_SECTION");
		float nodeCoord[N][2] = {};
		int nid;
		float xx, yy;
		for (int i = 0; i < N; ++i) {
			fscanf(pf, "\n%d %f %f", &nid, &xx, &yy);
			nodeCoord[i][0] = xx;
			nodeCoord[i][1] = yy;
		}
		float xi, yi, xj, yj;
		for (int i = 0; i < N; ++i) {
			for (int j = i + 1; j < N; ++j) {
				xi = nodeCoord[i][0];
				yi = nodeCoord[i][1];
				xj = nodeCoord[j][0];
				yj = nodeCoord[j][1];
				TSP::dist[i][j] = (float)sqrt((xi - xj) * (xi - xj) + (yi - yj) * (yi - yj));
				TSP::dist[j][i] = TSP::dist[i][j];
			}
		}
	}
	else if (strcmp(buff, "EXPLICIT") == 0) {
		fscanf(pf, "\nEDGE_WEIGHT_FORMAT: %[^\n]s", buff);
		fscanf(pf, "\nEDGE_WEIGHT_SECTION");
		float weight;
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j <= i; ++j) {
				fscanf(pf, "%f", &weight);
				TSP::dist[i][j] = weight;
				TSP::dist[j][i] = weight;
			}
		}
	}
}

class TSPWorker: public Worker<int> {
	public:
	void func(MessageBuffer<int> &messageBuffer) {
		for (int i = 0; i < MAX_SEED; ++i) {
			seeds[i] = TSP(n);
			seeds[i].solve();
		}
		int k = 0;
		for (int i = 1; i < MAX_SEED; ++i) {
			if (seeds[i].ret < seeds[k].ret) {
				k = i;
			}
		}
		seeds[k].output();
	}

	TSP seeds[MAX_SEED];
};

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Usage: ./%s input_filename.\n", argv[0]);
		exit(1);
	}
	loadFile(argv[1]);

	cerr << "Finish loading." << endl;

	srand(time(NULL));

	TSPWorker worker;
	worker.run();

	return 0;
}
