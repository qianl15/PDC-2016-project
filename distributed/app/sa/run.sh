mpic++ sa_worker.cpp -o sa_worker -std=c++11 -O2
mpic++ sa_coordinator.cpp -o sa_coordinator -std=c++11 -O2
mpirun -np 1 ./sa_coordinator : -np $1 ./sa_worker $2 $3
