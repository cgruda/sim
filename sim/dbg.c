#include "dbg.h"

const char* dbg_level_stamp[DBG_MAX] = {
	[DBG_ERROR]   = "ERROR",
	[DBG_WARNING] = "WARN",
	[DBG_INFO]    = "INFO",
	[DBG_VERBOSE] = "VERB",
	[DBG_TRACE]   = "TRACE"
};