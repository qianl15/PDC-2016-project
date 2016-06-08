#ifndef CORE_WORKER_HPP_
#define CORE_WORKER_HPP_

#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>
#include <sstream>
#include <thread>
#include "../utils/communication.hpp"
#include "../utils/time.hpp"
#include "../utils/global.hpp"
#include "../utils/MessageBuffer.hpp"

using namespace std;

template<class MessageT>
class Worker {
	public:
		Worker() {
			worker_id = get_worker_id();
			messageBuffer.init();
			if (worker_id == MASTER_RANK) {
				cerr << "Coordinator initialised." << endl;
			} else {
				cerr << "Worker-" << worker_id << " Initialised." << endl;
			}
		}

		virtual void func(MessageBuffer<MessageT> &msg_buffer) = 0;

		/* run the worker */
		void run() {
			halt_count = 0;
			bool active = true;
			while (active) {
				func(messageBuffer);
				cerr << "!!!" << endl;
				messageBuffer.sync_messages();
				if (is_finish()) {
					active = false;
				}
				++halt_count;
			}
		}

	private:
		bool is_finish() {
			int all_halt_count = 0;
			all_halt_count = all_sum(halt_count);
			return all_halt_count == get_num_workers();
		}

	private:
		int worker_id;
		MessageBuffer<MessageT> messageBuffer;
		int halt_count;
};
#endif /* CORE_WORKER_H_ */
