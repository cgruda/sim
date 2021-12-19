#ifndef _DBG_H_
#define _DBG_H_

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>

enum dbg_level {
	DBG_ERROR,
	DBG_WARNING,
	DBG_INFO,
	DBG_VERBOSE,
	DBG_TRACE,
	DBG_MAX
};

#define DBG_STDOUT	0
#define DBG_DUMP_TXT	1

#define DBG_OUTPUT	DBG_DUMP_TXT
#define DBG_LEVEL	DBG_MAX

#if (DBG_OUTPUT == DBG_DUMP_TXT)
	extern FILE *dbgfp;
	#define dbgpath "dbgdump.txt"
	#define dbg_fp_declare() FILE *dbgfp = NULL;
	#define dbg_fp_open() do { dbgfp = fopen(dbgpath, "w"); } while(0)
	#define dbg_fp_close() do { fclose(dbgfp); } while(0)
#else
	#define dbgfp stdout
	#define dbg_fp_declare()
	#define dbg_fp_open()
	#define dbg_fp_close()
#endif

#define DBG_LEVEL_STAMP(level)	(((level) == (DBG_ERROR))   ? "ERROR" : \
				 ((level) == (DBG_WARNING)) ? "WARN"  : \
				 ((level) == (DBG_INFO))    ? "INFO"  : \
				 ((level) == (DBG_VERBOSE)) ? "VERB"  : \
				 ((level) == (DBG_TRACE))   ? "TRACE" : \
				                              "N/A")

#define dbg_stamp() fprintf(dbgfp, "[%s][%d]", __func__, __LINE__)

#define dbg(level, ...)								\
	do {									\
		if (level <= DBG_LEVEL) {					\
			dbg_stamp();						\
			fprintf(dbgfp, "[%s] ", DBG_LEVEL_STAMP(level));	\
			fprintf(dbgfp, __VA_ARGS__);				\
		}								\
	} while (0)

#define dbg_error(...)   dbg(DBG_ERROR, __VA_ARGS__)
#define dbg_warning(...) dbg(DBG_WARNING, __VA_ARGS__)
#define dbg_info(...)    dbg(DBG_INFO, __VA_ARGS__)
#define dbg_verbose(...) dbg(DBG_VERBOSE, __VA_ARGS__)
#define dbg_trace(...)   dbg(DBG_TRACE, "%s\n", __func__)

#define print_error(...) 					\
	do {							\
		dbg_error(__VA_ARGS__);				\
		fprintf(dbgfp, ". %s\n", strerror(errno));	\
	} while (0)

#endif
