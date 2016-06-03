#ifndef UTILS_MESSAGEBUFFER_HPP_
#define UTILS_MESSAGEBUFFER_HPP_

#include <stddef.h>
#include <unordered_map>
#include <vector>

#include "global.hpp"

using namespace std;

int cnt = 0;

template<class MessageT>
class MessageBuffer {
	public:
		vector<vector<MessageT>> out_messages;
		vector<MessageT> in_messages;
		int tag;

		void init(int num_peers = get_num_workers()) {
			tag = cnt;
			for (int i = 0; i < num_peers; i++) {
				vector<MessageT> msgBuf;
				out_messages.push_back(msgBuf);
			}
		}

		void add_message(const int dest, const MessageT& msg) {
			out_messages[dest].push_back(msg);
		}

		vector<MessageT>& get_messages() {
			return in_messages;
		}

		void reset_in_messages(){
			in_messages.clear();
		}

		void sync_messages() {
			int num_worker = get_num_workers();
			int me = get_worker_id();

			all_to_all(out_messages, tag);

			// gather all messages
			for (int i = 0; i < num_worker; i++) {
				vector<MessageT>& msgBuf = out_messages[i];
				in_messages.insert(in_messages.end(), msgBuf.begin(), msgBuf.end());
			}

			for (int i = 0; i < num_worker; ++i) {
				out_messages[i].clear();
			}
		}
};
#endif /* UTILS_MESSAGEBUFFER_HPP_ */
