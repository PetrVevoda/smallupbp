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

#ifndef __PRIORITYSTACK_HXX__
#define __PRIORITYSTACK_HXX__

#include <forward_list>

template <class ElementType>
class PriorityStack
{
public:

	PriorityStack() : mSize(0) {}

	// Gets the element at the top of the stack
	const ElementType& Top() const
	{
		return mData.front().first;
	}


	// Gets the priority of the element at the top of the stack
	int TopPriority() const
	{
		return mData.front().second;
	}

	// Gets the second element from the top of the stack
	const ElementType& SecondFromTop() const
	{
		std::forward_list<std::pair<ElementType, int>> ::const_iterator i = mData.cbegin();
		++i;
		return i->first;
	}

	// Inserts the given element from the top of the stack right below all elements with priority higher than the given one
	void Push(ElementType element, int priority)
	{
		std::forward_list<std::pair<ElementType, int>>::const_iterator i = mData.cbegin();
		std::forward_list<std::pair<ElementType, int>>::const_iterator j = mData.cbefore_begin();

		while (i != mData.cend() && i->second > priority) 
		{
			++i;
			++j;
		}

		mData.insert_after(j, std::make_pair(element, priority));
		++mSize;
	}

	// Removes the given element with the given priority from the stack. Does nothing if such combination is not present.
	void Pop(ElementType element, int priority)
	{
		std::forward_list<std::pair<ElementType, int>>::const_iterator i = mData.cbegin();
		std::forward_list<std::pair<ElementType, int>>::const_iterator j = mData.cbefore_begin();

		while (i != mData.cend() && i->second > priority) 
		{
			++i;
			++j;
		}

		while (i != mData.cend() && i->second == priority) 
		{
			if (i->first == element) 
			{
				mData.erase_after(j);
				--mSize;
				break;
			}
			else 
			{
				++i;
				++j;
			}
		}
	}

	// Test whether the given element with the given priority is on the stack
	bool Contains(ElementType element, int priority)
	{
		for (std::forward_list<std::pair<ElementType, int>>::const_iterator i = mData.cbegin(); i != mData.cend(); ++i)
		{
			if (i->first == element && i->second == priority) return true;
		}

		return false;
	}

	// Discards the element on the top of the stack
	void PopTop()
	{
		mData.pop_front();
		--mSize;
	}

	// Deletes all elements in the stack
	void Clear()
	{
		mData.clear();
		mSize = 0;
	}

	// Gets current number of elements in the stack
	int Size() const
	{
		return mSize;
	}

	// Test whether the stack is empty
	bool IsEmpty() const
	{
		return mSize == 0;
	}

private:
	int mSize;
	std::forward_list<std::pair<ElementType, int>> mData;
};

template <class ElementType, int N = 5, int MAX_PRIORITY = 5>
class StaticPriorityStack
{
public:

	StaticPriorityStack() 
	{
		Clear();
	}

	// Gets the element at the top of the stack
	const ElementType& Top() const
	{
		UPBP_ASSERT(mHighestPriority >= 0);
		return mData[mHighestPriority][mSize[mHighestPriority] - 1];
	}


	// Gets the priority of the element at the top of the stack
	int TopPriority() const
	{
		return mHighestPriority;
	}

	// Gets the second element from the top of the stack
	const ElementType& SecondFromTop() const
	{
		UPBP_ASSERT(mSecondHighestPriority >= 0);
		// More elements having the same priority?
		if (mSecondHighestPriority == mHighestPriority)
			return mData[mHighestPriority][mSize[mHighestPriority] - 2];
		else
			return mData[mSecondHighestPriority][mSize[mSecondHighestPriority] - 1];
	}

	// Inserts the given element from the top of the stack right below all elements with priority higher than the given one
	void Push(ElementType element, int priority)
	{
		if (mHighestPriority < priority)
		{
			mSecondHighestPriority = mHighestPriority;
			mHighestPriority = priority;
		}
		else if (mSecondHighestPriority <= priority)
		{
			mSecondHighestPriority = priority;
		}
		UPBP_ASSERT(mSize[priority] < N);
		mData[priority][mSize[priority]] = element;
		++mSize[priority];
		++mTotalSize;
	}

	// Removes the given element with the given priority from the stack. Does nothing if such combination is not present.
	void Pop(ElementType element, int priority)
	{
		// Goes from back, because it is more probable to find the element there
		for (ElementType * it = mData[priority] + mSize[priority] - 1; it >= mData[priority]; --it)
		{
			if (*it == element) // Found
			{
				// Move elements
				--mTotalSize;
				--mSize[priority];
				for (ElementType * it2 = it, *e = mData[priority] + mSize[priority]; it2 != e; ++it2)
					*it2 = it2[1];
				// Update highest priority
				bool updateSecond = mSecondHighestPriority == priority;
				if (mHighestPriority == priority)
				{
					mHighestPriority = mSecondHighestPriority;
					updateSecond = true;
				}
				if (updateSecond)
				{
					// Update second highest priority
					if (mSize[mHighestPriority] > 1)
						mSecondHighestPriority = mHighestPriority;
					else
					{
						mSecondHighestPriority = -1;
						for (int p = mHighestPriority - 1; p >= 0; --p)
						{
							if (mSize[p] > 0)
							{
								mSecondHighestPriority = p;
								break;
							}
						}
					}
				}
				break;
			}
		}
	}

