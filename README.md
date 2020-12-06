# voidf

## Quick Start

```console
$ make
$ ./voidf
```

## setuid/setgid support

The application supports temporary privilege escalation via setuid/setgid bits. It uses the privilege only to scan `/dev/input/*` folder and open the selected input device. After that it lowers its privilege down to its real uid/gid and continues to function like a regular user application.

### setuid example

```
# cp voidf /usr/local/bin
# chown root:root /usr/local/bin/voidf
# chmod 4755 /usr/local/bin/voidf
```

## References

- https://github.com/freedesktop-unofficial-mirror/evtest/blob/b8343ec1124da18bdabcc04809a8731b9e39295d/evtest.c
