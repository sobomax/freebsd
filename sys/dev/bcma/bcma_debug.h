/*
 * bcma_debug.h
 *
 *  Created on: Jan 27, 2016
 *      Author: mizhka
 */

#pragma once

#define BCMA_WARN_LEVEL		0x00
#define BCMA_INFO_LEVEL		0x10
#define BCMA_DEBUG_LEVEL	0x20
#define BCMA_TRACE_LEVEL	0x30

#define BCMA_LOGGING	BCMA_INFO_LEVEL

#if BCMA_LOGGING >= BCMA_DEBUG_LEVEL
#define BCMA_LOGPRINT(a, level) {printf("[BCMA %s] => %s:%d: ", level, __func__, __LINE__), printf a; printf("\n");}
#else
#define BCMA_LOGPRINT(a, level) {printf("bcma: "), printf a; printf("\n");}
#endif

#if BCMA_LOGGING >= BCMA_WARN_LEVEL
#define BCMA_WARN(a)  BCMA_LOGPRINT(a,"!!!!!")
#else
#define BCMA_WARN(a) ;
#endif

#if BCMA_LOGGING >= BCMA_INFO_LEVEL
#define BCMA_INFO(a)  BCMA_LOGPRINT(a," info")
#else
#define BCMA_INFO(a) ;
#endif

#if BCMA_LOGGING >= BCMA_DEBUG_LEVEL
#define BCMA_DEBUG(a) BCMA_LOGPRINT(a,"debug")
#else
#define BCMA_DEBUG(a) ;
#endif

#if BCMA_LOGGING >= BCMA_TRACE_LEVEL
#define BCMA_TRACE(a) BCMA_LOGPRINT(a,"trace")
#else
#define BCMA_TRACE(a) ;
#endif
