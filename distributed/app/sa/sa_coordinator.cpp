#include <iostream>
#include <vector>
#include <algorithm>
#include <sys/time.h>

#include "sa.hpp"
#include "../../utils/global.hpp"
#include "../../utils/Communicator.hpp"

using namespace std;

int TSP::n;
float TSP::dist[N][N];

int main() {
	init();
	int n = getNumWorkers();
	Communicator<TSP> communicator;
	communicator.voteToHalt();

	barrier();

	struct timeval start, stop;
	gettimeofday(&start, NULL);

	vector<int> seedCount(n);
	vector<pair<int, int>> origin(n);
	vector<int> target(n);
	vector<vector<pair<int, int>>> arrange(n);
	float temperature = INIT_TEMP;
	while (!communicator.isFinished()) {
		temperature *= RATIO;
		if (temperature > 10) {
			continue;
		}

		communicator.gatherMaster(seedCount);

//		cerr << temperature << endl;

		int sum = 0;
		for (int i = 1; i < n; ++i) {
			sum += seedCount[i];
			origin[i] = make_pair(-seedCount[i], i);
		}

		sort(origin.begin() + 1, origin.end());
		int average = sum / (n - 1);
		for (int i = 1; i < n; ++i) {
			target[i] = average;
		}
		for (int i = 1; i < sum % (n - 1) + 1; ++i) {
			++target[i];
		}
		int j;
		for (j = n - 1; j > 0 && -origin[j].first == target[origin[j].second]; --j);
		for (int i = 1; i < n; ++i) {
			int u = origin[i].second;
			while (-origin[i].first > target[u]) {
				int v = origin[j].second;
				int delta = min(-origin[i].first - target[u], target[v] - (-origin[j].first));
				arrange[u].push_back(make_pair(v, delta));
				origin[i].first += delta;
				origin[j].first -= delta;
				while (j > 0 && -origin[j].first == target[origin[j].second]) {
					--j;
				}
			}
		}
		communicator.scatterMaster(arrange);
		for (int i = 1; i < n; ++i) {
			arrange[i].clear();
		}
		communicator.syncBuffer();
	}

	barrier();

	gettimeofday(&stop, NULL);
	double totTime = (stop.tv_sec - start.tv_sec) * 1000.0 + (stop.tv_usec - start.tv_usec) / 1000.0;
	printf("Total time used: %.3fms.\n", totTime);

	vector<TSP> results(n);
	communicator.gatherMaster(results);
	int k = 1;
	for (int i = 1; i < (int)results.size(); ++i) {
		if (results[i].curLen < results[k].curLen) {
			k = i;
		}
	}
	results[k].output();

	finalize();
	return 0;
}
