
#include "sys_defs.h"

#ifdef HAS_REDIS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include "dict.h"
#include "mymalloc.h"
#include "vstring.h"
#include "stringops.h"

/* Global library. */

#include "cfg_parser.h"

/* Application-specific. */

#include "dict_redis.h"
#include "hiredis.h"

typedef struct {
    DICT    dict;
    CFG_PARSER *parser;
    redisContext *c;
    char   *host;
    int     port;
} DICT_REDIS;

/* internal function declarations */
static const char *dict_redis_lookup(DICT *, const char *);
DICT   *dict_redis_open(const char *, int, int);
static void dict_redis_close(DICT *);
static void redis_parse_config(DICT_REDIS *, const char *);


static const char *dict_redis_lookup(DICT *dict, const char *name)
{
    const char *myname = "dict_redis_lookup";
    DICT_REDIS *dict_redis = (DICT_REDIS *) dict;
    redisReply *reply;
    const char *r;
    VSTRING *result;
    dict->error = 0;
    
    result = vstring_alloc(10);
    VSTRING_RESET(result);
    VSTRING_TERMINATE(result);
    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, name);
	name = lowercase(vstring_str(dict->fold_buf));
    }
 
    if(dict_redis->c) {
        reply = redisCommand(dict_redis->c,"GET %s",name);
    }
    else {
        dict->error = DICT_ERR_CONFIG;
    }
    if(reply->str) {
        vstring_strcpy(result,reply->str);
        r = vstring_str(result);
        freeReplyObject(reply);
    }
    else {
        dict->error = DICT_ERR_RETRY;
    }
    return ((dict->error == 0 && *r) ? r : 0);
}

/* redis_parse_config - parse redis configuration file */

static void redis_parse_config(DICT_REDIS *dict_redis, const char *rediscf)
{
    const char *myname = "redisname_parse";
    CFG_PARSER *p = dict_redis->parser;

    dict_redis->port = cfg_get_int(p, "port", 6379, 0, 0);
    dict_redis->host = cfg_get_str(p, "host", "127.0.0.1", 1, 0);
}

/* dict_redis_open - open redis data base */

DICT   *dict_redis_open(const char *name, int open_flags, int dict_flags)
{
    DICT_REDIS *dict_redis;
    CFG_PARSER *parser;
    redisContext *c;

    /*
     * Open the configuration file.
     */
    if ((parser = cfg_parser_alloc(name)) == 0)
	return (dict_surrogate(DICT_TYPE_REDIS, name, open_flags, dict_flags,
			       "open %s: %m", name));

    dict_redis = (DICT_REDIS *) dict_alloc(DICT_TYPE_REDIS, name,
					   sizeof(DICT_REDIS));
    dict_redis->dict.lookup = dict_redis_lookup;
    dict_redis->dict.close = dict_redis_close;
    dict_redis->dict.flags = dict_flags;
    dict_redis->parser = parser;
    redis_parse_config(dict_redis, name);
    dict_redis->dict.owner = cfg_get_owner(dict_redis->parser);
    c = redisConnect(dict_redis->host,dict_redis->port);
    if(c->err) {
    	dict_redis->c = NULL;
    } else {
        dict_redis->c = c;
    }
    
    return (DICT_DEBUG (&dict_redis->dict));
}

/* dict_redis_close - close redis database */

static void dict_redis_close(DICT *dict)
{
    DICT_REDIS *dict_redis = (DICT_REDIS *) dict;

    cfg_parser_free(dict_redis->parser);
    myfree(dict_redis->host);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

#endif
