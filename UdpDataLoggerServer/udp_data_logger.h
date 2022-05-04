
#ifndef __UDP_DATA_LOGGER__
#define __UDP_DATA_LOGGER__

#include <xdc/std.h>

/* Description: Function for sending UDP data. */
extern void udpSend();

/* Description: Get total number of lost datagrams. */
extern UInt32 getLostDatagrams();

#endif  /* __UDP_DATA_LOGGER__ */
