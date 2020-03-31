### Postfix Redis
This is a patch to the postfix 3.4.3+ MTA to add a lookup table for redis database.

This patch depends on hiredis client library.

Apply the patch to a vanilla postfix 3.4.3+ source tree

Copy dict_redis.c and dict_redis.h files to the postfix/src/global directory.

Copy the postfix-redis.patch to the postfix folder and apply with patch -p2 -i postfix-redis.patch

### Generate make files
```
$ make makefiles CCARGS="-DHAS_REDIS $(pkg-config --cflags hiredis)" AUXLIBS_REDIS="$(pkg-config --libs hiredis)" config_directory=/etc/postfix meta_directory=/etc/postfix daemon_directory=/usr/libexec/postfix shlib_directory=/usr/lib/postfix dynamicmaps=yes shared=yes
```

### Compile postfix
```
$ make
```

### Install postfix
``` 
$ make upgrade
```

### Example postfix configuration
main.cf:

```
virtual_mailbox_domains = redis:/etc/postfix/redis-vdomains.cf
virtual_mailbox_maps = redis:/etc/postfix/redis-vmailbox-maps.cf
virtual_alias_maps = redis:/etc/postfix/redis-valias-maps.cf
```

### Example .cf files
#### Defaults
```
host = 127.0.0.1
port = 6379
prefix = (none)
```

redis-vdomains.cf:
```
host = 127.0.0.1
port = 6379
prefix = VMD:
```

redis-vmailbox-maps.cf:
```
host = 127.0.0.1
port = 6379
prefix = VMM:
```

redis-redis-valias-maps.cf:
```
host = 127.0.0.1
port = 6379
prefix = VAM:
```

Example Redis keys and values:
```
1) "VMD:example.com" "example.com"
2) "VMM:user@example.com" "user@example.com"
3) "VAM:postmaster@example.com" "user@example.com"
```

### Testing Lookup

To test the lookup table you can postmap the string as follows
```
$ postmap -q "postmaster@example.com" redis:/etc/postfix/redis-redis-valias-maps.cf
```

## Creating Redis database
virtual_mailbox_domains
```
$ redis-cli set VMD:example.com example.com
```
virtual_mailbox_maps
```
$ redis-cli set VMM:user@example.com user@example.com
```

virtual_alias_maps
```
$ redis_cli set VAM:postmaster@example.com user@example.com
```
