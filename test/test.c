#include "png_parser.h"

#define BG_COL ((Rgb_t){ 255, 255, 255 })

int main(void) {
    int x, y;
    PngHndl_t *hndl = NULL;
    Pixmap_t *pixmap = NULL;
    const char *file = "test.png";

    hndl = imc_png_open(file);
    pixmap = imc_png_parse(hndl);

    imc_pixmap_to_ppm(pixmap, "normal.ppm", BG_COL);
    imc_pixmap_rotate_cw(pixmap);
    imc_pixmap_to_ppm(pixmap, "rotated.ppm", BG_COL);

    imc_png_close(hndl);
    return 0;
}
