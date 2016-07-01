#include <cstdio>
#include <vector>

#include "../../utils/serialization.hpp"

using namespace std;

#define N 1000
#define INIT_TEMP 99
#define STOP_TEMP 0.01
#define RATIO 0.999

class TSP {
	public:
		TSP(): contCnt(0) {
			for (int i = 0; i < n; ++i) {
				tour.push_back(i);
			}
			random_shuffle(tour.begin(), tour.end());
			preLen = curLen = getLength();
			halt = false;
		}

		float getLength() {
			if (n < 2) {
				return 0.0;
			}
			float ret = 0.0;
			for (int i = 1; i < n; ++i) {
				ret += dist[tour[i - 1]][tour[i]];
			}
			ret += dist[tour.back()][tour[0]];
			return ret;
		}

		void output() {
			printf("The shortest length is: %f.\n", curLen);
/*
			printf("The shortest length is: %f\nAnd the tour is:", curLen);
			for (int i = 0; i < (int)tour.size(); ++i) {
				printf(" %d", tour[i] + 1);
			}
			printf("\n");
*/
		}

		vector<int> tour;
		float curLen, preLen;
		int contCnt;
		bool halt;
		
		static int n;
		static float dist[N][N];
};

obinstream &operator<<(obinstream &bout, const TSP &tsp) {
	bout << tsp.tour;
	bout << tsp.curLen << tsp.preLen;
	bout << tsp.contCnt << tsp.halt;
	return bout;
}

ibinstream &operator>>(ibinstream &bin, TSP &tsp) {
	bin >> tsp.tour;
	bin >> tsp.curLen >> tsp.preLen;
	bin >> tsp.contCnt >> tsp.halt;
	return bin;
}
