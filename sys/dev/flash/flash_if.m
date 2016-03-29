# $FreeBSD$

# Flash chip interface description
#

#include <sys/bus.h>

INTERFACE flash;

# Get flash size
#
# Return values:
#  size 
#
METHOD uint64_t get_size {
	device_t dev;
};