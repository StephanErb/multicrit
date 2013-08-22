/*
 * timeing.h
 *
 *  Created on: 08.09.2010
 *      Author: kobitzsch
 */

#ifndef TIMER_H_
#define TIMER_H_

#ifndef WIN32
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <stdint.h>
#endif

namespace utility{
namespace tool{

class ClockCounterTimer{
public:
	ClockCounterTimer( double frequency_ )
	: frequency( frequency_ )
	{}

	inline void start(){
#ifndef WIN32
		count_at_start = rdtsc();
#endif
	}

	inline double stop(){
#ifndef WIN32
		count_at_stop = rdtsc();
		return (double)(count_at_stop - count_at_start) / frequency;
#else
		return 0;
#endif
	}

	unsigned long long getTicks() const {
		return (count_at_stop - count_at_start);
	}

	inline double getTime() const {
#ifndef WIN32
		return 1000.0 * (double)(count_at_stop - count_at_start) / frequency;
#else
		return 0;
#endif
	}
protected:
	double frequency;
	unsigned long long count_at_start;
	unsigned long long count_at_stop;

	inline unsigned long long rdtsc() {
#		ifndef WIN32
		uint32_t lo, hi;
		__asm__ __volatile__ (      // serialize
		"xorl %%eax,%%eax \n        cpuid"
		::: "%rax", "%rbx", "%rcx", "%rdx");
		/* We cannot use "=A", since this would use %rax on x86_64 and return only the lower 32bits of the TSC */
		__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
		return (uint64_t)hi << 32 | lo;
#else
		return 0;
#endif
	}
};

class CPUTimer{
public:
	inline void start(){
#		ifndef WIN32
			timespec ts;
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
			cpu_time_start = timespecToDouble(ts);
#		endif
	}

	//returns elapsed time in seconds (cpu time)
	double stop(){
#		ifndef WIN32
			timespec ts;
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
			cpu_time_end = timespecToDouble(ts);
			return cpu_time_end - cpu_time_start;
#		else
			return 0.0;
#		endif
	}

	//Only meaningful if stopped
	double getTime(){
#		ifndef WIN32
		return cpu_time_end - cpu_time_start;
#		else
			return 0.0;
#		endif
	}

private:
#	ifndef WIN32
		static double timespecToDouble(const timespec & ts){
			return static_cast<double>(ts.tv_sec) + static_cast<double>(ts.tv_nsec) / 1000000000.0;
		}

		double cpu_time_start;
		double cpu_time_end;
#	endif
};



class TimeOfDayTimer{
public:
	inline void start(){
		mics_since_midnight_start = microsecsonds_since_midnight_GMT();
	}

	long long stop(){
		mics_since_midnight_end = microsecsonds_since_midnight_GMT();
		const long long offset = 1e10;
		while( mics_since_midnight_end < mics_since_midnight_start ){
			mics_since_midnight_start += offset;
			mics_since_midnight_end += offset;
		}
		return (mics_since_midnight_end - mics_since_midnight_start) / 1000.0;
	}

	long long getTime() const {
		return (mics_since_midnight_end - mics_since_midnight_start) / 1000.0;
	}

	double getTimeInSeconds() const {
		return (mics_since_midnight_end - mics_since_midnight_start) / 1000000.0;
	}

protected:
	long long mics_since_midnight_start;
	long long mics_since_midnight_end;

	long long microsecsonds_since_midnight_GMT()
	{
#ifndef WIN32
		struct timeval t;
		gettimeofday(&t,NULL);
		//std::cout << t.tv_sec << " : " << t.tv_usec << std::endl;
		return((long long)t.tv_sec%(24*3600) * 1000000 + t.tv_usec);
#else
		return 0;
#endif
	}
};


}
}


#endif /* TIMER_H_ */
