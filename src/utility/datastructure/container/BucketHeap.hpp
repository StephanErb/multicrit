/*
 * MinBucketQueue.h
 *
 *  Created on: 17.02.2012
 *      Author: kobitzsch
 */

#ifndef BUCKET_HEAP_HPP_
#define BUCKET_HEAP_HPP_

#include "QueueStorages.hpp"
#include "../NullData.hpp"
#include "../../exception.h"

#include <algorithm>
#include <vector>

namespace utility {
namespace datastructure {

template< typename id_slot >
struct BucketElement{
	BucketElement( const id_slot & id_ = (id_slot)(-1) )
	: id( id_ )
	{}

	id_slot id;	//index into the data slot
};

template< typename id_slot >
struct HeapElement{
	HeapElement( const id_slot & id_ = (id_slot)(-1) )
	: id( id_ )
	{}

	id_slot id;
};

template< typename id_slot, typename key_slot, typename user_data_slot  >
struct DataElement : public user_data_slot {	//using base class optimization
	key_slot key;	//replicates key for
	size_t heap_index;
	size_t bucket;
	size_t reached_index;
};

template< typename id_slot,									//ID-Type stored in queue
		  typename key_slot,								//Key-Value for Min-Heap part Part
		  typename meta_key_slot,							//Meta Key of Min Heap Part
		  typename bucket_slot,								//Key-Value for Bucket Part
		  typename user_data_slot = NullData,				//Associated User Data
		  typename storage_slot = DataArrayStorage< id_slot, DataElement<id_slot,key_slot,user_data_slot> > >	//Storage for Data (Defaults to basic array, hashed versions for space efficiency)
class BucketHeap {
protected:
	unsigned int min_bucket;							//the id of the currently minimal bucket
	unsigned int bucket_count;
	std::vector< BucketElement<id_slot> > * buckets;	//array of buckets in form of vectors

	storage_slot storage;								//storage holding node data and localization data
	std::vector< HeapElement<id_slot> > heap;			//the binary heap
	std::vector< id_slot > reached;

public:
	BucketHeap( size_t number_of_buckets, size_t node_count, size_t bucket_reserve_size = 50 ) :
		min_bucket(0),
		bucket_count( number_of_buckets ),
		buckets( NULL ),
		storage( (id_slot)node_count+1 )
	{
		std::cout << "[status] number of buckets " << bucket_count << std::endl;
		GUARANTEE( number_of_buckets > 0, std::runtime_error, "[error] Tried to create bucket queue without buckets.")
		buckets = new std::vector< BucketElement<id_slot> >[number_of_buckets];
		for( size_t bucket = 0; bucket != number_of_buckets; ++bucket ){
			buckets[bucket].reserve(bucket_reserve_size);
		}

		//set the sentinel element
		storage[(id_slot)node_count].heap_index = 0;
		storage[(id_slot)node_count].key = meta_key_slot::min();

		//reserve heap for faster insertion
		heap.reserve( std::min( number_of_buckets * bucket_reserve_size, node_count + 1 ) );
		reached.reserve( std::min( number_of_buckets * bucket_reserve_size, node_count + 1 ) );

		heap.push_back( HeapElement<id_slot>( (id_slot) node_count ) );
	}

	~BucketHeap(){
		if( buckets ){
			delete[] buckets;
			buckets = NULL;
		}
	}

	/////////////////////////////////////////////////////////
	// ID/Key access
	/////////////////////////////////////////////////////////

	//returns the min key in respect to the min heap ordering
	const key_slot & getMinKey() const {
		GUARANTEE( heap.size() > 1, std::runtime_error, "[error] accessing an empty heap" )
		return storage[ heap[1].id ].key;
	}

	//returns the minimal element in respect to the min heap ordering
	const id_slot & getMin() const {
		GUARANTEE( heap.size() > 1, std::runtime_error, "[error] accessing an empty heap" )
		return heap[1].id;
	}

	//returns the frontmost element in the bucket queue
	const id_slot & front(){
		while( buckets[min_bucket].empty() ){
			min_bucket = (min_bucket + 1) % bucket_count;
		}
		return (buckets[min_bucket].back()).id;
	}

	//returns the key of an arbitary element
	const key_slot & getKey( const id_slot & id ) const {
		if( !isReached(id) ){
			std::cout << "bla" << std::endl;
		}
		GUARANTEE( isReached( id ), std::runtime_error, "[error] requesting invalid key" )
		return storage[ id ].key;
	}

	//returns the user data associated with the element
	const user_data_slot & getUserData( const id_slot & id ) const {
		return storage[id];
	}

	user_data_slot & getUserData( const id_slot & id ){
		return storage[id];
	}

	/////////////////////////////////////////////////////////
	// Insertion/Deletion/Modification
	/////////////////////////////////////////////////////////

	//pushes an element into the structure
	void push( const id_slot & id, const key_slot & key, const bucket_slot & bucket ){
		bucket_slot target_bucket = bucket % bucket_count;
		storage[id].key = key;
		storage[id].bucket = bucket;
		storage[id].reached_index = reached.size();

		buckets[target_bucket].push_back( BucketElement<id_slot>(id) );

		reached.push_back( id );

		heap.push_back( HeapElement<id_slot>(id) );
		upheap( heap.size()-1 );
	}

