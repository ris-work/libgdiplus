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
extern hb_font_t     *hb_font;
extern hb_language_t  g_hb_language;
extern double         g_extra_char_spacing_factor;

/* Declaration for the text shaping initialization routine.
   This function should initialize FreeType, load the face, create hb_font,
   set up g_hb_language (possibly from the environment variable "GDIPLUS_HARFBUZZ_LANGUAGE"),
   etc. It should be safe (idempotent) to call this at the beginning of each measurement/rendering call.
*/
extern void init_text_shaping(void);

#ifdef HARFBUZZ_PRIVATE_2_IMPLEMENTATION

/*
 * External (non-inline) definition for cairo_MeasureString.
 */
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
    /* Initialize text shaping */
    init_text_shaping();

    cairo_matrix_t originalMatrix;
    cairo_get_font_matrix(graphics->ct, &originalMatrix);

    char *utf8Text = utf16_to_utf8(stringUnicode, length);
    if (!utf8Text)
        return OutOfMemory;

    GpStringFormat *fmt;
    int status = Ok;
    if (!format) {
        status = GdipStringFormatGetGenericDefault(&fmt);
        if (status != Ok) {
            GdipFree(utf8Text);
            return status;
        }
    } else {
        fmt = (GpStringFormat *)format;
    }
    
    /* Create the HarfBuzz buffer and shape the text */
    hb_buffer_t *buf = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buf, hb_icu_get_unicode_funcs());
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_language(buf, g_hb_language);
    hb_buffer_add_utf8(buf, utf8Text, -1, 0, -1);
    hb_feature_t features[] = { { HB_TAG('k','e','r','n'), 1, 0, (unsigned int)-1 } };
    hb_shape(hb_font, buf, features, 1);

    unsigned int glyph_count = 0;
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    /* Compute raw total glyph advance (A) and count visual clusters */
    double totalGlyphAdvance = 0.0;
    int num_clusters = 0;
    if (glyph_count > 0) {
        totalGlyphAdvance = glyph_pos[0].x_advance / 64.0;
        num_clusters = 1;
        int lastCluster = glyph_info[0].cluster;
        for (unsigned int i = 1; i < glyph_count; i++) {
            totalGlyphAdvance += glyph_pos[i].x_advance / 64.0;
            if ((int)glyph_info[i].cluster != lastCluster) {
                num_clusters++;
                lastCluster = glyph_info[i].cluster;
            }
        }
    }

    /* Compute extra spacing total (E) */
    double font_size = cairo_get_font_size(graphics->ct);
    double extra_spacing = font_size * g_extra_char_spacing_factor;
    double totalExtraSpacing = (num_clusters - 1) * extra_spacing;

    /* Compute effective scale so that:
         (effective_scale * A) + E = A    ==> effective_scale = (A - E) / A */
    double computed_scale = 1.0;
    if (totalGlyphAdvance > 0 && totalGlyphAdvance > totalExtraSpacing)
        computed_scale = (totalGlyphAdvance - totalExtraSpacing) / totalGlyphAdvance;
    
    /* Allow override via "GDIPLUS_HORIZONTAL_SCALE" */
    double effective_scale = computed_scale;
    {
        const char *scaleEnv = getenv("GDIPLUS_HORIZONTAL_SCALE");
        if (scaleEnv && scaleEnv[0] != '\0') {
            double env_scale = atof(scaleEnv);
            if (env_scale > 0)
                effective_scale = env_scale;
        }
    }
    /* Now apply a constant multiplier from "GDIPLUS_HORIZONTAL_SCALE_MULTIPLIER" */
    double multiplier = 1.0;
    {
        const char *multEnv = getenv("GDIPLUS_HORIZONTAL_SCALE_MULTIPLIER");
        if (multEnv && multEnv[0] != '\0') {
            double env_mult = atof(multEnv);
            if (env_mult > 0)
                multiplier = env_mult;
        }
    }
    effective_scale *= multiplier;

    /* Apply the computed horizontal scale to the cairo font matrix */
    cairo_matrix_t scaledMatrix = originalMatrix;
    scaledMatrix.xx *= effective_scale;
    cairo_set_font_matrix(graphics->ct, &scaledMatrix);

    /* The final measured width is:
             finalWidth = (effective_scale * A) + E
    */
    double finalWidth = (effective_scale * totalGlyphAdvance) + totalExtraSpacing;

    /* Obtain Cairo font extents and set the output bounding box */
    cairo_font_extents_t fe;
    cairo_scaled_font_t *scaled = cairo_get_scaled_font(graphics->ct);
    cairo_scaled_font_extents(scaled, &fe);

    if (boundingBox) {
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

    /* Restore original font matrix */
    cairo_set_font_matrix(graphics->ct, &originalMatrix);

    return status;
}

