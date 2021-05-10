#pragma once

#include "Math/myassert.h"

template<typename T>
class Pool {
public:
	~Pool() {
		Deallocate();
	}

	void Allocate(unsigned amount) {
		Deallocate();

		// Allocate (We allocate in the same block to mantain locality)
		size_t allocatedSize = amount * sizeof(T*);
		size_t dataSize = amount * sizeof(T);
		size_t locationSize = amount * sizeof(size_t);

		char* allocatedBlock = (char*) ::operator new(allocatedSize + dataSize + locationSize);
		allocated = (T**) allocatedBlock;
		data = (T*) (allocatedBlock + allocatedSize);
		location = (size_t*) (allocatedBlock + allocatedSize + dataSize);

		// Initialize
		capacity = amount;
		count = 0;
		for (size_t i = 0; i < amount; ++i) {
			allocated[i] = data + i;
			location[i] = i;
		}
	}

	void Deallocate() {
		capacity = 0;
		count = 0;
		if (allocated != nullptr) {
			::operator delete(allocated);
			allocated = nullptr;
			data = nullptr;
			location = nullptr;
		}
	}

	template<typename... Args>
	T* Obtain(Args&&... args) {
		assert(data != nullptr);			   // ERROR: The pool hasn't been initialized
		if (count == capacity) return nullptr; // Pool overflow

		// Obtain a new object
		return new (allocated[count++]) T(std::forward<Args>(args)...);
	}

	void Release(T* object) {
		assert(object >= data && object < data + capacity); // ERROR: The object is not in the data array

		size_t index = object - data;
		size_t allocatedIndex = location[index];

		assert(allocatedIndex < count); // ERROR: The object is already free

		// Release the object
		object->~T();

		size_t lastIndex = count - 1;
		T* lastAllocated = allocated[lastIndex];
		size_t lastAllocatedIndex = lastAllocated - data;

		location[index] = location[lastAllocatedIndex];
		location[lastAllocatedIndex] = allocatedIndex;

		allocated[lastIndex] = allocated[allocatedIndex];
		allocated[allocatedIndex] = lastAllocated;

		count -= 1;
	}

	void Clear() {
		// Reset count
		count = 0;

		// Initialize lists
		for (size_t i = 0; i < capacity; ++i) {
			allocated[i] = data + i;
			location[i] = i;
		}
	}

	size_t Capacity() {
		return capacity;
	}

	size_t Count() {
		return count;
	}

	// Iteration

	class Iterator {
	public:
		Iterator(T** item__)
			: item(item__) {}

		const Iterator& operator++() {
			++item;
			return *this;
		}

		bool operator!=(const Iterator& other) const {
			return item != other.item;
		}

		T& operator*() const {
			return **item;
		}

	private:
		T** item;
	};

	class ConstIterator {
	public:
		ConstIterator(const T** item__)
			: item(item__) {}

		const ConstIterator& operator++() {
			++item;
			return *this;
		}

		bool operator!=(const ConstIterator& other) const {
			return item != other.item;
		}

		const T& operator*() const {
			return **item;
		}

	private:
		const T** item;
	};

	Iterator begin() {
		return Iterator(allocated);
	}

	ConstIterator begin() const {
		return ConstIterator((const T**) allocated);
	}

	Iterator end() {
		return Iterator(allocated + count);
	}

	ConstIterator end() const {
		return ConstIterator((const T**) (allocated + count));
	}

private:
	size_t capacity = 0;		// Max number of objects in the pool.
	size_t count = 0;			// Current number of objects in the pool.
	T* data = nullptr;			// Data storage.
	T** allocated = nullptr;	// Pointers to the allocated objects. This list is only used up to the starting 'count' positions.
	size_t* location = nullptr; // Indexes to the position of the object index in the 'allocated' array.
};
