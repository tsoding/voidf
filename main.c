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

#include <SDL2/SDL.h>

static const __u16 voidf[] = {KEY_V, KEY_O, KEY_I, KEY_D, KEY_F};
static const size_t voidf_count = sizeof(voidf) / sizeof(voidf[0]);

SDL_Texture *load_texture_from_bmp_file(SDL_Renderer *renderer,
                                        const char *image_filepath,
                                        SDL_Color color_key)
{
    SDL_Surface *image_surface = SDL_LoadBMP(image_filepath);

    SDL_SetColorKey(
        image_surface,
        SDL_TRUE,
        SDL_MapRGB(
            image_surface->format,
            color_key.r,
            color_key.g,
            color_key.b));

    SDL_Texture *image_texture = SDL_CreateTextureFromSurface(renderer, image_surface);
    SDL_FreeSurface(image_surface);
    return image_texture;
}

#define BITMAP_FONT_ROW_SIZE    18
#define BITMAP_FONT_CHAR_WIDTH  7
#define BITMAP_FONT_CHAR_HEIGHT 9

typedef struct {
    SDL_Texture *bitmap;
} Bitmap_Font;

SDL_Rect bitmap_font_char_rect(Bitmap_Font *font, char x)
{
    if (32 <= x && x <= 126) {
        const SDL_Rect rect = {
            ((x - 32) % BITMAP_FONT_ROW_SIZE) * BITMAP_FONT_CHAR_WIDTH,
            ((x - 32) / BITMAP_FONT_ROW_SIZE) * BITMAP_FONT_CHAR_HEIGHT,
            BITMAP_FONT_CHAR_WIDTH,
            BITMAP_FONT_CHAR_HEIGHT
        };
        return rect;
    } else {
        return bitmap_font_char_rect(font, '?');
    }
}

Bitmap_Font bitmap_font_from_file(SDL_Renderer *renderer, const char *file_path)
{
    Bitmap_Font result = {0};
    result.bitmap = load_texture_from_bmp_file(renderer, file_path, (SDL_Color) {0, 0, 0, 255});
    return result;
}

void bitmap_font_render(Bitmap_Font *font,
                        SDL_Renderer *renderer,
                        int x, int y,
                        int w, int h,
                        SDL_Color color,
                        const char *cstr)
{
    SDL_SetTextureColorMod(font->bitmap, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(font->bitmap, color.a);

    size_t n = strlen(cstr);

    for (int col = 0; (size_t) col < n; ++col) {
        const SDL_Rect src_rect = bitmap_font_char_rect(font, cstr[col]);
        const SDL_Rect dest_rect = {
            x + BITMAP_FONT_CHAR_WIDTH  * col * w,
            y + BITMAP_FONT_CHAR_HEIGHT       * h,
            src_rect.w * w,
            src_rect.h * h
        };
        SDL_RenderCopy(renderer, font->bitmap, &src_rect, &dest_rect);
    }
}

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
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "ERROR: Could not initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Window * const window = SDL_CreateWindow(
        "V̳͙̥̹̟͗̀̎̓͌͐́O̘̞͇̞̣͇͕͂͠I͙̋͐̍͂̀D̶͕̩̦̲͙F̟̖̮ͩ̏ͥ̂ͨ͠ ͍̰̫̯͙̯ͨ̉ͤ̈̿ͭI̤͍̲̯ͤ̎̀͝S̴̻͇̳̗̩ͧ̆ ̭̘̦ͭ͒Ĉ̸̰̼̤̖̲O̹̭̞̺̻͚̣̒M̪͓̗̤͋͢Ĩ͔̗̣̻̄̏̏̏̚N̳̦̂ͯ̅͂̓̈́G͈̣",
        0, 0,
        800, 600, SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        fprintf(stderr, "ERROR: Could not initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Renderer *const renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        fprintf(stderr, "ERROR: Could not initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    // TODO: filename is hardcoded
    const char *filename = "/dev/input/event14";

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        printf("Could not open file %s\n", filename);
        exit(1);
    }

    {
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    // TODO: charmap-oldschool.bmp should be baked into the executable
    Bitmap_Font font = bitmap_font_from_file(renderer, "./charmap-oldschool.bmp");

    int quit = 0;
    struct input_event ev[64];
    size_t cursor = 0;
    int voidf_count = 0;
    char voidf_buffer[1024];
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT: {
                quit = 1;
            } break;
            }
        }

        int rd = read(fd, ev, sizeof(ev));

        if (rd > 0) {
            size_t n = rd / sizeof(struct input_event);

            for (size_t i = 0; i < n; ++i) {
                if (ev[i].type == EV_KEY && ev[i].value == 0) {
                    if(is_voidf(&cursor, ev[i].code)) {
                        printf("VOIDF IS COMING\n");
                        voidf_count += 1;
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        snprintf(voidf_buffer, sizeof(voidf_buffer), "%d", voidf_count);

        // TODO: voidf counter is ugly
        bitmap_font_render(&font,
                           renderer,
                           0, 0,
                           10, 10,
                           (SDL_Color) {255, 255, 255, 255},
                           voidf_buffer);

        SDL_Delay(10);
        SDL_RenderPresent(renderer);
    }

    close(fd);
    SDL_Quit();

    return 0;
}
