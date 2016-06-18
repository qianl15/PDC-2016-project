/*
	Genetic algorithm for Traveling Salesman Problem
	@@ baseline version: no parallel optimization, single thread
	
	Input: xxx.tsp file
	Output: optimal value (total distance)
			& solution route: permutation of {1, 2, ..., N}
*/

#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
#include <sys/time.h>

using namespace std;

const int N = 1000;
const int MAX_SEED = 1000;
const int MAX_ITER = 10000;
const float PMATE = 0.95;
const float PMUTATE = 0.9;
const int REMAIN = MAX_SEED / 100;
const int BESTS = 3;

float dist[N][N] = {};	// The distance matrix, use (i-1) instead of i
int n;

class DNA {
	public:
	vector<int> a;
	int n;
	double len;

	DNA() {
	}

	DNA(int _n): n(_n) {
		a.resize(n);
		for (int i = 0; i < n; ++i) {
			a[i] = i;
		}
		random_shuffle(a.begin(), a.end());
		for (int i = 0; i < n - 1; ++i) {
			int k = i + 1;
			for (int j = i + 2; j < n; ++j) {
				if (dist[a[i]][a[j]] < dist[a[i]][a[k]]) {
					k = j;
				}
			}
			swap(a[i + 1], a[k]);
		}
		calcLen();
	}

	DNA(const vector<int> &_a): a(_a) {
		n = a.size();
		calcLen();
	}

	void calcLen() {
		len = 0.0;
		for (int i = 0; i < n; ++i) {
			len += dist[a[i]][a[(i + 1) % n]];
		}
	}

	bool operator<(const DNA &that) const {
		return len < that.len;
	}

	void output() {
		printf("The shortest length is: %f.\nAnd the tour is:", len);
		for (int i = 0; i < n; ++i) {
			printf(" %d", a[i]+1);
		}
		printf("\n");
	}
} seeds[MAX_SEED];

/* load the data */
void loadFile(char* filename) {
	FILE *pf;

	pf = fopen(filename, "r");
	if (pf == NULL) {
		printf("Cannot open the file!\n");
		exit(1);
	}
	char buff[200];
	fscanf(pf, "NAME: %[^\n]s", buff);
	printf("%s\n", buff);
	fscanf(pf, "\nTYPE: TSP%[^\n]s", buff);
	printf("%s\n", buff);
	fscanf(pf, "\nCOMMENT: %[^\n]s", buff);
	printf("%s\n", buff);
	fscanf(pf, "\nDIMENSION: %d", &n);
	printf("The N is: %d\n", n);
	fscanf(pf, "\nEDGE_WEIGHT_TYPE: %[^\n]s", buff);
	printf("the type is: %s\n", buff);
	memset(dist, 0, sizeof(dist));
	if (strcmp(buff, "EUC_2D") == 0) {
		fscanf(pf, "\nNODE_COORD_SECTION");
		float nodeCoord[N][2] = {};
		int nid;
		float xx, yy;
		for (int i = 0; i < n; ++i) {
			fscanf(pf, "\n%d %f %f", &nid, &xx, &yy);
			nodeCoord[i][0] = xx;
			nodeCoord[i][1] = yy;
		}
		float xi, yi, xj, yj;
		for (int i = 0; i < n; ++i) {
			for (int j = i + 1; j < n; ++j) {
				xi = nodeCoord[i][0];
				yi = nodeCoord[i][1];
				xj = nodeCoord[j][0];
				yj = nodeCoord[j][1];
				dist[i][j] = (float)sqrt((xi - xj) * (xi - xj) + (yi - yj) * (yi - yj));
				dist[j][i] = dist[i][j];
			}
		}
	}
	else if (strcmp(buff, "EXPLICIT") == 0) {
		fscanf(pf, "\nEDGE_WEIGHT_FORMAT: %[^\n]s", buff);
		fscanf(pf, "\n%[^\n]s", buff);
		char *disps = strstr(buff, "DISPLAY_DATA_TYPE");
		if (disps != NULL) {
			fscanf(pf, "\nEDGE_WEIGHT_SECTION");
		}
		float weight;
		for (int i = 0; i < n; ++i) {
			for (int j = 0; j <= i; ++j) {
				fscanf(pf, "%f", &weight);
				dist[i][j] = weight;
				dist[j][i] = weight;
			}
		}
	}
}

