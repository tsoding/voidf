#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

const char *shift(int *argc, char ***argv)
{
    assert(*argc > 0);
    const char *result = *argv[0];
    *argc -= 1;
    *argv += 1;
    return result;
}

int main(int argc, char *argv[])
{
    shift(&argc, &argv);        // skip program name

    if (argc <= 0) {
        fprintf(stderr, "Usage: image2c <filepath>\n");
        fprintf(stderr, "ERROR: expected file path\n");
        exit(1);
    }

    const char *filepath = shift(&argc, &argv);

    int x, y, n;
    uint32_t *data = (uint32_t *)stbi_load(filepath, &x, &y, &n, 4);

    if (data == NULL) {
        fprintf(stderr, "Could not load file `%s`\n", filepath);
        exit(1);
    }

    // TODO(#11): include guards and the array name are not customizable
    printf("#ifndef IMAGE_H_\n");
    printf("#define IMAGE_H_\n");
    printf("size_t image_width = %d;\n", x);
    printf("size_t image_height = %d;\n", y);
    printf("uint32_t image[] = {");
    for (size_t i = 0; i < (size_t)(x * y); ++i) {
        printf("0x%x, ", data[i]);
    }
    printf("};\n");
    printf("#endif // IMAGE_H_\n");

    return 0;
}
