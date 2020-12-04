// TODO: get rid of _GNU_SOURCE for more portable code
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/input.h>

static const __u16 voidf[] = {KEY_V, KEY_O, KEY_I, KEY_D, KEY_F};
static const size_t voidf_count = sizeof(voidf) / sizeof(voidf[0]);

int is_voidf(size_t *cursor, __u16 x)
{
    if (voidf[*cursor] == x) {
        *cursor += 1;
        if (*cursor >= voidf_count) {
            *cursor = 0;
            return 1;
        }
    } else {
        *cursor = 0;
    }

    return 0;
}

#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"

static int is_event_device(const struct dirent *dir)
{
    return strncmp(EVENT_DEV_NAME, dir->d_name, 5) == 0;
}

void scan_devices(void)
{
    struct dirent **namelist;

    int ndev = scandir(DEV_INPUT_EVENT, &namelist, is_event_device, versionsort);
    printf("Found %d input devices\n", ndev);

    for (int i = 0; i < ndev; ++i) {
        char fname[512];
        char name[256] = "???";

        snprintf(fname, sizeof(fname),
                 "%s/%s", DEV_INPUT_EVENT, namelist[i]->d_name);

        int fd = open(fname, O_RDONLY);
        if (fd < 0) {
            printf("%s: ERROR: could not open the file\n", fname);
        } else {
            ioctl(fd, EVIOCGNAME(sizeof(name)), name);
            printf("%s: %s\n", fname, name);
            close(fd);
        }

        free(namelist[i]);
    }
}

int main()
{
    // TODO: filename is hardcoded
    const char *filename = "/dev/input/event14";

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("Could not open file %s\n", filename);
        exit(1);
    }

    int quit = 0;
    struct input_event ev[64];
    size_t cursor = 0;
    while (!quit) {
        int rd = read(fd, ev, sizeof(ev));
        size_t n = rd / sizeof(struct input_event);

        for (size_t i = 0; i < n; ++i) {
            if (ev[i].type == EV_KEY && ev[i].value == 0) {
                if(is_voidf(&cursor, ev[i].code)) {
                    printf("VOIDF IS COMING\n");
                }
            }
        }
    }

    close(fd);

    return 0;
}
