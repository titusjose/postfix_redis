#ifndef _DICT_REDIS_H_INCLUDED_
#define _DICT_REDIS_H_INCLUDED_

#include <dict.h>

#define DICT_TYPE_REDIS	"redis"

extern DICT *dict_redis_open(const char *, int, int);

#endif
