/* Copyright (c) 2004, 2015, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA */

/* get time since epoc in 100 nanosec units */
/* thus to get the current time we should use the system function
   with the highest possible resolution */

#include "mysys_priv.h"
#include "my_static.h"

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/**
  Get high-resolution time.

  @remark For windows platforms we need the frequency value of
          the CPU. This is initialized in my_init.c through
          QueryPerformanceFrequency(). If the Windows platform
          doesn't support QueryPerformanceFrequency(), zero is
          returned.

  @retval current high-resolution time.
*/

ulonglong my_getsystime()
{
#ifdef HAVE_CLOCK_GETTIME
  struct timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);
  return (ulonglong)tp.tv_sec*10000000+(ulonglong)tp.tv_nsec/100;
#elif defined(_WIN32)
  LARGE_INTEGER t_cnt;
  if (query_performance_frequency)
  {
    QueryPerformanceCounter(&t_cnt);
    return ((t_cnt.QuadPart / query_performance_frequency * 10000000) +
            ((t_cnt.QuadPart % query_performance_frequency) * 10000000 /
             query_performance_frequency) + query_performance_offset);
  }
  return 0;
#else
  /* TODO: check for other possibilities for hi-res timestamping */
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return (ulonglong)tv.tv_sec*10000000+(ulonglong)tv.tv_usec*10;
#endif
}


/**
  Return current time.

  @param  flags   If MY_WME is set, write error if time call fails.

  @retval current time.
*/

time_t my_time(myf flags)
{
  time_t t;
  /*
    The following loop is here beacuse time() may fail on some systems.
    We're using a hardcoded my_message_stderr() here rather than going
    through the hook in my_message_local() because it's far too easy to
    come full circle with any logging function that writes timestamps ...
  */
  while ((t= time(0)) == (time_t) -1)
  {
    if (flags & MY_WME)
      my_message_stderr(0, "time() call failed", MYF(0));
  }
  return t;
}


#ifdef _WIN32
#define OFFSET_TO_EPOCH 116444736000000000ULL
#endif

/**
  Return time in microseconds.

  @remark This function is to be used to measure performance in
          micro seconds. As it's not defined whats the start time
          for the clock, this function us only useful to measure
          time between two moments.

  @retval Value in microseconds from some undefined point in time.
*/

ulonglong my_micro_time()
{
#ifdef _WIN32
  ulonglong newtime;
  GetSystemTimeAsFileTime((FILETIME*)&newtime);
  newtime-= OFFSET_TO_EPOCH;
  return (newtime/10);
#else
  ulonglong newtime;
  struct timeval t;
  /*
    The following loop is here because gettimeofday may fail on some systems
  */
  while (gettimeofday(&t, NULL) != 0)
  {}
  newtime= (ulonglong)t.tv_sec * 1000000 + t.tv_usec;
  return newtime;
#endif
}

