#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cstring>
#include <vector>
#include <algorithm>

#include "sa.hpp"
#include "../../utils/global.hpp"
#include "../../utils/Communicator.hpp"

using namespace std;

const int MAX_SEED = 500;
const int RELAX = 40000;
const int MAX_LAST = 3;
const float EPS = 1E-5;

float TSP::dist[N][N];
int TSP::n;

vector<TSP> seeds;

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
//	printf("%s\n", buff);
	fscanf(pf, "\nTYPE: TSP%[^\n]s", buff);
//	printf("%s\n", buff);
	fscanf(pf, "\nCOMMENT: %[^\n]s", buff);
//	printf("%s\n", buff);
	fscanf(pf, "\nDIMENSION: %d", &TSP::n);
//	printf("The N is: %d\n", TSP::n);
	fscanf(pf, "\nEDGE_WEIGHT_TYPE: %[^\n]s", buff);
//	printf("the type is: %s\n", buff);
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
		fscanf(pf, "\n%[^\n]s", buff);
		char *disps = strstr(buff, "DISPLAY_DATA_TYPE");
		if (disps != NULL) {
			fscanf(pf, "\nEDGE_WEIGHT_SECTION");
		}
		float weight;
		for (int i = 0; i < TSP::n; ++i) {
			for (int j = 0; j <= i; ++j) {
				fscanf(pf, "%f", &weight);
				TSP::dist[i][j] = weight;
				TSP::dist[j][i] = weight;
			}
		}
	}
	return;
}

bool solve(TSP &seed, float temperature) {
	/* stay in the same temperature for RELAX times */
	for (int i = 0; i < RELAX; ++i) {
		/* Proposal 1: Block Reverse between p and q */
		int p = rand() % TSP::n, q = rand() % TSP::n;
		// If will occur error if p=0 q=N-1...
		if (abs(p - q) == TSP::n - 1) {
			q = rand() % (TSP::n - 1);
			p = rand() % (TSP::n - 2);
		}
		if (p == q) {
			q = (q + 2) % TSP::n;
		}
		if (p > q) {
			swap(p, q);
		}
		int p1 = (p - 1 + TSP::n) % TSP::n, q1 = (q + 1) % TSP::n;
		int tp = seed.tour[p], tq = seed.tour[q], tp1 = seed.tour[p1], tq1 = seed.tour[q1];
		float delta = TSP::dist[tp][tq1] + TSP::dist[tp1][tq] - TSP::dist[tp][tp1] - TSP::dist[tq][tq1];
		/* whether to accept the change */
		if ((delta < 0) || ((delta > 0) && (exp(-delta / temperature) > (float)rand() / RAND_MAX))) {
			seed.curLen = seed.curLen + delta;
			int mid = (q - p) >> 1;
			int tmp;
			for (int k = 0; k <= mid; ++k) {
				swap(seed.tour[p + k], seed.tour[q - k]);
			}
/*
			if (seed.curLen != seed.getLength()) {
				fprintf(stderr, "p q p1 q1 is: %d %d %d %d\n", p, q, p1, q1);
				fprintf(stderr, "wrong! delta %f, %f vs. %f\n", delta, seed.getLength(), seed.curLen);
				exit(1);
			}
*/
		}
	}

	if (fabs(seed.curLen - seed.preLen) < EPS) {
		++seed.contCnt;
		if (seed.contCnt == MAX_LAST) {
			return false;
		}
	} else {
		seed.contCnt = 0;
	}
	seed.preLen = seed.curLen;
	return true;
}

int main(int argc, char *argv[]) {
	init();
	if (argc < 2) {
		fprintf(stderr, "Usage: %s input_filename.\n", argv[0]);
		exit(1);
	}
	loadFile(argv[1]);
	barrier();

	srand(time(NULL) + getWorkerID());
	Communicator<TSP> communicator;
	seeds.resize(MAX_SEED);

	vector<TSP> tmpSeeds, terminated;
	vector<pair<int, int>> target;
	float temperature = INIT_TEMP;
	while (!communicator.isFinished()) {
		if (temperature <= STOP_TEMP || seeds.empty()) {
			communicator.voteToHalt();
		}
		temperature *= RATIO;
		for (int i = 0; i < (int)seeds.size(); ++i) {
			if (solve(seeds[i], temperature)) {
				tmpSeeds.push_back(seeds[i]);
			} else {
				terminated.push_back(seeds[i]);
			}
		}
		seeds = tmpSeeds;
		tmpSeeds.clear();
		communicator.gatherWorker((int)seeds.size());
		communicator.scatterWorker(target);
		for (auto &p: target) {
			int dst = p.first;
			int cnt = p.second;
			while (cnt--) {
				communicator.putMessage(dst, seeds.back());
				seeds.pop_back();
			}
		}
		communicator.syncBuffer();
		for (auto &item: communicator.getMessage()) {
			seeds.push_back(item);
		}
		cerr << getWorkerID() << ' ' << seeds.size() << endl;
	}

	barrier();

	int k = 0;
	for (int i = 1; i < (int)terminated.size(); ++i) {
		if (terminated[i].curLen < terminated[k].curLen) {
			k = i;
		}
	}
	communicator.gatherWorker(terminated[k]);

	finalize();
	return 0;
}
