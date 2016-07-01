mpic++ sa_worker.cpp -o sa_worker -std=c++11 -O2
mpic++ sa_coordinator.cpp -o sa_coordinator -std=c++11 -O2
input=("../../../dataset/ch150.tsp" "../../../dataset/gr17.tsp" "../../../dataset/fri26.tsp" "../../../dataset/dantzig42.tsp")
for file in ${input[@]}
do
	echo $file
	for ((n = 1; n <= 64; n *= 4))
	do
		for ((k = 16; k <= 256; k *= 2))
		do
			echo $n $k
			m=$((k / n))
			if [ $k -ge $n ]
			then
				mpirun -np 1 ./sa_coordinator : -np $n ./sa_worker $file $m
			fi
		done
	done
done
