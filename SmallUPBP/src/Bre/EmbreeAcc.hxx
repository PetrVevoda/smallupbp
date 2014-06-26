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

#ifndef __EMBREEACC_HXX__
#define __EMBREEACC_HXX__

#include "include\embree.h"

/**
 * @brief	Simple class with static methods for initializing and destroying embree.
 */
class EmbreeAcc
{
public:

	/**
	 * @brief	To be called once before the first use of embree.
	 */
	static void initLib() 
	{
		embree::rtcInit();
		//embree::rtcSetVerbose(1);
		
		// Parameter below is the number of threads to use when building the data structure (0 means use all threads).
		embree::rtcStartThreads(0); 
	}

	/**
	 * @brief	To be called once after the last use of embree.
	 */
	static void cleanupLib() 
	{
		embree::rtcFreeMemory();
		embree::rtcStopThreads();
		embree::rtcExit();    
	}
};

#endif // __EMBREEACC_HXX__