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

#ifndef __TIMER_HXX__
#define __TIMER_HXX__

// Include ---------------------------------------------------------------
#define NOMINMAX

#include <time.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#define MAX_TIMER_NAME		256

/**
 * @brief	Use to enable high performance counting through MS counters.
 */
#define MS_TIMER

// Declaration -----------------------------------------------------------

class Timer {

// Static data ------------------------------------------------------

// Data -------------------------------------------------------------
private:
#ifdef MS_TIMER
	__int64	_timeElapsed;          //!< Total time spent in all the call start/stop.
	__int64	_lastTimeElapsed;      //!< Time spent in the last call start/stop.
	__int64	_lastTotalTimeElapsed; //!< Last total time before reset.
	__int64	_lastCountRepetition;  //!< Before reset.
	__int64	_timeStarted;          //!< System time from start.
	__int64	_count;                //!< Number of times stopped.
	__int64	_countRepetition;      //!< Number of time repeated.
#else
	long	_timeElapsed;          //!< Total time spent in all the call start/stop.
	long	_lastTimeElapsed;      //!< Time spent in the last call start/stop.
	long	_lastTotalTimeElapsed; //!< Last total time before reset.
	long	_lastCountRepetition;  //!< Before reset.
	long	_timeStarted;          //!< System time from start.
	long	_count;                //!< Number of times stopped.
	long	_countRepetition;      //!< Number of time repeated.
#endif

#ifdef MS_TIMER
  __int64           _counterFrequency;
  int               _counterInitialized;
#endif

// Public methods ---------------------------------------------------
public:

	/**
	 * @brief	Default constructor. Creates an empty timer.
	 */
	Timer();

	/**
	 * @brief	Resets the timer.
	 */
	void Reset();

	/**
	 * @brief	Starts the timer.
	 */
	void Start();

	/**
	 * @brief	Resets and starts the timer.
	 */
	void Restart() { Reset(); Start(); }

	/**
	 * @brief	Stop the timer and add the time to the total time spent timeElapsed
	 */
	void Stop();

	/**
	 * @brief	As stop but update the repetition counter with the given number.
	 *
	 * @param	repetition	The repetition.
	 */
	void Stop(long repetition);

	/**
	 * @brief	Gets elapsed time.
	 *
	 * @return	The elapsed time.
	 */
	double GetElapsedTime();

	/**
	 * @brief	Gets the last elapsed time (before reset).
	 *
	 * @return	The last elapsed time.
	 */
	double GetLastElapsedTime();

	/**
	 * @brief	Gets average elapsed time, that is elapsed time/repetition.
	 *
	 * @return	The average elapsed time.
	 */
	double GetAverageElapsedTime();	

	/**
	 * @brief	Gets the last average elapsed time, that is elapsed time/repetition (before reset).
	 *
	 * @return	The last average elapsed time.
	 */
	double GetLastAverageElapsedTime();	

#ifdef MS_TIMER

	/**
	 * @brief	Gets count.
	 *
	 * @return	The count.
	 */
	__int64 GetCount();

	/**
	 * @brief	Gets repetition count.
	 *
	 * @return	The repetition count.
	 */
	__int64 GetRepetitionCount();

	/**
	 * @brief	Gets global time.
	 *
	 * @return	The global time.
	 */
	static __int64 GetGlobalTime();
#else
	/**
	 * @brief	Gets count.
	 *
	 * @return	The count.
	 */
	long GetCount();

	/**
	 * @brief	Gets repetition count.
	 *
	 * @return	The repetition count.
	 */
	long GetRepetitionCount();

	/**
	 * @brief	Gets global time.
	 *
	 * @return	The global time.
	 */
  static long GetGlobalTime();
#endif

  /**
   * @brief	Gets global time clock.
   *
   * @return	The global time clock.
   */
  static long GetGlobalTimeClock();

};

// Inline functions ------------------------------------------------------

/**
 * @brief	Default constructor. Creates an empty timer.
 */
inline Timer::Timer() {
	_timeElapsed = 0;
	_countRepetition = 0;
	Reset();
#ifdef MS_TIMER
  _counterInitialized = 
    QueryPerformanceFrequency((LARGE_INTEGER*)&_counterFrequency);
#endif
}

/**
 * @brief	Resets the timer.
 */
