### Postfix Redis
This is a patch to the postfix 3 MTA to add a lookup table for redis database.
This patch depends on hiredis client library.

Apply the patch to a vanilla postfix 3 source tree

Copy dict_redis.c, dict_redis.h and hiredis.h files to the src/global directory  
Patch Makefile.in and mail_dict.c in src/global with the provided diffs

### Generate make files  
% make makefiles CCARGS="-DHAS_REDIS -I/usr/include/hiredis" AUXLIBS="-L/usr/lib -lhiredis"  
Change the include and library location for hiredis client library if required.  

### Compile postfix  
% make

### Install postfix  
% make install

### Using the lookup table  
transport_maps = redis:/etc/postfix/redis_transport_maps.cf  

redis_transport_maps.cf:  
host = localhost  
port = 6379

