/*
	Simulated Annealing algorithm for Traveling Salesman Problem
	@@ CUDA version: no parallel optimization, single thread
	
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
#include <pthread.h>
#include <curand_kernel.h>
#define MAXITER 20		// Proposal 20 routes and then select the best one
#define THRESH1 0.1		// Threshold 1 for the strategy
#define THRESH2 0.89	// Threshold 2 for the strategy
#define RELAX 400		// The times of relaxation of the same temperature
#define ALPHA 0.999		// Cooling rate
#define INITEMP 99.0	// Initial temperature
#define STOPTEMP 0.001	// Termination temperature
#define MAXLAST 3		// Stop if the tour length keeps unchanged for MAXLAST consecutive temperature
#define MAXN 250		// only support N <= 250
#define THREADITER 200
using namespace std;

float minTourDist = -1;		// The distance of shortest path
int *minTour = NULL;		// The shortest path
int N = 0;					// Number of cities
float *dist = NULL;	// The distance matrix, use (i-1) instead of i

int *currTour = NULL;
int blockNum = 1;		// block number
int threadNum = 1;	// thread number
int globalIter = -1;	// global iteration count
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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
	dist = (float *)malloc(sizeof(float) * N * N);
	memset(dist, 0, sizeof(float) * N * N);
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
				dist[i*N + j] = (float)sqrt((xi - xj) * (xi - xj) + (yi - yj) * (yi - yj));
				dist[j*N + i] = dist[i*N + j];
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
				dist[i*N + j] = weight;
				dist[j*N + i] = weight;
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
		cnt += dist[tour[i]*N + tour[i+1]];
	}
	cnt += dist[tour[N-1]*N + tour[0]];
	return cnt;
}

/* the main simulated annealing function */
__global__ void saTSP(int cityCnt, int* globalTour, curandState *randStates,  float *dev_dist, float temperature, int relaxiter) {
	int thid = (blockIdx.x * blockDim.x) + threadIdx.x;
	int *tour = &globalTour[thid * cityCnt];
	float currLen = 0;
	for (int i = 0; i < cityCnt - 1; ++i) {
		currLen += dev_dist[tour[i]*cityCnt + tour[i+1]];
	}
	currLen += dev_dist[tour[cityCnt-1]*cityCnt + tour[0]];
	//float temperature = INITEMP;
	//float lastLen = currLen;
	//int contCnt = 0; // the continuous same length times
	int iterCnt = 0;
	while (temperature > STOPTEMP) {
		temperature *= ALPHA;
		iterCnt += 1;
		/* stay in the same temperature for RELAX times */
		for (int i = 0; i < relaxiter; ++i) {
			/* Proposal 1: Block Reverse between p and q */
			int p = (int)(curand_uniform(&(randStates[thid])) * (float)(cityCnt + 10)) % cityCnt;
			int q = (int)(curand_uniform(&(randStates[thid])) * (float)(cityCnt + 10)) % cityCnt;
			// If will occur error if p=0 q=N-1...
			if (abs(p - q) == cityCnt - 1) {
				p = (int)(curand_uniform(&(randStates[thid])) * (float)(cityCnt - 3));
				q = (int)(curand_uniform(&(randStates[thid])) * (float)(cityCnt - 2));
			}
			if (p == q) {
				q = (q + 2) % cityCnt;
			}
			if (p > q) {
				int tmp = p;
				p = q;
				q = tmp;
			}
			int p1 = (p - 1 + cityCnt) % cityCnt;
			int q1 = (q + 1) % cityCnt;
			int tp = tour[p], tq = tour[q], tp1 = tour[p1], tq1 = tour[q1];
			float delta = dev_dist[tp*cityCnt + tq1] + dev_dist[tp1*cityCnt + tq] - dev_dist[tp*cityCnt + tp1] - dev_dist[tq*cityCnt + tq1];

			/* whether to accept the change */
			if ((delta < 0) || ((delta > 0) && 
				(expf(-delta/temperature) > curand_uniform(&(randStates[thid]))))) {
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
	/*
		if ((currLen - lastLen < 1e-2) && (currLen - lastLen > -1e-2)) {
			contCnt += 1;
			if (contCnt >= MAXLAST) {
				//printf("unchanged for %d times1!\n", contCnt);
				break;
			}
		}
		else
			contCnt = 0;
		lastLen = currLen;
	*/
	}
	
	return;
}

__global__ void setup_kernel_randomness(curandState * state, unsigned long seed)
{
	int s_id = (blockIdx.x*blockDim.x) + threadIdx.x;
	curand_init(seed*s_id, s_id, 0, &state[s_id]);
}

int main(int argc, char **argv) {
	cudaError_t err = cudaSuccess;
	float *dev_dist;
	
	if (argc < 2) {
		printf("Usage: ./cuda_tsp <filename> <blockNum> <threadNum>\n");
		return 0;
	}
	else {
		loadFile(argv[1]);
		err = cudaMalloc((void **)&dev_dist, sizeof(float) * N * N);
		if (err != cudaSuccess) {
			fprintf(stderr, "cudaMalloc() failed\n");
			exit(1);
		}

		cudaMemcpy((void *)dev_dist, dist, sizeof(float) * N * N, cudaMemcpyHostToDevice);
	}
	if (argc == 4) {
		blockNum = atoi(argv[2]);
		threadNum = atoi(argv[3]);
	}
	printf("blockNum is: %d, threadNum is: %d\n", blockNum, threadNum);
	struct timeval start, stop;
	gettimeofday(&start, NULL);
	srandom(time(0));
	int *dev_currTour; // currTour on device;
	int itersCnt = blockNum * threadNum; // total iterations
	err = cudaMalloc((void **)&dev_currTour, sizeof(int)*N*itersCnt);
	if (err != cudaSuccess) {
		fprintf(stderr, "cudaMalloc() failed\n");
		exit(1);
	}

	srand(time(0));
	currTour = (int *)malloc(sizeof(int) * N * itersCnt);
	for (int i = 0; i < itersCnt; ++i) {
		for (int j = 0; j < N; ++j) {
			currTour[i*N + j] = j;
		}
		random_shuffle(currTour+i*N, currTour+(i+1)*N);
		/*for (int j = 0; j < N; ++j) {
			printf("%d ", currTour[i*N + j]);
		}
		printf("%d before: %f\n", i, tourLen(currTour + i*N));*/
	}
	err = cudaMemcpy(dev_currTour, currTour, itersCnt * N * sizeof(int), cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		fprintf(stderr, "cudaMalloc() for dev_currTour failed\n");
		exit(1);
	}

	// allocate random seed for each thread
	curandState *devStates;
	cudaMalloc((void **)&devStates, itersCnt * sizeof(curandState));	
	setup_kernel_randomness<<<blockNum, threadNum>>>(devStates, time(0));
	cudaDeviceSynchronize();

	float currLen = 0;
	
	float temperature = INITEMP;
	int contCnt = 0;
	float tempstep = pow(ALPHA, THREADITER);
	//while (temperature > STOPTEMP) {
		//printf("%.06f \n", temperature);
		saTSP<<<blockNum, threadNum>>>(N, dev_currTour, devStates, dev_dist, temperature, RELAX);
		cudaDeviceSynchronize();	
	//	temperature *= tempstep;
	//}

	minTour = (int *)malloc(sizeof(int) * N);
	memset(currTour, 0, itersCnt * N * sizeof(int));
	err = cudaMemcpy(currTour, dev_currTour, itersCnt * N * sizeof(int), cudaMemcpyDeviceToHost);
	if (err != cudaSuccess) {
		fprintf(stderr, "cudaMemcpyc(Device to Host) failed with %d\n", err);
		exit(1);
	}

	/* find the minimal answer */
	int minidx = 0;
	for (int i = 0; i < itersCnt; ++i) {
		currLen = tourLen(&currTour[i * N]);
		/*for (int j = 0; j < N; ++j) {
			printf("%d ", currTour[i*N + j]);
		}

		printf("%d after: %f\n", i, currLen);*/
		if ((currLen < minTourDist) || (minTourDist < 0)) {
			minTourDist = currLen;
			minidx = i;
		}
	}
	for (int i = 0; i < N; ++i) {
		minTour[i] = currTour[minidx * N + i];
	}
	gettimeofday(&stop, NULL);

	// ------------- Print the result! -----------------
	int tottime = stop.tv_sec - start.tv_sec;
	int timemin = tottime / 60;
	int timesec = tottime % 60;
	printf("Total time usage: %d min %d sec. \n", timemin, timesec);
	printf("N is %d, The shortest length is: %f\n And the tour is: \n", N, minTourDist);
	for (int i = 0; i < N; ++i) {
		printf("%d \n", minTour[i]+1);
	}
	free(dist);
	free(minTour);
	free(currTour);
	
	return 0;
}