inline void Timer::Reset() {
	_lastTotalTimeElapsed = _timeElapsed;
	_lastCountRepetition = _countRepetition;

	_timeElapsed = 0;
	_lastTimeElapsed = 0;
	_count = 0;
	_countRepetition = 0;
}

/**
 * @brief	Starts the timer.
 */
inline void Timer::Start() {
#ifdef MS_TIMER
  QueryPerformanceCounter((LARGE_INTEGER*)&_timeStarted);
#else
	_timeStarted = clock();
#endif
}

/**
 * @brief	Stop the timer and add the time to the total time spent timeElapsed.
 */
inline void Timer::Stop() {
#ifdef MS_TIMER
  __int64 t;
  QueryPerformanceCounter((LARGE_INTEGER*)&t);
	_lastTimeElapsed = t - _timeStarted;
#else
	_lastTimeElapsed = clock() - _timeStarted;
#endif
	_timeElapsed += _lastTimeElapsed;
	_count ++;
	_countRepetition ++;
}

/**
 * @brief	As stop but update the repetition counter with the given number.
 *
 * @param	repetition	The repetition.
 */
inline void Timer::Stop(long repetition) {
#ifdef MS_TIMER
  __int64 t;
  QueryPerformanceCounter((LARGE_INTEGER*)&t);
  _timeElapsed += t - _timeStarted;
#else
	_timeElapsed += clock() - _timeStarted;
#endif
	_count ++;
	_countRepetition += repetition;
}

/**
 * @brief	Gets the last elapsed time (before reset).
 *
 * @return	The last elapsed time.
 */
inline double Timer::GetLastElapsedTime() {
#ifdef MS_TIMER
	return (double)_lastTimeElapsed / (double)_counterFrequency; 
#else
	return (double)_lastTimeElapsed / (double)CLOCKS_PER_SEC; 
#endif
}

/**
 * @brief	Gets elapsed time.
 *
 * @return	The elapsed time.
 */
inline double Timer::GetElapsedTime() {
#ifdef MS_TIMER
	return (double)_timeElapsed / (double)_counterFrequency; 
#else
	return (double)_timeElapsed / (double)CLOCKS_PER_SEC; 
#endif
}

/**
 * @brief	Gets average elapsed time, that is elapsed time/repetition.
 *
 * @return	The average elapsed time.
 */
inline double Timer::GetAverageElapsedTime() {
#ifdef MS_TIMER
	return (double)_timeElapsed / ((double)_counterFrequency*(double)_countRepetition); 
#else
	return (double)_timeElapsed / 
		((double)CLOCKS_PER_SEC * (double)_countRepetition); 
#endif
}

/**
 * @brief	Gets the last average elapsed time, that is elapsed time/repetition (before reset).
 *
 * @return	The last average elapsed time.
 */
inline double Timer::GetLastAverageElapsedTime() {
  if(_lastCountRepetition) {
#ifdef MS_TIMER
		return (double)_lastTotalTimeElapsed / 
			((double)_counterFrequency * (double)_lastCountRepetition); 
#else
		return (double)_lastTotalTimeElapsed / 
			((double)CLOCKS_PER_SEC * (double)_lastCountRepetition); 
#endif
  } else {
		return 0;
  }
}

#ifdef MS_TIMER

/**
 * @brief	Gets count.
 *
 * @return	The count.
 */
inline __int64 Timer::GetCount() {
	return _count;
}

/**
 * @brief	Gets repetition count.
 *
 * @return	The repetition count.
 */
inline __int64 Timer::GetRepetitionCount() {
	return _countRepetition;
}

/**
 * @brief	Gets global time.
 *
 * @return	The global time.
 */
inline __int64 Timer::GetGlobalTime() {
  __int64 t;
  QueryPerformanceCounter((LARGE_INTEGER*)&t);
  return t;
}

#else

/**
 * @brief	Gets count.
 *
 * @return	The count.
 */
inline long Timer::GetCount() {
	return _count;
}

/**
 * @brief	Gets repetition count.
 *
 * @return	The repetition count.
 */
inline long Timer::GetRepetitionCount() {
	return _countRepetition;
}

/**
 * @brief	Gets global time.
 *
 * @return	The global time.
 */
inline long Timer::GetGlobalTime() {
  long t;
  t = clock();
  return t;
}

#endif

/**
 * @brief	Gets global time clock.
 *
 * @return	The global time clock.
 */
inline long Timer::GetGlobalTimeClock() {
  long t;
  t = clock();
  return t;
}

#endif // __TIMER_HXX__