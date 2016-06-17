CC=g++
FILE1=omp_SA_TSP.cpp
FLAGS1=-fopenmp -O2 -Wall
OUT1= -o omp_out

FILE2=pthread_SA_TSP.cpp
FLAGS2=-O2 -lpthread -Wall
OUT2= -o pthread_out

all:
	$(CC) $(FILE1) $(FLAGS1) $(OUT1)
	$(CC) $(FILE2) $(FLAGS2) $(OUT2)
