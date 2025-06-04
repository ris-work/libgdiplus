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

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-icu.h>

/* Global state variables (static to this translation unit) */
static hb_font_t   *hb_font = NULL;
static FT_Library   ft_library = NULL;
static FT_Face      ft_face = NULL;
static double       g_extra_char_spacing_factor = 0.15;
static int          g_text_shaping_initialized = 0;
hb_language_t g_hb_language;

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
    
    if (FT_New_Face(ft_library, font_path, 0, &ft_face)) {
        fprintf(stderr, "Error: Could not load font face from %s\n", font_path);
        exit(EXIT_FAILURE);
    }
    
    /* Set a default pixel size (adjust 12 to your needs) */
    if (FT_Set_Pixel_Sizes(ft_face, 0, 12)) {
        fprintf(stderr, "Error: Could not set pixel size on the font face\n");
        exit(EXIT_FAILURE);
    }
    
    hb_font = hb_ft_font_create(ft_face, NULL);
    if (!hb_font) {
        fprintf(stderr, "Error: Could not create HarfBuzz font\n");
        exit(EXIT_FAILURE);
    }
    
    /* Retrieve extra character spacing factor from the environment */
    g_extra_char_spacing_factor = gdiplus_get_extra_char_spacing();
    
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
                                    hb_direction_t direction, double startX, double startY)
{
    /* 1. Initialize text shaping (load fonts, set language, etc.).
       This call should be safe to call repeatedly.
    */
    init_text_shaping();

    /* 2. Save the current Cairo font matrix so that we can restore it later.
       We will modify the horizontal scaling factor here.
    */
    cairo_matrix_t originalMatrix;
    cairo_get_font_matrix(ct, &originalMatrix);

    /* 3. Create a HarfBuzz buffer and shape the text.
       We set the language and direction, then add the UTF-8 text.
    */
    hb_buffer_t *buf = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buf, hb_icu_get_unicode_funcs());
    hb_buffer_set_direction(buf, direction);
    hb_buffer_set_language(buf, g_hb_language);
    hb_buffer_add_utf8(buf, text, -1, 0, -1);

    hb_feature_t features[] = { { HB_TAG('k','e','r','n'), 1, 0, (unsigned int)-1 } };
    hb_shape(hb_font, buf, features, 1);

    /* 4. Retrieve the glyph information and positions from HarfBuzz */
    unsigned int glyph_count = 0;
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    /* 5. Compute raw total glyph advance (A) and count visual clusters.
       - We sum the x_advance values (converted from 26.6 fixed point) for all glyphs
         to obtain the raw advance A.
       - Consecutive glyphs with the same cluster form one visual unit.
         The number of gaps between clusters is (num_clusters - 1).
    */
    double totalGlyphAdvance = 0.0;
    int num_clusters = 0;
    if (glyph_count > 0) {
        totalGlyphAdvance = glyph_pos[0].x_advance / 64.0;
        num_clusters = 1;
        for (unsigned int i = 1; i < glyph_count; i++) {
            totalGlyphAdvance += glyph_pos[i].x_advance / 64.0;
            if ((int)glyph_info[i].cluster != (int)glyph_info[i - 1].cluster)
                num_clusters++;
        }
    }

    /* 6. Compute the total extra spacing to be added between clusters (E):
         Each gap (there are num_clusters - 1) gets an extra spacing value.
         This extra spacing is based on the current font size and a configurable factor.
    */
    double font_size = cairo_get_font_size(ct);
    double extra_spacing = font_size * g_extra_char_spacing_factor;
    double totalExtraSpacing = (num_clusters - 1) * extra_spacing;

    /* 7. Compute the effective horizontal scaling factor.
         Without adjustment, if you add extra spacing E to the scaled glyph advances,
         the overall width would be:
             finalWidth = (effective_scale * A) + E.
         To preserve the original width (A) so that caret positioning remains unchanged,
         we want:
             (effective_scale * A) + E = A.
         Thus, effective_scale = (A - E) / A.
         (If A is zero or E is larger than A, we simply use a scale of 1.)
    */
    double computed_scale = 1.0;
    if (totalGlyphAdvance > 0 && totalGlyphAdvance > totalExtraSpacing)
        computed_scale = (totalGlyphAdvance - totalExtraSpacing) / totalGlyphAdvance;

    /* 8. Allow overrides and constant adjustment.
         The user can override the computed scale via the "GDIPLUS_HORIZONTAL_SCALE" environment variable.
         Additionally, a constant multiplier from "GDIPLUS_HORIZONTAL_SCALE_MULTIPLIER" can fine-tune the output.
    */
    double effective_scale = computed_scale;
    const char *scaleEnv = getenv("GDIPLUS_HORIZONTAL_SCALE");
    if (scaleEnv && scaleEnv[0] != '\0') {
        double env_scale = atof(scaleEnv);
        if (env_scale > 0)
            effective_scale = env_scale;
    }

    double multiplier = 1.0;
    const char *multEnv = getenv("GDIPLUS_HORIZONTAL_SCALE_MULTIPLIER");
    if (multEnv && multEnv[0] != '\0') {
        double env_mult = atof(multEnv);
        if (env_mult > 0)
            multiplier = env_mult;
    }
    effective_scale *= multiplier;

    /* 9. Apply the computed horizontal scale by adjusting the font matrix.
         This "squishes" the text horizontally so that when we add extra spacing,
         the overall width remains equal to the raw width A.
    */
    cairo_matrix_t scaledMatrix = originalMatrix;
    scaledMatrix.xx *= effective_scale;
    cairo_set_font_matrix(ct, &scaledMatrix);

    /* 10. Build the Cairo glyph array.
         For each glyph, the new x position is incremented by the scaled advance.
         Extra spacing is added only when we cross from one cluster to the next.
    */
    cairo_glyph_t *cairo_glyphs = (cairo_glyph_t *)malloc(glyph_count * sizeof(cairo_glyph_t));
    double x = startX, y = startY;
    for (unsigned int i = 0; i < glyph_count; i++) {
        cairo_glyphs[i].index = glyph_info[i].codepoint;
        cairo_glyphs[i].x = x + glyph_pos[i].x_offset / 64.0;
        cairo_glyphs[i].y = y - glyph_pos[i].y_offset / 64.0;

        double advance = glyph_pos[i].x_advance / 64.0;
        x += effective_scale * advance;

        /* Add the extra spacing only when a new visual cluster starts */
        if (i + 1 < glyph_count) {
            if ((int)glyph_info[i + 1].cluster != (int)glyph_info[i].cluster)
                x += extra_spacing;
        }
    }
    cairo_show_glyphs(ct, cairo_glyphs, glyph_count);
    free(cairo_glyphs);

    /* 11. Clean up: Destroy the HarfBuzz buffer and restore the original font matrix */
    hb_buffer_destroy(buf);
    cairo_set_font_matrix(ct, &originalMatrix);
}


#ifdef __cplusplus
}
#endif

#endif /* HARFBUZZ_PRIVATE_H */

