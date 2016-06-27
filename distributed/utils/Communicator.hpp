#ifndef UTILS_COMMUNICATOR_HPP_
#define UTILS_COMMUNICATOR_HPP_

#include <iostream>
#include <mpi.h>
#include <vector>

#include "global.hpp"
#include "serialization.hpp"

template<class BufferT>
class Communicator {
	public:
		Communicator() {
			numPeers = getNumWorkers();
			me = getWorkerID();
			outBuffer.resize(numPeers);
			active = 1;
		}

		void voteToHalt() {
			active = 0;
		}

		bool isFinished() {
			int ret;
			MPI_Allreduce(&active, &ret, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
			return ret == 0;
		}

		void putMessage(const int dst, const BufferT &msg) {
			outBuffer[dst].push_back(msg);
		}

		std::vector<BufferT> &getMessage() {
			return inBuffer;
		}

		void clearInBuffer() {
			inBuffer.clear();
		}

		void syncBuffer() {
			clearInBuffer();
			allToAll(outBuffer);
			for (int i = 0; i < numPeers; ++i) {
				std::vector<BufferT> msgBuf = outBuffer[i];
				inBuffer.insert(inBuffer.end(), msgBuf.begin(), msgBuf.end());
			}
			for (int i = 0; i < numPeers; ++i) {
				outBuffer[i].clear();
			}
		}

		template<class MessageT>
			void send(int dst, const MessageT &msg) {
				obinstream bout;
				bout << msg;
				int sendCount = bout.size();
				MPI_Send(&sendCount, 1, MPI_INT, dst, 0, MPI_COMM_WORLD);
				char *sendBuffer = bout.getBuffer();
				MPI_Send(sendBuffer, sendCount, MPI_CHAR, dst, 0, MPI_COMM_WORLD);
			}

		template<class MessageT>
			void recv(int src, MessageT &msg) {
				MPI_Status status;
				int recvCount;
				MPI_Recv(&recvCount, 1, MPI_INT, src, 0, MPI_COMM_WORLD, &status);
				char *recvBuffer = new char[recvCount];
				MPI_Recv(recvBuffer, recvCount, MPI_CHAR, src, 0, MPI_COMM_WORLD, &status);
				ibinstream bin(recvBuffer, recvCount);
				bin >> msg;
			}

		template<class MessageT>
			void gatherMaster(std::vector<MessageT> &msgBuf) {
				int sendCount = 0;
				int *recvCount = new int[numPeers];
				int *recvOffset = new int[numPeers];

				/* Get the sizes of messages from each worker */
				MPI_Gather(&sendCount, 1, MPI_INT, recvCount, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);
				recvOffset[0] = 0;
				for (int i = 1; i < numPeers; ++i) {
					recvOffset[i] = recvOffset[i - 1] + recvCount[i - 1];
				}

				/* Get messages from each worker */
				int recvTotal = recvOffset[numPeers - 1] + recvCount[numPeers - 1];
				char *recvBuffer = new char[recvTotal];
				MPI_Gatherv(NULL, 0, MPI_CHAR, recvBuffer, recvCount, recvOffset, MPI_CHAR, MASTER_RANK, MPI_COMM_WORLD);

				/* Decode the messages */
				ibinstream bin(recvBuffer, recvTotal);
				for (int i = 0; i < numPeers; ++i) {
					if (i != me) {
						bin >> msgBuf[i];
					}
				}

				delete []recvCount;
				delete []recvOffset;
			}

		template<class MessageT>
			void gatherWorker(const MessageT &msg) {
				/* Encode the message */
				obinstream bout;
				bout << msg;
				int sendCount = bout.size();

				/* Send the size of the message to master */
				MPI_Gather(&sendCount, 1, MPI_INT, NULL, 0, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

				char *sendBuffer = bout.getBuffer();
				MPI_Gatherv(sendBuffer, sendCount, MPI_CHAR, NULL, NULL, NULL, MPI_CHAR, MASTER_RANK, MPI_COMM_WORLD);
			}

		template<class MessageT>
			void scatterMaster(std::vector<MessageT> &msgBuf) {
				int *sendCount= new int[numPeers];
				int *sendOffset = new int[numPeers];
				int recvCount;

				/* Encode the messages */
				obinstream bout;
				for (int i = 0; i < numPeers; ++i) {
					if (i != me) {
						int preSize = bout.size();
						bout << msgBuf[i];
						sendCount[i] = bout.size() - preSize;
					} else {
						sendCount[i] = 0;
					}
				}

				/* Send the sizes of messages to each worker */
				MPI_Scatter(sendCount, 1, MPI_INT, &recvCount, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);
				sendOffset[0] = 0;
				for (int i = 1; i < numPeers; ++i) {
					sendOffset[i] = sendOffset[i - 1] + sendCount[i - 1];
				}

				/* Sent messages to each worker */
				char *sendBuffer = bout.getBuffer();
				MPI_Scatterv(sendBuffer, sendCount, sendOffset, MPI_CHAR, NULL, 0, MPI_CHAR, MASTER_RANK, MPI_COMM_WORLD);

				delete []sendCount;
				delete []sendOffset;
			}

		template<class MessageT>
			void scatterWorker(MessageT &msg) {
				/* Get the size of the message from master */
				int recvCount;
				MPI_Scatter(NULL, 0, MPI_INT, &recvCount, 1, MPI_INT, MASTER_RANK, MPI_COMM_WORLD);

				/* Get the message from master */
				char *recvBuffer = new char[recvCount];
				MPI_Scatterv(NULL, NULL, NULL, MPI_CHAR, recvBuffer, recvCount, MPI_CHAR, MASTER_RANK, MPI_COMM_WORLD);

				/* Decode the message */
				ibinstream bin(recvBuffer, recvCount);
				bin >> msg;
			}

		template<class MessageT>
			void allToAll(std::vector<MessageT> &msgBuf) {
				for (int i = 0; i < numPeers; ++i) {
					int partner = (i - me + numPeers) % numPeers;
					if (me == partner) {
						continue;
					}
					if (me < partner) {
						send(partner, msgBuf[partner]);
						recv(partner, msgBuf[partner]);
					} else {
						auto tmpSend = msgBuf[partner];
						recv(partner, msgBuf[partner]);
						send(partner, tmpSend);
					}
				}
			}

	private:
		int numPeers;
		int me;
		int active;
		std::vector<BufferT> inBuffer;
		std::vector<std::vector<BufferT>> outBuffer;
};

#endif /* UTILS_COMMUNICATOR_HPP_ */