#else  /* Static inline version for translation units that do not need an external symbol */

static inline int cairo_MeasureString(
    GpGraphics            *graphics,
    const unsigned short  *stringUnicode,
    int                    length,
    const void            *font,
    const RectF           *rc,
    const void            *format,
    RectF                 *boundingBox,
    int                   *codepointsFitted,
    int                   *linesFilled)
{
    init_text_shaping();

    cairo_matrix_t originalMatrix;
    cairo_get_font_matrix(graphics->ct, &originalMatrix);

    char *utf8Text = utf16_to_utf8(stringUnicode, length);
    if (!utf8Text)
        return OutOfMemory;
    
    GpStringFormat *fmt;
    int status = Ok;
    if (!format) {
        status = GdipStringFormatGetGenericDefault(&fmt);
        if (status != Ok) {
            GdipFree(utf8Text);
            return status;
        }
    } else {
        fmt = (GpStringFormat *)format;
    }
    
    hb_buffer_t *buf = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buf, hb_icu_get_unicode_funcs());
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_language(buf, g_hb_language);
    hb_buffer_add_utf8(buf, utf8Text, -1, 0, -1);
    hb_feature_t features[] = { { HB_TAG('k','e','r','n'), 1, 0, (unsigned int)-1 } };
    hb_shape(hb_font, buf, features, 1);

    unsigned int glyph_count = 0;
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    double totalGlyphAdvance = 0.0;
    int num_clusters = 0;
    if (glyph_count > 0) {
        totalGlyphAdvance = glyph_pos[0].x_advance / 64.0;
        num_clusters = 1;
        int lastCluster = glyph_info[0].cluster;
        for (unsigned int i = 1; i < glyph_count; i++) {
            totalGlyphAdvance += glyph_pos[i].x_advance / 64.0;
            if ((int)glyph_info[i].cluster != lastCluster) {
                num_clusters++;
                lastCluster = glyph_info[i].cluster;
            }
        }
    }
    
    double font_size = cairo_get_font_size(graphics->ct);
    double extra_spacing = font_size * g_extra_char_spacing_factor;
    double totalExtraSpacing = (num_clusters - 1) * extra_spacing;

    double computed_scale = 1.0;
    if (totalGlyphAdvance > 0 && totalGlyphAdvance > totalExtraSpacing)
        computed_scale = (totalGlyphAdvance - totalExtraSpacing) / totalGlyphAdvance;
    
    double effective_scale = computed_scale;
    {
        const char *scaleEnv = getenv("GDIPLUS_HORIZONTAL_SCALE");
        if (scaleEnv && scaleEnv[0] != '\0') {
            double env_scale = atof(scaleEnv);
            if (env_scale > 0)
                effective_scale = env_scale;
        }
    }
    double multiplier = 1.0;
    {
        const char *multEnv = getenv("GDIPLUS_HORIZONTAL_SCALE_MULTIPLIER");
        if (multEnv && multEnv[0] != '\0') {
            double env_mult = atof(multEnv);
            if (env_mult > 0)
                multiplier = env_mult;
        }
    }
    effective_scale *= multiplier;

    cairo_matrix_t scaledMatrix = originalMatrix;
    scaledMatrix.xx *= effective_scale;
    cairo_set_font_matrix(graphics->ct, &scaledMatrix);

    double finalWidth = (effective_scale * totalGlyphAdvance) + totalExtraSpacing;

    cairo_font_extents_t fe;
    cairo_scaled_font_t *scaled = cairo_get_scaled_font(graphics->ct);
    cairo_scaled_font_extents(scaled, &fe);

    if (boundingBox) {
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

#endif  /* HARFBUZZ_PRIVATE_2_IMPLEMENTATION */

/* ---------------------------------------------------------------------------
   RenderShapedText: Renders the text using similar dynamic squishing logic.
--------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif
#endif  /* HARFBUZZ_PRIVATE_2_H */

