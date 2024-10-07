#ifndef __KernelDebug_H__
#define __KernelDebug_H__

#define DEBUG_ON

#ifdef DEBUG_ON
    // #define DEBUG(fmt, args...) printk("[FILE = %s][FUNC = %s][LINE = %d] [" fmt "]\n", __FILE__, __FUNCTION__, __LINE__, ##args)
    #define DEBUG(fmt, args...) printk("[FUNC = %s][LINE = %d] [" fmt "]\n", __FUNCTION__, __LINE__, ##args)
#else
    #define DEBUG(fmt, args...)

#endif

#endif