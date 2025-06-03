/* --- Global Declarations / Helper Functions --- */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <cairo.h>
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-icu.h>

/* Define PI if not already defined */
#ifndef PI
#define PI 3.14159265358979323846
#endif
#include <cairo.h>

/* Fallback: If your Cairo version doesn't provide cairo_get_font_size,
   define it using cairo_get_font_matrix. This assumes that the scaling
   is uniform (i.e., the x-scale is the font size).
*/
#ifndef HAVE_CAIRO_GET_FONT_SIZE
double cairo_get_font_size(cairo_t *cr)
{
    cairo_matrix_t matrix;
    cairo_get_font_matrix(cr, &matrix);
    return matrix.xx; /* Assumes uniform scaling (matrix.xx equals font size) */
}
#endif


/*
 * RenderShapedText:
 *
 * Shapes the given UTF-8 text using HarfBuzz (with the provided hb_font)
 * and renders the resulting glyphs on the Cairo context 'ct' starting at
 * (startX, startY). The 'direction' parameter should be HB_DIRECTION_LTR
 * for horizontal text or HB_DIRECTION_TTB for vertical text.
 *
 * IMPORTANT: Be sure that your Cairo context has already been set up with
 * the proper font face and size and that the hb_font has been created (for
 * example, via hb_ft_font_create() from your FreeType face) before calling
 * this function.
 */
void RenderShapedText(cairo_t *ct, const char *text, hb_font_t *hb_font,
                      hb_direction_t direction, double startX, double startY)
{
    /* Create and configure a HarfBuzz buffer */
    hb_buffer_t *buf = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buf, hb_icu_get_unicode_funcs());
    hb_buffer_set_direction(buf, direction);
    hb_buffer_add_utf8(buf, text, -1, 0, -1);

    /* Enable kerning explicitly */
    hb_feature_t features[] = {
        { HB_TAG('k','e','r','n'), 1, 0, (unsigned int)-1 }
    };
    hb_shape(hb_font, buf, features, 1);

    unsigned int glyph_count = 0;
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    cairo_glyph_t *cairo_glyphs = malloc(glyph_count * sizeof(cairo_glyph_t));

    double x = startX;
    double y = startY;
    double font_size = cairo_get_font_size(ct);

    /* Extra spacing multiplier.
     * GDIPLUS_EXTRA_CHAR_SPACING_FACTOR is a unique constant you can adjust.
     * For example, if set to 0.1 and font_size is 12, an extra 1.2 pixels is added.
     */
    #define GDIPLUS_EXTRA_CHAR_SPACING_FACTOR 0.1
    double extra_spacing = font_size * GDIPLUS_EXTRA_CHAR_SPACING_FACTOR;

    for (unsigned int j = 0; j < glyph_count; j++) {
        cairo_glyphs[j].index = glyph_info[j].codepoint;
        cairo_glyphs[j].x = x + glyph_pos[j].x_offset / 64.0;
        cairo_glyphs[j].y = y - glyph_pos[j].y_offset / 64.0;
        x += glyph_pos[j].x_advance / 64.0 + extra_spacing;
        y -= glyph_pos[j].y_advance / 64.0;
    }

    cairo_show_glyphs(ct, cairo_glyphs, glyph_count);
    free(cairo_glyphs);
    hb_buffer_destroy(buf);
}