double newRand() {
	return rand() % (int)1E9 / 1E9;
}

int mateChoose(DNA seeds[]) {
	float maxLen = seeds[REMAIN - 1].len;
	float tot = 0.0;
	for (int i = 0; i < REMAIN; ++i) {
		tot += maxLen / seeds[i].len;
	}
	tot *= newRand();
	int ret;
	for (int i = 0; i < REMAIN; ++i) {
		tot -= maxLen / seeds[i].len;
		if (tot <= 0) {
			ret = i;
			break;
		}
	}
	return ret;
}

DNA mate(const DNA &a, const DNA &b) {
	static int prevA[N], nextA[N], prevB[N], nextB[N];
	vector<int> ret(n);
	for (int i = 0; i < n; ++i) {
		nextA[a.a[i]] = a.a[(i + 1) % n];
		prevA[a.a[(i + 1) % n]] = a.a[i];
		nextB[b.a[i]] = b.a[(i + 1) % n];
		prevB[b.a[(i + 1) % n]] = b.a[i];
	}
	ret[0] = rand() % n;
	for (int i = 0; i < n - 1; ++i) {
		int k = ret[i];
		if (dist[k][nextA[k]] < dist[k][nextB[k]]) {
			ret[i + 1] = nextA[k];
		} else {
			ret[i + 1] = nextB[k];
		}
		nextA[prevA[k]] = nextA[k];
		prevA[nextA[k]] = prevA[k];
		nextB[prevB[k]] = nextB[k];
		prevB[nextB[k]] = prevB[k];
	}
	return DNA(ret);
}

void mutate(DNA &a) {
	int l = rand() % n, r = rand() % n;
	if (abs(l - r) == n - 1) {
		r = rand() % (n - 1);
		l = rand() % (n - 2);
	}
	if (l == r) {
		r = (r + 2) % n;
	}
	if (l > r) {
		swap(l, r);
	}
	++r;
	reverse(a.a.begin() + l, a.a.begin() + r);
	a.calcLen();
}

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Please enter the filename!\n");
		return 0;
	}
	else {
		loadFile(argv[1]);
	}
	srand(time(NULL));
	struct timeval start, stop;
	gettimeofday(&start, NULL);

	for (int i = 0; i < MAX_SEED; ++i) {
		seeds[i] = DNA(n);
	}
	sort(seeds, seeds + MAX_SEED);
	for (int t = 0; t < MAX_ITER; ++t) {
		for (int i = REMAIN; i < MAX_SEED; ++i) {
			double pMate = newRand();
			if (pMate > PMATE) {
				continue;
			}
			int p = mateChoose(seeds), q = mateChoose(seeds);
			if (p == q) {
				seeds[i] = seeds[p];
			} else {
				seeds[i] = mate(seeds[p], seeds[q]);
			}
		}
		for (int i = BESTS; i < MAX_SEED; ++i) {
			double pMutate = newRand();
			if (pMutate <= PMUTATE) {
				mutate(seeds[i]);
			}
		}
		sort(seeds, seeds + MAX_SEED);
		if (t % 100 == 0) {
			cerr << t << ": " << seeds[0].len << endl;
		}
	}

	gettimeofday(&stop, NULL);
	// ------------- Print the result! -----------------
	int tottime = stop.tv_sec - start.tv_sec;
	int timemin = tottime / 60;
	int timesec = tottime % 60;
	printf("Total time usage: %d min %d sec. \n", timemin, timesec);

	seeds[0].output();

	return 0;
}
