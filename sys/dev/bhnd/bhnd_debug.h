/*
 * bhnd_debug.h
 *
 *  Created on: Feb 11, 2016
 *      Author: mizhka
 */

#pragma once

#define BHND_WARN_LEVEL		0x00
#define BHND_INFO_LEVEL		0x10
#define BHND_DEBUG_LEVEL	0x20
#define BHND_TRACE_LEVEL	0x30

#define BHND_DEFAULT_LEVEL	BHND_DEBUG_LEVEL

#if !(defined(BHND_LOGGING))
#define BHND_LOGGING	BHND_DEFAULT_LEVEL
#endif

#if BHND_LOGGING >= BHND_DEBUG_LEVEL
#define BHND_LOGPRINT(a, level) {printf("[BHND %s] => %s:%d: ", level, __func__, __LINE__), printf a; printf("\n");}
#else
#define BHND_LOGPRINT(a, level) {printf("bhnd: "), printf a; printf("\n");}
#endif

#if BHND_LOGGING >= BHND_WARN_LEVEL
#define BHND_WARN(a)  BHND_LOGPRINT(a,"!!!!!")
#else
#define BHND_WARN(a) ;
#endif

#if BHND_LOGGING >= BHND_INFO_LEVEL
#define BHND_INFO(a)  BHND_LOGPRINT(a," info")
#else
#define BHND_INFO(a) ;
#endif

#if BHND_LOGGING >= BHND_DEBUG_LEVEL
#define BHND_DEBUG(a) BHND_LOGPRINT(a,"debug")
#else
#define BHND_DEBUG(a) ;
#endif

#if BHND_LOGGING >= BHND_TRACE_LEVEL
#define BHND_TRACE(a) BHND_LOGPRINT(a,"trace")
#else
#define BHND_TRACE(a) ;
#endif
