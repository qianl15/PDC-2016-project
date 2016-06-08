#include <string>

#include "../../core/Worker.hpp"

using namespace std;

class SampleWorker: public Worker<int> {
	public:
	void func(MessageBuffer<int> &messageBuffer) {
		static int cnt = 0;
		int myID = get_worker_id();
		++cnt;
		if (cnt == 1) {
			cout << "I am Worker-" << myID << endl;
			for (int i = 1; i < get_num_workers(); ++i) {
				messageBuffer.add_message((myID + i) % get_num_workers(), myID);
			}
		} else {
			cout << "I get messages from:";
			for (auto i: messageBuffer.get_messages()) {
				cout << " " << i;
			}
			cout << endl;
		}
	}
};

int main() {
	init_workers();
	SampleWorker sampleWorker;
	sampleWorker.run();
	worker_finalize();
	return 0;
}
