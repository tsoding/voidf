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

### setgid example on Debian

On my Debian Stale 10 I have a group called `input` that has the access to `/dev/input/*` files. So this is what I do on my machine:

```
# cp voidf /usr/local/bin
# chown root:input /usr/local/bin/voidf
# chmod g+s /usr/local/bin/voidf
```

This way voidf is never ran with root privileges. It is only ran with temporary `input` group privilege just to open the input device file.

## Controls

To increment the counter you have to type the sequence of characters `voidf`. Focus on the main window is not required.

The following actions do require the window focus tho:

| Shortcut | Description |
| --- | --- |
| <kbd>F5</kbd> | Reset the counter |
| <kbd>Ctrl + Z</kbd> | Undo the counter reset |

## References

- https://github.com/freedesktop-unofficial-mirror/evtest/blob/b8343ec1124da18bdabcc04809a8731b9e39295d/evtest.c
- https://www.gnu.org/software/libc/manual/html_node/Setuid-Program-Example.html