	//deletes the next element in regard to the bucket ordering
	int pop(){
		while( buckets[min_bucket].empty() ) min_bucket = (min_bucket + 1) % bucket_count;
		id_slot to_delete = (buckets[min_bucket].back()).id;
		buckets[min_bucket].pop_back();
		std::swap( heap[storage[to_delete].heap_index], heap[heap.size()-1] );
		heap.pop_back();
		downheap( storage[to_delete].heap_index );
		return min_bucket;
	}

	//deletes the minimal element in regard to the binary heap ordering
	//runs linear in the number of elements in the same bucket
	void deleteMin(){
		id_slot to_delete = heap[1].id;
		std::swap( heap[1], heap[heap.size()-1] );
		heap.pop_back();
		downheap( 1 );
		size_t & bucket = storage[to_delete].bucket;
		storage[to_delete].heap_index = 0;	//mark deleted
		for( size_t i = 0, end = buckets[bucket].size(); i != end; ++i ){
			if( buckets[bucket][i].id == to_delete ){
				std::swap( buckets[bucket][i], buckets[bucket][end-1] );
				buckets[bucket].pop_back();
				break;
			}
		}
	}

	//Decrease the binary heap key of an element
	void decreaseKey( const id_slot & id, const key_slot & new_key ){
		GUARANTEE( contains( id ), std::runtime_error, "[error] decreasing key of element not in heap")
		GUARANTEE( storage[id].key > new_key, std::runtime_error, "[error] decrease key to larger or equal value called")

		storage[id].key = new_key;
		upheap( storage[id].heap_index );
	}

	//deletes an arbitrary element from the queue
	//runs linear in the number of elements in the same bucket
	inline void deleteNode( const id_slot & id ){
		GUARANTEE( contains( id ), std::runtime_error, "[error] deleting element not contained")
		std::swap( heap[storage[id].heap_index], heap[heap.size()-1] );
		heap.pop_back();
		downheap( heap[storage[id].heap_index] );
		size_t & bucket = storage[id].bucket;
		for( size_t i = 0, end = buckets[bucket].size(); i != end; ++i ){
			if( buckets[bucket][i] == id ){
				std::swap( buckets[bucket][i], buckets[bucket][end-1] );
				buckets[bucket].pop_back();
				break;
			}
		}
	}

	/////////////////////////////////////////////////////////
	// Heap Status / Global view
	/////////////////////////////////////////////////////////

	//returns whether an element is currently contained within the heap
	bool contains( const id_slot & id ) const {
		return ( (storage[id].heap_index < heap.size()) && (heap[storage[id].heap_index].id == id) );
	}


	//returns whether an element was inserted into the structure since the last clear
	inline bool isReached( const id_slot & id ) const {
		return ((storage[id].reached_index < reached.size()) && (reached[ storage[id].reached_index ] == id));
	}

	//clear the heap for another query
	void clear(){
		reached.clear();
		heap.resize( 1 );	//remove everything but the sentinel
		for( size_t i = 0; i < bucket_count; ++i ){
			buckets[i].clear();
		}
	}

	size_t size() const {
		return reached.size();
	}

	bool empty() const {
		return (heap.size() == 1);
	}


protected:
	//shifts an element upwards in the heap
	inline void upheap( size_t heap_position ){
		register size_t position_on_heap = heap_position;
		while( storage[ heap[position_on_heap].id ].key < storage[ heap[position_on_heap>>1].id ].key ){
			std::swap( heap[position_on_heap].id, heap[position_on_heap>>1].id );
			storage[ heap[ position_on_heap ].id ].heap_index = position_on_heap;
			heap_position >>=1;
		}
		storage[ heap[ position_on_heap ].id ].heap_index = position_on_heap;
	}

	//shifts an element downwards in the heap
	inline void downheap( size_t heap_position ){
		register size_t next_position;
		while( ((heap_position << 1) | 1 ) < heap.size() &&
			   (next_position = (heap_position << 1) | (storage[heap[(heap_position << 1)].id].key > storage[heap[(heap_position << 1) | 1].id].key))
			   && storage[heap[next_position].id].key < storage[heap[heap_position].id].key )
		{
			std::swap( heap[heap_position], heap[next_position] );
			storage[heap[heap_position].id].heap_index = heap_position;
			heap_position = next_position;
		}
		next_position = heap_position << 1;
		if( next_position < heap.size() && storage[heap[next_position].id].key < storage[heap[heap_position].id].key ){
			std::swap( heap[heap_position], heap[next_position] );
			storage[heap[heap_position].id].heap_index = heap_position;
			heap_position = next_position;
		}
		storage[ heap[ heap_position ].id ].heap_index = heap_position;
	}

	//checks the integrity of the heap structure (DEBUG function)
	bool checkIntegrity(){
		//TODO implement
		return false;
	}
};

} /* namespace datastructure */
} /* namespace utility */

#endif /* BUCKET_HEAP_HPP_ */
