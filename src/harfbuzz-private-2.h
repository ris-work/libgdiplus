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

/* Internal libgdiplus headers */
#include "graphics-private.h"   /* Provides GpGraphics, RectF, etc. */
#include "stringformat.h"       /* Declares GdipStringFormatGetGenericDefault(), GdipDeleteStringFormat() */

/* Forward declarations for helper routines (provided elsewhere in libgdiplus) */
extern char *utf16_to_utf8(const unsigned short *input, int length);
extern void GdipFree(void *ptr);

/* Global variables used for HarfBuzz shaping:
   These must be defined and initialized elsewhere in libgdiplus.
*/
extern hb_font_t     *g_hb_font;
extern hb_language_t  g_hb_language;
extern double         g_extra_char_spacing_factor;

/* Declaration for the text shaping initialization routine.
   This function should initialize FreeType, load the face, create hb_font,
   set up g_hb_language (possibly from the environment variable "GDIPLUS_HARFBUZZ_LANGUAGE"),
   etc. It should be safe (idempotent) to call this at the beginning of each measurement/rendering call.
*/
extern void init_text_shaping(void);

#ifdef HARFBUZZ_PRIVATE_2_IMPLEMENTATION

int cairo_MeasureString(
    GpGraphics            *graphics,
    const unsigned short  *stringUnicode,
    int                    length,
    const void            *font,       /* Ignored in this implementation */
    const RectF           *rc,         /* Ignored in this implementation */
    const void            *format,
    RectF                 *boundingBox,
    int                   *codepointsFitted,
    int                   *linesFilled)
{
    /* 1. Initialize text shaping and set the Cairo font face. */
    init_text_shaping();
    cairo_set_font_face(graphics->ct, g_cairo_face);

    cairo_matrix_t originalMatrix;
    cairo_get_font_matrix(graphics->ct, &originalMatrix);

    /* 2. Convert the input UTF-16 string to UTF-8. */
    char *utf8Text = utf16_to_utf8(stringUnicode, length);
    if (!utf8Text)
        return OutOfMemory;

    GpStringFormat *fmt;
    int status = Ok;
    if (!format)
    {
        status = GdipStringFormatGetGenericDefault(&fmt);
        if (status != Ok)
        {
            GdipFree(utf8Text);
            return status;
        }
    }
    else
    {
        fmt = (GpStringFormat *)format;
    }

    /* 3. Create the HarfBuzz buffer, set properties, and shape the text. */
    hb_buffer_t *buf = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buf, hb_icu_get_unicode_funcs());
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_language(buf, g_hb_language);
    hb_buffer_add_utf8(buf, utf8Text, -1, 0, -1);
    hb_feature_t features[] = { { HB_TAG('k','e','r','n'), 1, 0, (unsigned int)-1 } };
    hb_shape(g_hb_font, buf, features, 1);

    /* 4. Retrieve glyph information and compute the total advance width. */
    unsigned int glyph_count = 0;
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    double finalWidth = 0.0;
    for (unsigned int i = 0; i < glyph_count; i++)
    {
        finalWidth += glyph_pos[i].x_advance / 64.0;
    }

    /* 5. Obtain Cairo font extents for height information. */
    cairo_font_extents_t fe;
    cairo_scaled_font_t *scaled = cairo_get_scaled_font(graphics->ct);
    cairo_scaled_font_extents(scaled, &fe);

    /* 6. Set the measured bounding box and codepoints/lines info. */
    if (boundingBox)
    {
        boundingBox->X = 0;
        boundingBox->Y = 0;
        boundingBox->Width = finalWidth;
        boundingBox->Height = fe.height;
    }
    if (codepointsFitted)
        *codepointsFitted = glyph_count;
    if (linesFilled)
        *linesFilled = 1;

    /* 7. Clean up and restore the original font matrix. */
    hb_buffer_destroy(buf);
    GdipFree(utf8Text);
    if (!format)
        GdipDeleteStringFormat(fmt);

    cairo_set_font_matrix(graphics->ct, &originalMatrix);
    return status;
}

#else  /* Static inline version for translation units that do not need an external symbol */

static inline int cairo_MeasureString(
    GpGraphics            *graphics,
    const unsigned short  *stringUnicode,
    int                    length,
    const void            *font,       /* Ignored in this implementation */
    const RectF           *rc,         /* Ignored in this implementation */
    const void            *format,
    RectF                 *boundingBox,
    int                   *codepointsFitted,
    int                   *linesFilled)
{
    init_text_shaping();
    cairo_set_font_face(graphics->ct, g_cairo_face);

    cairo_matrix_t originalMatrix;
    cairo_get_font_matrix(graphics->ct, &originalMatrix);

    char *utf8Text = utf16_to_utf8(stringUnicode, length);
    if (!utf8Text)
        return OutOfMemory;

    GpStringFormat *fmt;
    int status = Ok;
    if (!format)
    {
        status = GdipStringFormatGetGenericDefault(&fmt);
        if (status != Ok)
        {
            GdipFree(utf8Text);
            return status;
        }
    }
    else
    {
        fmt = (GpStringFormat *)format;
    }

    hb_buffer_t *buf = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buf, hb_icu_get_unicode_funcs());
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_language(buf, g_hb_language);
    hb_buffer_add_utf8(buf, utf8Text, -1, 0, -1);
    hb_feature_t features[] = { { HB_TAG('k','e','r','n'), 1, 0, (unsigned int)-1 } };
    hb_shape(g_hb_font, buf, features, 1);

    unsigned int glyph_count = 0;
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    double finalWidth = 0.0;
    for (unsigned int i = 0; i < glyph_count; i++)
    {
        finalWidth += glyph_pos[i].x_advance / 64.0;
    }

    cairo_font_extents_t fe;
    cairo_scaled_font_t *scaled = cairo_get_scaled_font(graphics->ct);
    cairo_scaled_font_extents(scaled, &fe);

    if (boundingBox)
    {
        boundingBox->X = 0;
        boundingBox->Y = 0;
        boundingBox->Width = finalWidth;
        boundingBox->Height = fe.height;
    }
    if (codepointsFitted)
        *codepointsFitted = glyph_count;
    if (linesFilled)
        *linesFilled = 1;

    hb_buffer_destroy(buf);
    GdipFree(utf8Text);
    if (!format)
        GdipDeleteStringFormat(fmt);

    cairo_set_font_matrix(graphics->ct, &originalMatrix);
    return status;
}

#endif


/* ---------------------------------------------------------------------------
   RenderShapedText: Renders the text using similar dynamic squishing logic.
--------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif
#endif  /* HARFBUZZ_PRIVATE_2_H */

