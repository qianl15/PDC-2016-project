#ifndef UTILS_SERIALIZATION_HPP_
#define UTILS_SERIALIZATION_HPP_

#include <vector>
#include <string>

#include "global.hpp"

class obinstream {
	private:
		std::vector<char> buf;

	public:
		char *getBuffer() {
			return &buf[0];
		}

		size_t size() {
			return buf.size();
		}

		void rawByte(char c) {
			buf.push_back(c);
		}

		void rawBytes(const void *ptr, int size) {
			buf.insert(buf.end(), (const char *)ptr, (const char *)ptr + size);
		}
};

obinstream &operator<<(obinstream &bout, bool i) {
	bout.rawBytes(&i, sizeof(bool));
	return bout;
}

obinstream &operator<<(obinstream &bin, char c) {
	bin.rawByte(c);
	return bin;
}

obinstream &operator<<(obinstream &bout, int i) {
	bout.rawBytes(&i, sizeof(int));
	return bout;
}

obinstream &operator<<(obinstream &bout, size_t i) {
	bout.rawBytes(&i, sizeof(size_t));
	return bout;
}

obinstream &operator<<(obinstream &bout, float i) {
	bout.rawBytes(&i, sizeof(float));
	return bout;
}

obinstream &operator<<(obinstream &bout, double i) {
	bout.rawBytes(&i, sizeof(double));
	return bout;
}

template<class T1, class T2>
obinstream &operator<<(obinstream &bout, const std::pair<T1, T2> &p) {
	bout << p.first;
	bout << p.second;
	return bout;
}

template<class T>
obinstream &operator<<(obinstream &bout, const std::vector<T> &v) {
	bout << v.size();
	for (auto &e: v) {
		bout << e;
	}
	return bout;
}

obinstream &operator<<(obinstream &bout, const std::vector<int> &v) {
	bout << v.size();
	bout.rawBytes(&v[0], v.size() * sizeof(int));
	return bout;
}

obinstream &operator<<(obinstream &bout, const std::vector<float> &v) {
	bout << v.size();
	bout.rawBytes(&v[0], v.size() * sizeof(float));
	return bout;
}

class ibinstream {
	private:
		char *buf; //responsible for deleting the buffer, do not delete outside
		size_t size;
		size_t idx;

	public:
		ibinstream(char *_buf, size_t _size): buf(_buf), size(_size), idx(0) {
		}

		ibinstream(char *_buf, size_t _size, size_t _idx): buf(_buf), size(_size), idx(_idx) {
		}

		~ibinstream() {
			delete []buf;
		}

		char rawByte() {
			return buf[idx++];
		}

		void *rawBytes(unsigned int n) {
			char *ret = buf + idx;
			idx += n;
			return ret;
		}
};

ibinstream &operator>>(ibinstream &bin, bool &i) {
	i = *(bool *)bin.rawBytes(sizeof(bool));
	return bin;
}

ibinstream &operator>>(ibinstream &bin, char &c) {
	c = bin.rawByte();
	return bin;
}

ibinstream &operator>>(ibinstream &bin, int &i) {
	i = *(int *)bin.rawBytes(sizeof(int));
	return bin;
}

ibinstream &operator>>(ibinstream &bin, size_t &i) {
	i = *(size_t *)bin.rawBytes(sizeof(size_t));
	return bin;
}

ibinstream &operator>>(ibinstream &bin, float &i) {
	i = *(float *)bin.rawBytes(sizeof(float));
	return bin;
}

ibinstream &operator>>(ibinstream &bin, double &i) {
	i = *(double *)bin.rawBytes(sizeof(double));
	return bin;
}

template<class T1, class T2>
ibinstream &operator>>(ibinstream &bin, std::pair<T1, T2> &p) {
	bin >> p.first;
	bin >> p.second;
	return bin;
}

template<class T>
ibinstream &operator>>(ibinstream &bin, std::vector<T> &v) {
	size_t size;
	bin >> size;
	v.resize(size);
	for (typename std::vector<T>::iterator it = v.begin(); it != v.end(); ++it) {
		bin >> *it;
	}
	return bin;
}

ibinstream &operator>>(ibinstream &bin, std::vector<int> &v) {
	size_t size;
	bin >> size;
	v.resize(size);
	int *data = (int *)bin.rawBytes(sizeof(int) * size);
	v.assign(data, data + size);
	return bin;
}

ibinstream &operator>>(ibinstream &bin, std::vector<float> &v) {
	size_t size;
	bin >> size;
	v.resize(size);
	float *data = (float *)bin.rawBytes(sizeof(float) * size);
	v.assign(data, data + size);
	return bin;
}

#endif /* UTILS_SERIALIZATION_HPP_ */
