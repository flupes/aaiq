#ifndef AAQIM_DEBUG_H
#define AAQIM_DEBUG_H

#if defined(AAQIM_DEBUG)
#define dbg_printf(args...) printf(args)
#else
#define dbg_printf(...)
#endif

#endif
