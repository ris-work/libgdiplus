#ifndef HARFBUZZ_PRIVATE_2_H
#define HARFBUZZ_PRIVATE_2_H

#ifdef __cplusplus
extern "C" {
#endif

/* Standard headers */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* FreeType, Cairo, and HarfBuzz headers */
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cairo.h>
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-icu.h>
#include "harfbuzz-private.h"

/* Internal libgdiplus headers */
#include "graphics-private.h"   /* Defines GpGraphics (with graphics->ct) */
#include "stringformat.h"       /* Declares GdipStringFormatGetGenericDefault and GdipDeleteStringFormat */

/* Forward declarations for helper routines (provided elsewhere in libgdiplus) */
extern char *utf16_to_utf8(const unsigned short *input, int length);
extern void GdipFree(void *ptr);

/* Global variables used for text shaping.
   (These must be defined and initialized elsewhere in libgdiplus.)
*/
extern hb_font_t *hb_font;
extern double g_extra_char_spacing_factor;

/* 
 * The following macro controls the linkage of cairo_MeasureString.
 * In most translation units you can include this header normally.
 * In one (and only one) source file (for example, in a central implementation file)
 * define the macro HARFBUZZ_PRIVATE_2_IMPLEMENTATION *before* including this header.
 *
 * When HARFBUZZ_PRIVATE_2_IMPLEMENTATION is defined the function is defined with external
 * linkage (no inline, no static) so that an external symbol is emitted.
 * Otherwise, the function is provided as static inline.
 */

#ifdef HARFBUZZ_PRIVATE_2_IMPLEMENTATION

int cairo_MeasureString(
    GpGraphics           *graphics,
    const unsigned short *stringUnicode,
    int                   length,
    const void           *font,
    const RectF          *rc,
    const void           *format,
    RectF                *boundingBox,
    int                  *codepointsFitted,
    int                  *linesFilled)
{
	init_text_shaping();
    cairo_matrix_t SavedMatrix;
    GpStringFormat *fmt;
    char *utf8Text;
    int status = Ok;

    /* Convert UTF-16 input to UTF-8.
       (utf16_to_utf8 allocates memory; the result must be freed via GdipFree.)
    */
    utf8Text = utf16_to_utf8(stringUnicode, length);
    if (!utf8Text)
        return OutOfMemory;

    /* Use default string format if none provided */
    if (!format)
    {
        status = GdipStringFormatGetGenericDefault(&fmt);
        if (status != Ok) {
            GdipFree(utf8Text);
            return status;
        }
    }
    else
    {
        fmt = (GpStringFormat *)format;
    }

    /* Save current Cairo font matrix */
    cairo_get_font_matrix(graphics->ct, &SavedMatrix);

    /* Create a HarfBuzz buffer and shape the UTF-8 text */
    hb_buffer_t *buf = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buf, hb_icu_get_unicode_funcs());
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_add_utf8(buf, utf8Text, -1, 0, -1);

    hb_feature_t features[] = {
        { HB_TAG('k','e','r','n'), 1, 0, (unsigned int)-1 }
    };
    hb_shape(hb_font, buf, features, 1);

    unsigned int glyph_count = 0;
    /* We extract the glyph positions (advances, etc.) */
    hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    double totalAdvance = 0.0;
    double font_size = cairo_get_font_size(graphics->ct);
    double extra_spacing = font_size * g_extra_char_spacing_factor;
    for (unsigned int i = 0; i < glyph_count; i++) {
        totalAdvance += (glyph_pos[i].x_advance / 64.0) + extra_spacing;
    }

    cairo_font_extents_t fe;
    cairo_scaled_font_t *scaled = cairo_get_scaled_font(graphics->ct);
    cairo_scaled_font_extents(scaled, &fe);

    if (boundingBox)
    {
        boundingBox->X = 0;
        boundingBox->Y = 0;
        boundingBox->Width = totalAdvance;
        boundingBox->Height = fe.height;
    }
    if (codepointsFitted)
        *codepointsFitted = glyph_count;
    if (linesFilled)
        *linesFilled = 1;

    hb_buffer_destroy(buf);

    /* Restore original Cairo font matrix */
    cairo_set_font_matrix(graphics->ct, &SavedMatrix);

    GdipFree(utf8Text);
    if (!format)
        GdipDeleteStringFormat(fmt);

    return status;
}

#else  /* Use static inline version */

static inline int cairo_MeasureString(
    GpGraphics           *graphics,
    const unsigned short *stringUnicode,
    int                   length,
    const void           *font,
    const RectF          *rc,
    const void           *format,
    RectF                *boundingBox,
    int                  *codepointsFitted,
    int                  *linesFilled)
{
    cairo_matrix_t SavedMatrix;
    GpStringFormat *fmt;
    char *utf8Text;
    int status = Ok;

    utf8Text = utf16_to_utf8(stringUnicode, length);
    if (!utf8Text)
        return OutOfMemory;

    if (!format)
    {
        status = GdipStringFormatGetGenericDefault(&fmt);
        if (status != Ok) {
            GdipFree(utf8Text);
            return status;
        }
    }
    else
    {
        fmt = (GpStringFormat *)format;
    }

    cairo_get_font_matrix(graphics->ct, &SavedMatrix);

    hb_buffer_t *buf = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buf, hb_icu_get_unicode_funcs());
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_add_utf8(buf, utf8Text, -1, 0, -1);

    hb_feature_t features[] = {
        { HB_TAG('k','e','r','n'), 1, 0, (unsigned int)-1 }
    };
    hb_shape(hb_font, buf, features, 1);

    unsigned int glyph_count = 0;
    hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    double totalAdvance = 0.0;
    double font_size = cairo_get_font_size(graphics->ct);
    double extra_spacing = font_size * g_extra_char_spacing_factor;
    for (unsigned int i = 0; i < glyph_count; i++) {
        totalAdvance += (glyph_pos[i].x_advance / 64.0) + extra_spacing;
    }

    cairo_font_extents_t fe;
    cairo_scaled_font_t *scaled = cairo_get_scaled_font(graphics->ct);
    cairo_scaled_font_extents(scaled, &fe);

    if (boundingBox)
    {
        boundingBox->X = 0;
        boundingBox->Y = 0;
        boundingBox->Width = totalAdvance;
        boundingBox->Height = fe.height;
    }
    if (codepointsFitted)
        *codepointsFitted = glyph_count;
    if (linesFilled)
        *linesFilled = 1;

    hb_buffer_destroy(buf);

    cairo_set_font_matrix(graphics->ct, &SavedMatrix);

    GdipFree(utf8Text);
    if (!format)
        GdipDeleteStringFormat(fmt);

    return status;
}

#endif  /* HARFBUZZ_PRIVATE_2_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif

#endif /* HARFBUZZ_PRIVATE_2_H */