	// Test whether the given element with the given priority is on the stack
	bool Contains(ElementType element, int priority)
	{
		// Goes from back, because it is more probable to find the element there
		for (ElementType * it = mData[priority] + mSize[priority] - 1; it >= mData[priority]; --it)
		{
			if (*it == element) 
				return true;
		}
		return false;
	}

	// Discards the element on the top of the stack
	void PopTop()
	{
		UPBP_ASSERT(mHighestPriority > -1);
		// Move elements
		--mTotalSize;
		--mSize[mHighestPriority];
		for (ElementType * it = mData[mHighestPriority], *e = mData[mHighestPriority] + mSize[mHighestPriority]; it != e; ++it)
			*it = it[1];
		// Update highest priority
		mHighestPriority = mSecondHighestPriority;
		// Update second highest priority
		if (mSize[mHighestPriority] > 1)
			mSecondHighestPriority = mHighestPriority;
		else
		{
			mSecondHighestPriority = -1;
			for (int p = mHighestPriority - 1; p >= 0; --p)
			{
				if (mSize[p] > 0)
				{
					mSecondHighestPriority = p;
					break;
				}
			}
		}
	}

	// Deletes all elements in the stack
	void Clear()
	{
		memset(mSize, 0, sizeof(int)*MAX_PRIORITY);
		mHighestPriority = -1;
		mSecondHighestPriority = -1;
		mTotalSize = 0;
	}

	// Gets current number of elements in the stack
	int Size() const
	{
		return mTotalSize;
	}

	// Test whether the stack is empty
	bool IsEmpty() const
	{
		return mTotalSize == 0;
	}

private:
	ElementType mData[MAX_PRIORITY][N]; // Stack data
	int mSize[MAX_PRIORITY]; // Sizes of each stack
	int mHighestPriority, mSecondHighestPriority; // Stores priority of element with highest priority and element with second highest priority (these two may have same value !!!)
	int mTotalSize; // Total stack size
};

template <class ElementType, int N = 20>
class StaticPriorityStack2
{
public:

	StaticPriorityStack2() : mSize(0) {}

	// Gets the element at the top of the stack
	const ElementType& Top() const
	{
		UPBP_ASSERT(mSize > 0);
		return mData[mSize-1].element;
	}


	// Gets the priority of the element at the top of the stack
	int TopPriority() const
	{
		UPBP_ASSERT(mSize > 0);
		return mData[mSize - 1].priority;
	}

	// Gets the second element from the top of the stack
	const ElementType& SecondFromTop() const
	{
		UPBP_ASSERT(mSize > 1);
		return mData[mSize - 2].element;
	}

	// Inserts the given element from the top of the stack right below all elements with priority higher than the given one
	void Push(ElementType element, int priority)
	{
		if (mSize == N)
		{
			std::cout << " Stack overflow - clearing last elem" << std::endl;
			--mSize;
		}
		for (DataElement * d = mData + mSize - 1; d >= mData; --d)
		{
			if (d->priority <= priority)
			{
				// Move elements
				for (DataElement * d2 = mData + mSize - 1; d2 != d; --d2)
					d2[1] = *d2;
				d[1].element = element;
				d[1].priority = priority;
				++mSize;
				return;
			}
		}
		// All elements have higher priority - move all elements
		for (DataElement * d2 = mData + mSize - 1; d2 >= mData; --d2)
			d2[1] = *d2;
		mData[0].element = element;
		mData[0].priority = priority;
		++mSize;
	}

	// Removes the given element with the given priority from the stack and returns true. Does nothing if such combination is not present and returns false.
	bool Pop(ElementType element, int priority)
	{
		for (DataElement * d = mData + mSize - 1; d >= mData && d->priority >= priority; --d)
		{
			if (d->priority == priority && d->element == element) // Found
			{
				for (DataElement * d2 = d, *e = mData + mSize - 1; d2 != e; ++d2)
					*d2 = d2[1];
				--mSize;
				return true;
			}
		}
		return false;
	}

	// Test whether the given element with the given priority is on the stack
	bool Contains(ElementType element, int priority)
	{
		for (DataElement * d = mData + mSize - 1; d >= mData && d->priority >= priority; --d)
		{
			if (d->priority == priority && d->element == element)
			{
				return true;
			}
		}
		return false;
	}

	// Discards the element on the top of the stack
	void PopTop()
	{
		UPBP_ASSERT(mSize > 0);
		--mSize;
	}

	// Deletes all elements in the stack
	void Clear()
	{
		mSize = 0;
	}

	// Gets current number of elements in the stack
	int Size() const
	{
		return mSize;
	}

	// Test whether the stack is empty
	bool IsEmpty() const
	{
		return mSize == 0;
	}

private:
	int mSize;

	struct DataElement
	{
		ElementType element;
		int priority;
	};

	DataElement mData[N];
};

#endif //__PRIORITYSTACK_HXX__