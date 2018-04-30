#ifndef LOGD_UTIL_H
#define LOGD_UTIL_H

#include "log.h"

void printl(log_t* l);
int snprintl(char* buf, int blen, log_t* l);

#endif
