#ifndef LOGD_DEBUG_H
#define LOGD_DEBUG_H

#ifdef LOGD_DEBUG
#define DEBUG_ASSERT(cond) assert(cond);
#else
#define DEBUG_ASSERT(cond)
#endif

#endif
