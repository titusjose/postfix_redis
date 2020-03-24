### Postfix Redis
This is a patch to the postfix 3.4.3+ MTA to add a lookup table for redis database.

This patch depends on hiredis client library.

Apply the patch to a vanilla postfix 3.4.3+ source tree

Copy dict_redis.c and dict_redis.h files to the postfix/src/global directory.

Copy the postfix-redis.patch to the postfix folder and apply with patch -p2 -i postfix-redis.patch

### Generate make files
```
#make makefiles CCARGS="-DHAS_REDIS $(pkg-config --cflags hiredis)" AUXLIBS_REDIS="$(pkg-config --libs hiredis)" config_directory=/etc/postfix meta_directory=/etc/postfix daemon_directory=/usr/libexec/postfix shlib_directory=/usr/lib/postfix dynamicmaps=yes shared=yes
```

### Compile postfix
```
#make
```

### Install postfix
``` 
#make upgrade
```

### Using the lookup table
transport_maps = redis:/etc/postfix/redis_transport_maps.cf

redis_transport_maps.cf:
host = localhost
port = 6379
