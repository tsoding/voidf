// TODO(#1): get rid of _GNU_SOURCE for more portable code
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

void bitmap_font_text_size(const char *cstr, int sw, int sh, int *w, int *h)
{
    *w = strlen(cstr) * BITMAP_FONT_CHAR_WIDTH * sw;
    *h = BITMAP_FONT_CHAR_HEIGHT * sh;
}

void bitmap_font_render(Bitmap_Font *font,
                        SDL_Renderer *renderer,
                        int x, int y,
                        int sw, int sh,
                        SDL_Color color,
                        const char *cstr)
{
    SDL_SetTextureColorMod(font->bitmap, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(font->bitmap, color.a);

    size_t n = strlen(cstr);

    for (int col = 0; (size_t) col < n; ++col) {
        const SDL_Rect src_rect = bitmap_font_char_rect(font, cstr[col]);
        const SDL_Rect dest_rect = {
            x + BITMAP_FONT_CHAR_WIDTH  * col * sw,
            y,
            src_rect.w * sw,
            src_rect.h * sh
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

typedef struct {
    struct dirent entry;
    char name[256];
} Device;

size_t scan_devices(Device *devices, size_t capacity)
{
    struct dirent **namelist;

    int ndev = scandir(DEV_INPUT_EVENT, &namelist, is_event_device, versionsort);

    size_t size = 0;
    for (int i = 0; i < ndev && size < capacity; ++i) {
        char fname[512];
        snprintf(fname, sizeof(fname),
                 "%s/%s", DEV_INPUT_EVENT, namelist[i]->d_name);

        int fd = open(fname, O_RDONLY);
        if (fd >= 0) {
            ioctl(fd, EVIOCGNAME(sizeof(devices[size].name)), devices[size].name);
            memcpy(&devices[size].entry, namelist[i], sizeof(devices[size].entry));
            size += 1;
            close(fd);
        }
        free(namelist[i]);
    }
    free(namelist);

    return size;
}

#define DEVICES_CAPACITY 256
Device devices[DEVICES_CAPACITY];

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define FPS 60
#define DELTA_TIME (1.0f / FPS)

typedef struct {
    char text[256];
    float a;
} Popup;

#define POPUPS_CAPACITY 32
Popup popups[POPUPS_CAPACITY] = {0};

#define POPUP_FADEOUT_RATE 1.5f
#define POPUP_FADEOUT_DISTANCE 150
#define POPUP_SWIDTH 20
#define POPUP_SHEIGHT 20
#define POPUP_COLOR ((SDL_Color) {255, 255, 255, 255})

void popups_render(SDL_Renderer *renderer, Bitmap_Font *font)
{
    for (size_t i = 0; i < POPUPS_CAPACITY; ++i) {
        if (popups[i].a > 0.0) {
            int w = 0;
            int h = 0;
            bitmap_font_text_size(popups[i].text, POPUP_SWIDTH, POPUP_SHEIGHT, &w, &h);
            const int x = SCREEN_WIDTH / 2 - w / 2;
            const int y = SCREEN_HEIGHT / 2 - h / 2 - (int) floorf((1.0 - popups[i].a) * POPUP_FADEOUT_DISTANCE);

            SDL_Color color = POPUP_COLOR;
            color.a = (Uint8) floorf(255.0f * popups[i].a);
            bitmap_font_render(
                    font,
                    renderer,
                    x, y,
                    POPUP_SWIDTH, POPUP_SHEIGHT,
                    color,
                    popups[i].text);
        }
    }
}

void popups_update(float dt)
{
    for (size_t i = 0; i < POPUPS_CAPACITY; ++i) {
        if (popups[i].a > 0) {
            popups[i].a -= dt * POPUP_FADEOUT_RATE;
        }
    }
}

void popups_new(const char *text)
{
    for (size_t i = 0; i < POPUPS_CAPACITY; ++i) {
        if (popups[i].a <= 0) {
            snprintf(popups[i].text, sizeof(popups[i].text), "%s", text);
            popups[i].a = 1.0f;
            return;
        }
    }
}

int main()
{
    const size_t devices_size = scan_devices(devices, DEVICES_CAPACITY);

    printf("Found %lu devices\n", devices_size);
    if (devices_size == 0) {
        printf("Most likely voidf does not have enough permissions to read files from %s\n", DEV_INPUT_EVENT);
        exit(1);
    }

    for (size_t i = 0; i < devices_size; ++i) {
        printf("%lu: %s\n", i, devices[i].name);
    }

    printf("Which one is your keyboard? [0-%lu] ", devices_size - 1);

    char input[256];
    fgets(input, sizeof(input), stdin);
    int selected_device = atoi(input);

    if (selected_device < 0 || selected_device >= (int) devices_size) {
        fprintf(stdout, "ERROR: Incorrect device number\n");
        exit(1);
    }

    printf("Selected %d: %s\n", selected_device, devices[selected_device].name);

    char filename[512];
    snprintf(filename, sizeof(filename), "%s/%s", DEV_INPUT_EVENT, 
             devices[selected_device].entry.d_name);

    printf("File path of the Device: %s\n", filename);

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Could not open file %s\n", filename);
        exit(1);
    }

    {
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "ERROR: Could not initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Window * const window = SDL_CreateWindow(
        "V̳͙̥̹̟͗̀̎̓͌͐́O̘̞͇̞̣͇͕͂͠I͙̋͐̍͂̀D̶͕̩̦̲͙F̟̖̮ͩ̏ͥ̂ͨ͠ ͍̰̫̯͙̯ͨ̉ͤ̈̿ͭI̤͍̲̯ͤ̎̀͝S̴̻͇̳̗̩ͧ̆ ̭̘̦ͭ͒Ĉ̸̰̼̤̖̲O̹̭̞̺̻͚̣̒M̪͓̗̤͋͢Ĩ͔̗̣̻̄̏̏̏̚N̳̦̂ͯ̅͂̓̈́G͈̣",
        0, 0,
        SCREEN_WIDTH, SCREEN_HEIGHT, 
        SDL_WINDOW_RESIZABLE);
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

    SDL_RenderSetLogicalSize(renderer,
            (int) SCREEN_WIDTH,
            (int) SCREEN_HEIGHT);

    // TODO(#3): charmap-oldschool.bmp should be baked into the executable
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
                        // TODO(#6): quake-style combo message
                        popups_new("voidf");
                    }
                }
            }
        }

        popups_update(DELTA_TIME);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        snprintf(voidf_buffer, sizeof(voidf_buffer), "%d", voidf_count);

        const int sw = 10 * 3;
        const int sh = 10 * 3;
        int w = 0;
        int h = 0;
        bitmap_font_text_size(voidf_buffer, sw, sh, &w, &h);
        const int x = SCREEN_WIDTH / 2 - w / 2;
        const int y = SCREEN_HEIGHT / 2 - h / 2;
        bitmap_font_render(&font,
                           renderer,
                           x, y,
                           sw, sh,
                           (SDL_Color) {255, 255, 255, 255},
                           voidf_buffer);

        popups_render(renderer, &font);

        SDL_Delay((int) floorf(DELTA_TIME * 1000.0f));
        SDL_RenderPresent(renderer);
    }

    close(fd);
    SDL_Quit();

    return 0;
}
