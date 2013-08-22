/*
 * BucketQueue.hpp
 *
 *  Created on: May 30, 2011
 *      Author: kobitzsch
 */

#ifndef BUCKETQUEUE_HPP_
#define BUCKETQUEUE_HPP_

#include <limits>
#include <vector>
#include "../../exception.h"
#include "QueueStorages.hpp"

namespace utility{
namespace datastructure{

template< typename id_slot, typename storage_slot = ArrayStorage< id_slot > >
class BucketQueue{
protected:
	unsigned int num_buckets;
	storage_slot * bucket_storages;

	unsigned int min_bucket;

	unsigned int count;
public:
	BucketQueue( unsigned int num_buckets_ ) :
		num_buckets( num_buckets_ ),
		bucket_storages( new storage_slot[num_buckets_] ),
		min_bucket( std::numeric_limits<unsigned int>::max() ),	//no bucket set
		count( 0 )
	{
		//std::cout << "[status] Created bucket queue with " << num_buckets_ << " buckets." << std::endl;
	}

	~BucketQueue(){
		if( bucket_storages ) delete[] bucket_storages, bucket_storages = NULL;
	}

	//push an element to the bucket queue into specified bucket
	void push( unsigned int bucket, id_slot element ){
		bucket %= num_buckets;
		bucket_storages[bucket].push_back( element );
		if( count == 0 )
			min_bucket = bucket;
		++count;
	}

	//get the minimal element from the bucket queue (first element in lowest bucket)
	const id_slot & top() const {
		GUARANTEE( count, std::runtime_error, "BucketQueue::Top::Accessing empty queue")
		return bucket_storages[min_bucket].front();
	}

	bool empty() const {
		return count == 0;
	}

	void clear() {
		for( size_t i = 0; i < num_buckets; ++i )
			bucket_storages[i].clear();
		count = 0;
		min_bucket = 0;
		//while( !empty() ) pop();	//not as sexual as it looks
	}

	unsigned int size() const { return count; }

	//delete the minimal element from the bucket queue
	void pop(){
		GUARANTEE( count, std::runtime_error, "BucketQueue::Top::Accessing empty queue")
		bucket_storages[min_bucket].pop_front();
		--count;
		if( count ){
			while( bucket_storages[min_bucket].empty() )
				min_bucket = (min_bucket + 1) % num_buckets;
		}
	}

	const storage_slot & getStorage( unsigned int bucket ) const {
		bucket %= num_buckets;
		return bucket_storages[bucket];
	}
};

}
}

#endif /* BUCKETQUEUE_HPP_ */

