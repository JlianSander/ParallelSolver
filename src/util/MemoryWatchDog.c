#include "../../include/util/MemoryWatchDog.h"

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

long get_mem_usage()
{
	struct rusage rsc_usage;
	getrusage(RUSAGE_SELF, &rsc_usage);
	return rsc_usage.ru_maxrss;
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

long get_ctxt_switches_volun()
{
	struct rusage rsc_usage;
	getrusage(RUSAGE_SELF, &rsc_usage);
	return rsc_usage.ru_nvcsw;
}

/*===========================================================================================================================================================*/
/*===========================================================================================================================================================*/

long get_ctxt_switches_involun()
{
	struct rusage rsc_usage;
	getrusage(RUSAGE_SELF, &rsc_usage);
	return rsc_usage.ru_nivcsw;
}