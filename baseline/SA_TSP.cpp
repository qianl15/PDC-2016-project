/*
   Simulated Annealing algorithm for Traveling Salesman Problem
   @@ baseline version: no parallel optimization, single thread

Input: xxx.tsp file
Output: optimal value (total distance)
& solution route: permutation of {1, 2, ..., N}
 */

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <sys/time.h>
#define MAXITER 20		// Proposal 20 routes and then select the best one
#define THRESH1 0.1		// Threshold 1 for the strategy
#define THRESH2 0.89	// Threshold 2 for the strategy
#define RELAX 40000		// The times of relaxation of the same temperature
#define ALPHA 0.999		// Cooling rate
#define INITEMP 99.0	// Initial temperature
#define STOPTEMP 0.001	// Termination temperature
#define MAXLAST 3		// Stop if the tour length keeps unchanged for MAXLAST consecutive temperature
#define MAXN 1000		// only support N <= 1000
using namespace std;

float minTourDist = -1;		// The distance of shortest path
int *minTour = NULL;		// The shortest path
int N = 0;					// Number of cities
float dist[MAXN][MAXN] = {};	// The distance matrix, use (i-1) instead of i

class rand_x { 
	unsigned int seed;
	public:
	rand_x(int init) : seed(init) {}

	int operator()(int limit) {
		int divisor = RAND_MAX/(limit+1);
		int retval;

		do { 
			retval = rand_r(&seed) / divisor;
		} while (retval > limit);

		return retval;
	}        
};

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
	fscanf(pf, "\nDIMENSION: %d", &N);
	printf("The N is: %d\n", N);
	fscanf(pf, "\nEDGE_WEIGHT_TYPE: %[^\n]s", buff);
	printf("the type is: %s\n", buff);
	memset(dist, 0, sizeof(dist));
	if (strcmp(buff, "EUC_2D") == 0) {
		fscanf(pf, "\nNODE_COORD_SECTION");
		float nodeCoord[MAXN][2] = {};
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
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j <= i; ++j) {
				fscanf(pf, "%f", &weight);
				dist[i][j] = weight;
				dist[j][i] = weight;
			}
		}
	}
	return;
}

/* Calculate the length of the tour */
float tourLen(int *tour) {
	if (tour == NULL) {
		printf("tour not exist!\n");
		return -1;
	}
	float cnt = 0;
	for (int i = 0; i < N - 1; ++i) {
		cnt += dist[tour[i]][tour[i+1]];
	}
	cnt += dist[tour[N-1]][tour[0]];
	return cnt;
}

/* the main simulated annealing function */
void saTSP(int* tour) {
	float currLen = tourLen(tour);
	float temperature = INITEMP;
	float lastLen = currLen;
	int contCnt = 0; // the continuous same length times
	while (temperature > STOPTEMP) {
		temperature *= ALPHA;
		/* stay in the same temperature for RELAX times */
		unsigned int s = time(0);
		s = s + random();
		for (int i = 0; i < RELAX; ++i) {
			/* Proposal 1: Block Reverse between p and q */
			int p = rand_r(&s)%N, q = rand_r(&s)%N;
			// If will occur error if p=0 q=N-1...
			if (abs(p - q) == N-1) {
				q = rand_r(&s)%(N-1);
				p = rand_r(&s)%(N-2);
			}
			if (p == q) {
				q = (q + 2) % N;
			}
			if (p > q) {
				int tmp = p;
				p = q;
				q = tmp;
			}
			int p1 = (p - 1 + N) % N;
			int q1 = (q + 1) % N;
			int tp = tour[p], tq = tour[q], tp1 = tour[p1], tq1 = tour[q1];
			float delta = dist[tp][tq1] + dist[tp1][tq] - dist[tp][tp1] - dist[tq][tq1];

			/* whether to accept the change */
			if ((delta < 0) || ((delta > 0) && 
						(exp(-delta/temperature) > (float)rand_r(&s)/RAND_MAX))) {
				currLen = currLen + delta;
				int mid = (q - p) >> 1;
				int tmp;
				for (int k = 0; k <= mid; ++k) {
					tmp = tour[p+k];
					tour[p+k] = tour[q-k];
					tour[q-k] = tmp;
				}
				//currLen = tourLen(tour);
			}

		}

		if (fabs(currLen - lastLen) < 1e-3) {
			contCnt += 1;
			if (contCnt >= MAXLAST) {
				//printf("unchanged for %d times1!\n", contCnt);
				break;
			}
		}
		else
			contCnt = 0;
		lastLen = currLen;
	}
	return;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Please enter the filename!\n");
		return 0;
	}
	else {
		loadFile(argv[1]);
	}
	struct timeval start, stop;
	gettimeofday(&start, NULL);
	srandom(time(0));
	minTour = (int *)malloc(sizeof(int) * N);
	int *currTour = (int *)malloc(sizeof(int) * N);
	unsigned int s = time(0);
	for (int i = 0; i < MAXITER; ++i) {
		for (int j = 0; j < N; ++j)
			currTour[j] = j;
		rand_x rg(s);
		random_shuffle(currTour, currTour + N, rg);
		saTSP(currTour);
		float currLen = tourLen(currTour);
		//printf("currLen is: %f\n", currLen);
		if ((minTourDist < 0) ||(currLen < minTourDist)) {
			minTourDist = currLen;
			for (int j = 0; j < N; ++j) {
				minTour[j] = currTour[j];
			}
		}
	}

	gettimeofday(&stop, NULL);
	// ------------- Print the result! -----------------
	int tottime = stop.tv_sec - start.tv_sec;
	int timemin = tottime / 60;
	int timesec = tottime % 60;
	printf("Total time usage: %d min %d sec. \n", timemin, timesec);
	printf("The shortest length is: %f\n And the tour is: \n", minTourDist);
	for (int i = 0; i < N; ++i) {
		printf("%d \n", minTour[i]+1);
	}
	free(minTour);
	free(currTour);
	return 0;
}
