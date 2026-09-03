#include "png_parser.h"

#define BG_COL ((Rgb_t){ 255, 255, 255 })

int main(void) {
    const char *file = "../res/images/github_profile.png";

    PngHndl_t *hndl = imc_png_open(file);
    Pixmap_t *pixmap = imc_png_parse(hndl);
    //imc_pixmap_to_ppm(pixmap, "normal.ppm", BG_COL);
    //imc_pixmap_rotate_cw(pixmap);
    //imc_pixmap_to_ppm(pixmap, "rotated.ppm", BG_COL);
    //imc_pixmap_to_grayscale(pixmap);
    //imc_pixmap_to_ppm(pixmap, "grayscale.ppm", BG_COL);
    //imc_pixmap_to_ascii(pixmap, "ascii.txt");
    imc_png_close(hndl);

    return EXIT_SUCCESS;
}
