#ifndef HARFBUZZ_PRIVATE_H
#define HARFBUZZ_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Standard C includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* FreeType, Cairo, and HarfBuzz includes */
#include <ft2build.h>
#include FT_FREETYPE_H

#include <cairo.h>
#include <cairo-ft.h>

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-icu.h>
#include "harfbuzz-private-lang.h"

/* Global state variables (static to this translation unit) */
	static double g_default_font_size = 12.0;
	cairo_font_face_t *g_cairo_face = NULL;
static hb_font_t   *g_hb_font = NULL;
static FT_Library   ft_library = NULL;
static FT_Face      g_ft_face = NULL;
static double       g_extra_char_spacing_factor = 0.15;
static int          g_text_shaping_initialized = 0;
//hb_language_t g_hb_language;
/* In your header file, e.g., harfbuzz-private-2.h */
//extern hb_script_t g_hb_script;
//hb_script_t g_hb_script = HB_SCRIPT_TAMIL;
static unsigned char *g_font_buffer = NULL;
static size_t g_font_buffer_size = 0;


// Helper function to load the entire font file into memory.
unsigned char* load_font_file(const char* font_path, size_t* out_size) {
    FILE *fp = fopen(font_path, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open font file %s\n", font_path);
        exit(EXIT_FAILURE);
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    unsigned char *buffer = malloc(size);
    if (!buffer) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    if (fread(buffer, 1, size, fp) != (size_t)size) {
        fprintf(stderr, "Error: Could not read the full font file\n");
        free(buffer);
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    fclose(fp);

    *out_size = (size_t)size;
    return buffer;
}


/**
 * gdiplus_get_font_path
 *
 * Returns the font file path from the environment variable "GDIPLUS_FONT_PATH".
 * If not set or empty, returns "NotoSans-Regular.ttf".
 */
static inline const char* gdiplus_get_font_path(void)
{
    const char *font_env = getenv("GDIPLUS_FONT_PATH");
    return (font_env && font_env[0] != '\0') ? font_env : "NotoSans-Regular.ttf";
}

/**
 * gdiplus_get_extra_char_spacing
 *
 * Returns the extra character spacing multiplier from the environment variable
 * "GDIPLUS_EXTRA_CHAR_SPACING_FACTOR". If not set, returns 0.15.
 */
static inline double gdiplus_get_extra_char_spacing(void)
{
    const char *spacing_env = getenv("GDIPLUS_EXTRA_CHAR_SPACING_FACTOR");
    return (spacing_env && spacing_env[0] != '\0') ? atof(spacing_env) : 0.15;
}
static inline hb_language_t gdiplus_get_hb_language(void)
{
    const char *lang_env = getenv("GDIPLUS_HARFBUZZ_LANGUAGE");
    return (lang_env && lang_env[0] != '\0')
               ? hb_language_from_string(lang_env, -1)
               : hb_language_from_string("en", -1);
}


/**
 * init_text_shaping
 *
 * Initializes FreeType and HarfBuzz using the font path from gdiplus_get_font_path()
 * (or default "NotoSans-Regular.ttf"). It also sets the default pixel size and retrieves
 * the extra character spacing factor from the environment. This function is guarded so
 * that initialization occurs only once.
 *
 * Must be called once before any text rendering.
 */
static inline void init_text_shaping(void)
{
    if (g_text_shaping_initialized)
        return;
    g_hb_language = gdiplus_get_hb_language();
    
    const char *font_path = gdiplus_get_font_path();
    
    if (FT_Init_FreeType(&ft_library)) {
        fprintf(stderr, "Error: Could not initialize FreeType library\n");
        exit(EXIT_FAILURE);
    }
    g_font_buffer = load_font_file(font_path, &g_font_buffer_size);
    
    if (FT_New_Memory_Face(ft_library, g_font_buffer, g_font_buffer_size, 0, &g_ft_face) != 0) {
        fprintf(stderr, "Error: Could not load font face from %s\n @3", font_path);
        exit(EXIT_FAILURE);
    }

        // Create a font face from the memory buffer
    if (FT_New_Memory_Face(ft_library, g_font_buffer, g_font_buffer_size, 0, &g_ft_face) != 0) {
        fprintf(stderr, "Error: Could not create font face from memory\n");
        free(g_font_buffer);
        return -1;
    }
    
    /* Set a default pixel size (adjust 12 to your needs) */
    
    g_hb_font = hb_ft_font_create(g_ft_face, NULL);
    if (!g_hb_font) {
        fprintf(stderr, "Error: Could not create HarfBuzz font\n");
        exit(EXIT_FAILURE);
    }
    if (g_ft_face) {
        // Create a Cairo font face from your loaded FT_Face (no fallback)
        g_cairo_face = cairo_ft_font_face_create_for_ft_face(g_ft_face, 0);
    }
    
    /* Retrieve extra character spacing factor from the environment */
    g_extra_char_spacing_factor = gdiplus_get_extra_char_spacing();
    // Create a cairo font face from the FT_Face.
// The second parameter is flags; usually 0 is fine.
cairo_font_face_t *cairo_face = cairo_ft_font_face_create_for_ft_face(g_ft_face, 0);

// Set the font face on your cairo context (cr)
//cairo_set_font_face(cr, cairo_face);

// Optionally also set the font size if needed:
//cairo_set_font_size(cr, desired_font_size);
    // Optionally override the default pixel size using the "GDIPLUS_FONT_SIZE" environment variable.
    const char *env_font_size = getenv("GDIPLUS_FONT_SIZE");
    int pixel_size = 12;  // Default value.
    if (env_font_size && env_font_size[0] != '\0')
    {
        pixel_size = atoi(env_font_size);
        if (pixel_size <= 0)
            pixel_size = 12;
    }
    float desiredSize = pixel_size;
    if (FT_Set_Pixel_Sizes(g_ft_face, 0, pixel_size)) {
        fprintf(stderr, "Error: Could not set pixel size on the font face to %d\n", pixel_size);
        exit(EXIT_FAILURE);
    }
    g_default_font_size = pixel_size;

    
    g_text_shaping_initialized = 1;
    
    printf("Initialized text shaping.\n");
    printf("Using font: %s\n", font_path);
    printf("Using extra char spacing factor: %f\n", g_extra_char_spacing_factor);
}

/**
 * cleanup_text_shaping
 *
 * Cleans up FreeType and HarfBuzz resources allocated by init_text_shaping.
 * Resets the initialization flag so that initialization can be performed again,
 * if needed.
 */
static inline void cleanup_text_shaping(void)
{
    if (g_hb_font) {
        hb_font_destroy(g_hb_font);
        g_hb_font = NULL;
    }
    if (g_ft_face) {
        FT_Done_Face(g_ft_face);
        g_ft_face = NULL;
    }
    if (ft_library) {
        FT_Done_FreeType(ft_library);
        ft_library = NULL;
    }
    g_text_shaping_initialized = 0;
}

/* 
 * If your version of Cairo doesn't have cairo_get_font_size(), supply a fallback.
 * This fallback assumes the font matrix scaling is uniform.
 */
#ifndef HAVE_CAIRO_GET_FONT_SIZE
static inline double cairo_get_font_size(cairo_t *cr)
{
    cairo_matrix_t matrix;
    cairo_get_font_matrix(cr, &matrix);
    return matrix.xx;  /* Assumes uniform scaling (matrix.xx equals font size) */
}
#endif

/**
 * RenderShapedText
 *
 * Shapes the given UTF-8 text with HarfBuzz and renders the resulting glyphs with Cairo,
 * applying an extra character spacing.
 *
 * The extra spacing (in device units) for each glyph is computed as:
 *     extra_spacing = current_font_size * g_extra_char_spacing_factor
 *
 * Kerning is enabled by activating the 'kern' feature.
 *
 * Parameters:
 *   - ct:         Cairo context used for drawing.
 *   - text:       UTF-8 encoded text to render.
 *   - hb_font:    HarfBuzz font (should be initialized via init_text_shaping).
 *   - direction:  Text direction (e.g., HB_DIRECTION_LTR or HB_DIRECTION_TTB).
 *   - startX,
 *     startY:     Starting coordinates for rendering.
 */
static inline void RenderShapedText(cairo_t *ct, const char *text, hb_font_t *hb_font,
                                    hb_direction_t direction, double startX, double startY, double FontSize)
{
    // Initialize text shaping and set the Cairo font face.
    init_text_shaping();
    init_hb_languages();
    const char *font_path = gdiplus_get_font_path();
    const char *env_font_size = getenv("GDIPLUS_FONT_SIZE");
FT_Face      l_ft_face = NULL;
	double l_default_font_size = 12.0;
	//cairo_font_face_t *l_cairo_face = NULL;
hb_font_t   *l_hb_font = NULL;
    /*if (FT_New_Memory_Face(ft_library, g_font_buffer, g_font_buffer_size, 0, &l_ft_face) != 0) {
        fprintf(stderr, "Error: Could not load font face from %s\n @4", font_path);
        exit(EXIT_FAILURE);
    }*/
    const char *l_env_font_size = getenv("GDIPLUS_FONT_SIZE");
    int l_pixel_size = 12;  // Default value.
    if (env_font_size && env_font_size[0] != '\0')
    {
        l_pixel_size = atoi(env_font_size);
        if (l_pixel_size <= 0)
            l_pixel_size = 12;
    }
    float desiredSizeF = l_pixel_size * FontSize/12.0;
    int desiredSize = (int)desiredSizeF;
#if dbgHbCrFt
        fprintf(stderr, "TextRenderer: attempt to set to %f\n", desiredSize);
#endif
    if (FT_Set_Pixel_Sizes(g_ft_face, 0, desiredSize)) {
        fprintf(stderr, "Error: Could not set pixel size on the font face to %d\n", l_pixel_size);
        exit(EXIT_FAILURE);
    }
    l_default_font_size = l_pixel_size;
    l_hb_font = hb_ft_font_create(g_ft_face, NULL);
    if (!l_hb_font) {
        fprintf(stderr, "Error: Could not create HarfBuzz font\n");
        exit(EXIT_FAILURE);
    }
	cairo_font_face_t *l_cairo_face = NULL;
        l_cairo_face = cairo_ft_font_face_create_for_ft_face(g_ft_face, 0);
    cairo_set_font_face(ct, l_cairo_face);
    cairo_set_font_size(ct, desiredSize);

    // Create a HarfBuzz buffer and shape the text.
    hb_buffer_t *buf = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buf, hb_icu_get_unicode_funcs());
    hb_buffer_set_direction(buf, direction);
    hb_buffer_set_language(buf, g_hb_language);
    hb_buffer_set_script(buf, g_hb_script);

    hb_buffer_add_utf8(buf, text, -1, 0, -1);
    hb_feature_t features[] = { { HB_TAG('k','e','r','n'), 1, 0, (unsigned int)-1 } };
    hb_shape(l_hb_font, buf, features, 1);

    // Get glyph and position data.
    unsigned int glyph_count = 0;
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    // Build and render the Cairo glyph array.
    cairo_glyph_t *glyphs = malloc(glyph_count * sizeof(cairo_glyph_t));
    double x = startX, y = startY;
    for (unsigned int i = 0; i < glyph_count; i++) {
        glyphs[i].index = glyph_info[i].codepoint;
        glyphs[i].x = x + glyph_pos[i].x_offset / 64.0;
        glyphs[i].y = y - glyph_pos[i].y_offset / 64.0;
        x += glyph_pos[i].x_advance / 64.0;
    }
    cairo_show_glyphs(ct, glyphs, glyph_count);
    free(glyphs);

    hb_buffer_destroy(buf);
    
    if (l_hb_font) {
        hb_font_destroy(l_hb_font);
        l_hb_font = NULL;
    }
    /*
    if (l_ft_face) {
        FT_Done_Face(l_ft_face);
        l_ft_face = NULL;
    }*/
}


#ifdef __cplusplus
}
#endif

#endif /* HARFBUZZ_PRIVATE_H */

