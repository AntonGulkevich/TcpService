#pragma once
#include <mutex>
#include <array>
#include <iterator>

template <typename T, size_t max_size_in_bytes>
class CircleBuffer
{
private:
	using ArrayIterator = typename std::array<T, max_size_in_bytes>::iterator;

	std::mutex _ioMutex;
	std::array<T, max_size_in_bytes> _buffer;
	ArrayIterator _tail;
	UINT _currentCycle;
	//todo: fast and furious chunk adding
	// Slow Adding 
	void Add(const T & data)
	{
		*_tail = data;
		if (++_tail == _buffer.end())
		{
			++_currentCycle;
			_tail = _buffer.begin();
		}
	}

public:
	CircleBuffer() : _currentCycle(NULL) { _tail = _buffer.begin(); }
	virtual ~CircleBuffer() {}

	auto GetCycle() const { return _currentCycle; }
	auto GetTail() const { return _tail; }
	auto Begin() const { return _buffer.begin(); }
	auto AdvanceIterator(const ArrayIterator& iterator, const UINT distance)
	{
		auto retIt = iterator;
		for (UINT i = 0; i < distance; ++i)
		{
			if (++retIt == _buffer.end())
				retIt = _buffer.begin();
		}
		return retIt;
	}
	auto Add(const T * data, size_t count)
	{
		std::lock_guard<std::mutex> lock(_ioMutex);
		for (size_t i = 0; i < count; ++i)
			Add(data[i]);
		return _tail;
	}
	auto Write(T* data, size_t maxSize, UINT &realCount, ArrayIterator offset = _buffer.begin())
	{
		std::lock_guard<std::mutex> lock(_ioMutex);

		// If maxSize is more than max size of the buffer need to limit it
		maxSize = maxSize > max_size_in_bytes ? max_size_in_bytes : maxSize;
		UINT avaliableCount = 0;

		// Fixed undefined behaviour
		if (offset == _tail)
		{
			realCount = 0;
			return _tail;
		}

		if (offset < _tail)
		{
			// Calculate count of elements avaliable for read
			avaliableCount = std::distance(offset, _tail);
			// If max count is less than avaliable
			if (avaliableCount > maxSize)
			{
				realCount = maxSize;
				auto toIt = AdvanceIterator(offset, realCount);
				std::copy(offset, toIt, stdext::checked_array_iterator<T*>(data, max_size_in_bytes));
				return toIt;
			}
			else
			{
				realCount = avaliableCount;
				std::copy(offset, _tail, stdext::checked_array_iterator<T*>(data, max_size_in_bytes));
				return _tail;
			}
		}
		else
		{
			auto distanceFromOffsetToBack = std::distance(offset, _buffer.end());
			auto distanceFromBeginToTail = std::distance(_buffer.begin(), _tail);
			avaliableCount = distanceFromOffsetToBack + distanceFromBeginToTail;

			// If max count is less than avaliable
			if (avaliableCount > maxSize)
			{
				realCount = maxSize;
				auto toIt = AdvanceIterator(offset, realCount);
				if (offset > toIt)
				{
					std::copy(offset, _buffer.end(), stdext::checked_array_iterator<T*>(data, max_size_in_bytes));
					std::copy(_buffer.begin(), toIt, stdext::checked_array_iterator<T*>(data + std::distance(offset, _buffer.end()), max_size_in_bytes));
				}
				else
				{
					std::copy(offset, toIt, stdext::checked_array_iterator<T*>(data, max_size_in_bytes));
				}
				return toIt;
			}
			else
			{
				// If length of output array is enought to copy all avaliable words
				realCount = avaliableCount;
				std::copy(offset, _buffer.end(), stdext::checked_array_iterator<T*>(data, max_size_in_bytes));
				std::copy(_buffer.begin(), _tail, stdext::checked_array_iterator<T*>(data + std::distance(offset, _buffer.end()), max_size_in_bytes));
				return _tail;
			}
		}
	}
	static auto GetMaximumSize() { return max_size_in_bytes; };
};
