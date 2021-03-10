/*++
/* NAME
/*	dict_redis 3
/* SUMMARY
/*	dictionary manager interface to Redis databases
/* SYNOPSIS
/*	#include <dict_redis.h>
/*
/*	DICT	*dict_redis_open(name, open_flags, dict_flags)
/*	const char *name;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_redis_open() creates a dictionary of type 'redis'.  This
/*	dictionary is an interface for the postfix key->value mappings
/*	to redis.  The result is a pointer to the installed dictionary,
/*	or a null pointer in case of problems.
/* .PP
/*	Arguments:
/* .IP name
/*	Either the path to the Redis configuration file (if it
/*	starts with '/' or '.'), or the prefix which will be used to
/*	obtain main.cf configuration parameters for this search.
/*
/*	In the first case, the configuration parameters below are
/*	specified in the file as \fIname\fR=\fIvalue\fR pairs.
/*
/*	In the second case, the configuration parameters are
/*	prefixed with the value of \fIname\fR and an underscore,
/*	and they are specified in main.cf.  For example, if this
/*	value is \fIredisDB\fR, the parameters would look like
/*	\fIredisDB_host\fR, \fIpredisDB_prefix\fR, and so on.
/* .IP other_name
/*	reference for outside use.
/* .IP open_flags
/*	unused.
/* .IP dict_flags
/*	See dict_open(3).
/*
/* .PP
/*	Configuration parameters:
/* .IP host
/*	IP address of the redis server hosting the database.
/* .IP port
/*	Port number to connect to the above.
/* .IP prefix
/*	Prefix to add to the key when looking it up.
/* .PP
/*	For example, if you want the map to reference databases on
/*	redis host "127.0.0.1" and prefix the query with VDOM:
/*	Then the configuration file
/*	should read:
/* .PP
/*	host = 127.0.0.1
/* .br
/*	port = 6379
/* .br
/*	prefix = VDOM:
/* .PP
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* AUTHOR(S)
/*	Titus Jose
/*	titus.nitt@gmail.com
/*
/*	Updated by:
/*	Duncan Bellamy
/*	dunk@denkimushi.com
/*--*/

#include "sys_defs.h"

#ifdef HAS_REDIS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include "msg.h"
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
    char   *prefix;
    VSTRING *result;
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
    int error;
    dict->error = 0;

    if (msg_verbose)
        msg_info("%s: Requesting key %s%s",dict_redis->host,dict_redis->prefix,name);
    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, name);
	name = lowercase(vstring_str(dict->fold_buf));
    }

    reply = redisCommand(dict_redis->c,"GET %s%s",dict_redis->prefix,name);
    error = dict->error;
    if(reply->str) {
        vstring_strcpy(dict_redis->result,reply->str);
        r = vstring_str(dict_redis->result);
    }
    else {
        error = 1;
    }
    freeReplyObject(reply);
    return ((error == 0 && *r) ? r : 0);
}

/* redis_parse_config - parse redis configuration file */

static void redis_parse_config(DICT_REDIS *dict_redis, const char *rediscf)
{
    const char *myname = "redis_parse_config";
    CFG_PARSER *p = dict_redis->parser;

    dict_redis->port = cfg_get_int(p, "port", 6379, 0, 0);
    dict_redis->host = cfg_get_str(p, "host", "127.0.0.1", 1, 0);
    dict_redis->prefix = cfg_get_str(p, "prefix", "", 0, 0);
    dict_redis->result = vstring_alloc(10);
}

/* dict_redis_open - open redis data base */

DICT   *dict_redis_open(const char *name, int open_flags, int dict_flags)
{
    DICT_REDIS *dict_redis;
    CFG_PARSER *parser;

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
    dict_redis->c = redisConnect(dict_redis->host,dict_redis->port);
    if(dict_redis->c == NULL || dict_redis->c->err) {
        msg_warn("%s:%s: Cannot connect to Redis server %s",
            DICT_TYPE_REDIS, name, dict_redis->host);
        dict_redis->dict.close((DICT *)dict_redis);
        return (dict_surrogate(DICT_TYPE_REDIS, name, open_flags, dict_flags,
			       "open %s: %m", name));
    }

    return (DICT_DEBUG (&dict_redis->dict));
}

/* dict_redis_close - close redis database */

static void dict_redis_close(DICT *dict)
{
    DICT_REDIS *dict_redis = (DICT_REDIS *) dict;

    if (dict_redis->c) {
        redisFree(dict_redis->c);
        dict_redis->c = NULL;
    }
    cfg_parser_free(dict_redis->parser);
    myfree(dict_redis->host);
    myfree(dict_redis->prefix);
    vstring_free(dict_redis->result);
    if (dict->fold_buf)
	    vstring_free(dict->fold_buf);
    dict_free(dict);
}

#endif
