#include <sys/time.h> /* tm, timeval */
#include <stdio.h>    /* printf */
#include <stdarg.h>   /* va_arg */
#include <stdint.h>   /* uint */
#include <time.h>     /* strftime, localtime */
#include <errno.h>    /* errno */
#include <string.h>   /* strerror */

#include "log.h"

char* log_gettime()
{
	static time_t last;    // The last time that was retrieved
	static char buf[16];   // Time buffer to return
	static int len;        // Length of the Hour-Minute-Second info
	
	struct timeval tv;
	struct tm *now;

	// Get the current time
	gettimeofday(&tv, NULL);
	now = localtime(&(tv.tv_sec));

	// Return previous buffer if time is the same
	if(tv.tv_sec != last)
	{
		last = tv.tv_sec;
		len  = strftime(buf, sizeof(buf), "%H:%M:%S:", now);
	}
	
	// Print milliseconds in the remaining buffer
	snprintf(buf+len, 16-len, "%06d", (int) tv.tv_usec);

	return buf;
}

void log(const char* fmt, ...)
{
#ifndef LOG_DISABLE
	// Add timestamp
	fprintf(stdout, "%s - ", log_gettime());
	
	// Display message
	va_list args;
	va_start(args, fmt);
	vfprintf (stdout, fmt, args);
	va_end(args);
#endif
}

void log_error(const char* fmt, ...)
{
#ifndef LOG_DISABLE
	// Add timestamp
	fprintf(stderr, "%s - ", log_gettime());
	
	// Display message
	va_list args;
	va_start(args, fmt);
	vfprintf (stderr, fmt, args);
	va_end(args);

	// Print the Error afterwards 
	fprintf(stderr, " - Error: %s\n", strerror(errno));

	// Ensure error is printed
	fflush(stderr);
#endif
}