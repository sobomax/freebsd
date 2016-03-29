/*
 * bhnd_debug.h
 *
 *  Created on: Feb 11, 2016
 *      Author: mizhka
 */

#pragma once
#include <sys/systm.h>

#define BHND_WARN_LEVEL		0x00
#define BHND_INFO_LEVEL		0x10
#define BHND_DEBUG_LEVEL	0x20
#define BHND_TRACE_LEVEL	0x30

#if !(defined(BHND_LOGGING))
#define BHND_LOGGING		BHND_TRACE_LEVEL
#endif

#if BHND_LOGGING >= BHND_DEBUG_LEVEL
#define BHND_LOGPRINT(a, level) 		{printf("[BHND %s] => %s:%d: ", level, __func__, __LINE__); printf a; printf("\n");}
#define BHND_DEVPRINT(dev, a, level) 	{device_printf(dev, "[BHND %s] => %s:%d: ", level, __func__, __LINE__); printf a; printf("\n");}
#else
#define BHND_LOGPRINT(a, level) 		{printf("bhnd: "), printf a; printf("\n");}
#define BHND_DEVPRINT(dev, a, level) 	{device_printf(dev, "bhnd: "), printf a; printf("\n");}
#endif

#define BHND_ERROR(a)			BHND_LOGPRINT(a, 	  "ERROR")
#define BHND_ERROR_DEV(dev, a)	BHND_DEVPRINT(dev, a, "ERROR")

#if BHND_LOGGING >= BHND_WARN_LEVEL
#define BHND_WARN(a)  			BHND_LOGPRINT(a, 	 "!!!!!")
#define BHND_WARN_DEV(dev, a) 	BHND_DEVPRINT(dev, a,"!!!!!")

#if BHND_LOGGING >= BHND_INFO_LEVEL
#define BHND_INFO(a)  			BHND_LOGPRINT(a,	 " info")
#define BHND_INFO_DEV(dev, a) 	BHND_DEVPRINT(dev, a," info")

#if BHND_LOGGING >= BHND_DEBUG_LEVEL
#define BHND_DEBUG(a) 			if(bootverbose) BHND_LOGPRINT(a,	 "debug")
#define BHND_DEBUG_DEV(dev, a) 	if(bootverbose) BHND_DEVPRINT(dev, a,"debug")

#if BHND_LOGGING >= BHND_TRACE_LEVEL
#define BHND_TRACE(a) 			if(bootverbose) BHND_LOGPRINT(a,	 "trace")
#define BHND_TRACE_DEV(dev, a) 	if(bootverbose) BHND_DEVPRINT(dev, a,"trace")

#endif
#endif
#endif
#endif

//Empty
#if !(defined(BHND_WARN))
#define BHND_WARN(a);
#endif
#if !(defined(BHND_INFO))
#define BHND_INFO(a);
#endif
#if !(defined(BHND_DEBUG))
#define BHND_DEBUG(a);
#endif
#if !(defined(BHND_TRACE))
#define BHND_TRACE(a);
#endif

#if !(defined(BHND_WARN_DEV))
#define BHND_WARN_DEV(dev, a);
#endif
#if !(defined(BHND_INFO_DEV))
#define BHND_INFO_DEV(dev, a);
#endif
#if !(defined(BHND_DEBUG_DEV))
#define BHND_DEBUG_DEV(dev, a);
#endif
#if !(defined(BHND_TRACE_DEV))
#define BHND_TRACE_DEV(dev, a);
#endif
