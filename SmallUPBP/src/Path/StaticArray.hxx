/*
 * Copyright (C) 2014, Petr Vevoda, Martin Sik (http://cgg.mff.cuni.cz/~sik/), 
 * Tomas Davidovic (http://www.davidovic.cz), Iliyan Georgiev (http://www.iliyan.com/), 
 * Jaroslav Krivanek (http://cgg.mff.cuni.cz/~jaroslav/)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * (The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
 */

#ifndef __STATICARRAY_HXX__
#define __STATICARRAY_HXX__

#include <algorithm> // For sorting

#include "..\Misc\Defs.hxx"

#define DEBUGARRAY

#ifdef DEBUGARRAY
#include <iostream>
#define ASSERT(expr) \
do { if (!(expr)) { std::cerr << "Error: assertion `"#expr"' failed at " << __FILE__ << ":" << __LINE__ << std::endl; exit(2); } } while (0)
#else
#define ASSERT(expr)
#endif

template< typename T, size_t N = 20 >
class StaticArray
{
public:

	INLINE StaticArray() :
		mSize(0), mStart(mElements)
	{
	}

	/// Size of the array
	INLINE size_t size() const
	{
		return mSize;
	}

	/// Is array empty?
	INLINE bool empty() const
	{
		return mSize == 0;
	}

	/// Add element
	INLINE void push_back(const T & aElem)
	{
		ASSERT(mSize != N);
		mStart[mSize++] = aElem;
	}

	/// Add element to start
	INLINE void push_front(const T & aElem)
	{
		ASSERT(mSize != N);
		if (mStart == mElements) // Is shift necessary ?
		{
			for (T * i = mElements + mSize; i > mElements; --i)
			{
				*i = *(i - 1);
			}
		}
		*mStart = aElem;
		++mSize;
	}
	/// Remove element from back
	INLINE void pop_back()
	{
		ASSERT(mSize != 0);
		--mSize;
	}

	/// Remove element from start
	INLINE void pop_front()
	{
		ASSERT(mSize != 0);
		++mStart;
		--mSize;
	}

	/// Clear array
	INLINE void clear()
	{
		mSize = 0;
		mStart = mElements;
	}

	/// Access functions
	INLINE T & operator[](int i)
	{
		ASSERT(mSize > i);
		return mStart[i];
	}

	INLINE const T & operator[](int i) const
	{
		ASSERT(mSize > i);
		return mStart[i];
	}

	INLINE T & front()
	{
		ASSERT(mSize > 0);
		return mStart[0];
	}

	INLINE T & back()
	{
		ASSERT(mSize > 0);
		return mStart[mSize - 1];
	}

	/// Other functions
	INLINE void sort()
	{
		std::sort(mStart, mStart + mSize);
	}

	/// Iterator
	class iterator
	{
		friend StaticArray;
		INLINE iterator(T * ptr) :mPtr(ptr)
		{
		}
#ifdef DEBUGARRAY
		INLINE iterator(T * ptr, T * start, T * end) : mPtr(ptr), mStart(start), mEnd(end)
		{
		}
#endif
	public:
		INLINE iterator() :
			mPtr(0)
		{
		}

		INLINE iterator operator++()
		{
			ASSERT(mEnd != mPtr);
			++mPtr;
			return *this;
		}

		INLINE iterator operator++(int)
		{
			ASSERT(mEnd != mPtr);
			iterator tmp = *this;
			++mPtr;
			return tmp;
		}

		INLINE iterator operator--()
		{
			ASSERT(mStart != mPtr);
			--mPtr;
			return *this;
		}

		INLINE iterator operator--(int)
		{
			ASSERT(mStart != mPtr);
			iterator tmp = *this;
			--mPtr;
			return tmp;
		}

		INLINE const T* operator->() const
		{
			ASSERT(mEnd != mPtr);
			return mPtr;
		}

		INLINE T* operator->()
		{
			ASSERT(mEnd != mPtr);
			return mPtr;
		}

		INLINE const T & operator*() const
		{
			ASSERT(mEnd != mPtr);
			return *mPtr;
		}

		INLINE T & operator*()
		{
			ASSERT(mEnd != mPtr);
			return *mPtr;
		}

		INLINE bool operator==(const iterator &aIt)
		{
			return mPtr == aIt.mPtr;
		}

		INLINE bool operator!=(const iterator &aIt)
		{
			return mPtr != aIt.mPtr;
		}
	private:
		T * mPtr;
#ifdef DEBUGARRAY
		T * mStart, *mEnd;
#endif
	};

	/// Constant iterator
	class const_iterator
	{
		friend StaticArray;
		INLINE const_iterator(const T * ptr) :mPtr(ptr)
		{
		}
#ifdef DEBUGARRAY
		INLINE const_iterator(const T * ptr, const T * start, const T * end) : mPtr(ptr), mStart(start), mEnd(end)
		{
		}
#endif
	public:
		INLINE const_iterator() :
		mPtr(0)
		{
		}

		INLINE const_iterator operator++()
		{
			ASSERT(mEnd != mPtr);
			++mPtr;
			return *this;
		}

		INLINE const_iterator operator++(int)
		{
			ASSERT(mEnd != mPtr);
			iterator tmp = *this;
			++mPtr;
			return tmp;
		}

		INLINE const_iterator operator--()
		{
			ASSERT(mStart != mPtr);
			--mPtr;
			return *this;
		}

		INLINE const_iterator operator--(int)
		{
			ASSERT(mStart != mPtr);
			iterator tmp = *this;
			--mPtr;
			return tmp;
		}

		INLINE const T *  operator->() const
		{
			ASSERT(mEnd != mPtr);
			return mPtr;
		}

		INLINE const T & operator*() const
		{
			ASSERT(mEnd != mPtr);
			return *mPtr;
		}

		INLINE bool operator==(const const_iterator &aIt)
		{
			return mPtr == aIt.mPtr;
		}

		INLINE bool operator!=(const const_iterator &aIt)
		{
			return mPtr != aIt.mPtr;
		}
	private:
		const T * mPtr;
#ifdef DEBUGARRAY
		const T * mStart, * mEnd;
#endif
	};

	/// Iterator functions

	INLINE iterator begin()
	{
#ifndef DEBUGARRAY
		return iterator(mStart);
#else
		return iterator(mStart,mStart,mStart+mSize);
#endif
	}

	INLINE iterator end()
	{
#ifndef DEBUGARRAY
		return iterator(mStart + mSize);
#else
		return iterator(mStart + mSize, mStart, mStart + mSize);
#endif
	}

	INLINE const_iterator begin() const
	{
#ifndef DEBUGARRAY
		return const_iterator(mStart);
#else
		return const_iterator(mStart, mStart, mStart + mSize);
#endif
	}

	INLINE const_iterator end() const
	{
#ifndef DEBUGARRAY
		return const_iterator(mStart + mSize);
#else
		return const_iterator(mStart + mSize, mStart, mStart + mSize);
#endif
	}

	INLINE const_iterator cbegin() const
	{
#ifndef DEBUGARRAY
		return const_iterator(mStart);
#else
		return const_iterator(mStart, mStart, mStart + mSize);
#endif
	}

	INLINE const_iterator cend() const
	{
#ifndef DEBUGARRAY
		return const_iterator(mStart + mSize);
#else
		return const_iterator(mStart + mSize, mStart, mStart + mSize);
#endif
	}
private:
	T mElements[N];
	T * mStart;
	size_t mSize;
};

#endif //__STATICARRAY_HXX__