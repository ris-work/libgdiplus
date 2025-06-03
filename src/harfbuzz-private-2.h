#include <stdio.h>
#include <stdlib.h>
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

/* Global variables */
hb_font_t   *hb_font = NULL;
FT_Library   ft_library = NULL;
FT_Face      ft_face = NULL;

/*
 * init_hb_font:
 *
 * Initializes FreeType, loads a font from 'font_path', sets a default
 * pixel size, and creates a HarfBuzz font from the loaded FreeType face.
 *
 * Call this function before any text drawing takes place.
 *
 * On error, the function prints a message and terminates the program.
 */
void init_hb_font(const char *font_path)
{
    /* Initialize FreeType library */
    if (FT_Init_FreeType(&ft_library)) {
        fprintf(stderr, "Error: Could not initialize FreeType library\n");
        exit(EXIT_FAILURE);
    }

    /* Load the font face. Replace font_path with the actual path to your font file */
    if (FT_New_Face(ft_library, font_path, 0, &ft_face)) {
        fprintf(stderr, "Error: Could not load font face from %s\n", font_path);
        exit(EXIT_FAILURE);
    }

    /* Set a default pixel size (for example, 12 pixels tall) */
    if (FT_Set_Pixel_Sizes(ft_face, 0, 12)) {
        fprintf(stderr, "Error: Could not set pixel size on the font face\n");
        exit(EXIT_FAILURE);
    }

    /* Create a HarfBuzz font from the FreeType face */
    hb_font = hb_ft_font_create(ft_face, NULL);
    if (!hb_font) {
        fprintf(stderr, "Error: Could not create HarfBuzz font from the FreeType face\n");
        exit(EXIT_FAILURE);
    }

    /* Optionally, you may print a success message */
    printf("Initialized HarfBuzz font successfully from: %s\n", font_path);
}

/*
 * cleanup_hb_font:
 *
 * Cleans up all globals created by init_hb_font.
 * Make sure to call this after you're done with drawing.
 */
void cleanup_hb_font(void)
{
    if (hb_font) {
        hb_font_destroy(hb_font);
        hb_font = NULL;
    }
    if (ft_face) {
        FT_Done_Face(ft_face);
        ft_face = NULL;
    }
    if (ft_library) {
        FT_Done_FreeType(ft_library);
        ft_library = NULL;
    }
}

