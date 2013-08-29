#ifndef WRITE_BUFFER_H_
#define WRITE_BUFFER_H_

#include "tbb/atomic.h"
#include "../options.hpp"

typedef tbb::atomic<size_t> AtomicCounter;

template<typename value_type>
class ThreadLocalWriteBuffer {
private:
	value_type* const shared_data;
	const value_type default_value;
	
	AtomicCounter& shared_counter;
	size_t current = 0;
	size_t end = 0;

	inline void init_new_bucket(const size_t start) {
		for (unsigned short i = 0; i < BATCH_SIZE; ++i) {
				shared_data[start+i] = default_value;
		}
	}

public:
	ThreadLocalWriteBuffer(value_type* const _data, AtomicCounter& _counter, const value_type _default_value)
		: shared_data(_data), default_value(_default_value), shared_counter(_counter)
	{ }

	inline size_t reset() {
		const size_t unused_buffer_spaced = end - current;
		current = end = 0;
		return unused_buffer_spaced;
	}
	
	template<typename ...Args>
	inline void emplace_back(Args&& ...args) {
		if (current == end) {
			current = shared_counter.fetch_and_add(BATCH_SIZE);
			end = current + BATCH_SIZE;
			init_new_bucket(current);
		}
		shared_data[current++] = value_type(std::forward<Args>(args)...);
	}
};

#endif