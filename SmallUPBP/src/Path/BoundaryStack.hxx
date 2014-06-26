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

#ifndef __BOUNDARYSTACK_HXX__
#define __BOUNDARYSTACK_HXX__

#include "PriorityStack.hxx"

// For path tracing ids of material and medium of last hist geometry is stored on the stack
class StackElement
{
public:
	StackElement()
	{
	}

	StackElement(int materialId, int mediumId) :
		mMediumId( mediumId),
		mMaterialId( materialId )
	{
	}

	bool operator==(const StackElement & elem) const
	{
		return elem.mMediumId == mMediumId && elem.mMaterialId == mMaterialId;
	}

	bool operator!=(const StackElement & elem) const
	{
		return elem.mMediumId != mMediumId || elem.mMaterialId != mMaterialId;
	}

	int mMediumId;
	int mMaterialId;
};

#define STACK_TYPE 2
#if STACK_TYPE == 0
typedef PriorityStack<StackElement> BoundaryStack;
#elif STACK_TYPE == 1
typedef StaticPriorityStack<StackElement,5,5> BoundaryStack;
#else
typedef StaticPriorityStack2<StackElement,20> BoundaryStack;
#endif

#endif //__PRIORITYSTACK_HXX__