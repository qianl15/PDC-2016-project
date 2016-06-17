#ifndef UTILS_GLOBAL_HPP_
#define UTILS_GLOBAL_HPP_

#include <mpi.h>

#define MASTER_RANK 0

int myRank;
int numWorkers;

inline int getWorkerID() {
	return myRank;
}

inline int getNumWorkers() {
	return numWorkers;
}

void init() {
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &numWorkers);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
}

void finalize() {
	MPI_Finalize();
}

void barrier() {
	MPI_Barrier(MPI_COMM_WORLD);
}

#endif /* UTILS_GLOBAL_HPP_ */
