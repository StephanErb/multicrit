#ifndef WRITE_BUFFER_H_
#define WRITE_BUFFER_H_

#include <vector>
#include "tbb/atomic.h"
#include "../options.hpp"

typedef tbb::atomic<size_t> AtomicCounter;


template<typename _value_type>
class ThreadLocalWriteBuffer {
public:
	typedef _value_type value_type;
	size_t current = 0;
	size_t end = 0;
private:
	value_type* const data;
	const value_type default_value;
	AtomicCounter& counter;

	inline void init_new_batch(const size_t start) {
		for (unsigned short i = 0; i < BATCH_SIZE; ++i) {
				data[start+i] = default_value;
		}
	}
public:
	ThreadLocalWriteBuffer(value_type* const _data, AtomicCounter& _counter, const value_type _default_value)
		: data(_data), default_value(_default_value), counter(_counter)
	{ }
	inline void reset() {
		current = end = 0;
	}
	template<typename ...Args>
	inline void emplace_back(Args&& ...args) {
		if (current == end) {
			current = counter.fetch_and_add(BATCH_SIZE);
			end = current + BATCH_SIZE;
			init_new_batch(current);
		}
		data[current++] = value_type(std::forward<Args>(args)...);
	}
};

#endif