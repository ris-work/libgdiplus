#include "cairo-private-tee.h"
#include "cairo-tee.h"
#include <stdlib.h>

#include <cairo.h>
#include <stdbool.h>
#include <cairo.h>
#include <stdio.h>

// Global variables that track whether the full-window surface is created
// and store its reference.
static bool g_window_surface_created = false;
static cairo_surface_t *g_window_surface = NULL;
static cairo_status_t patched_svg_write_callback(void *closure,
                                                 const unsigned char *data,
                                                 unsigned int length)
{
    /* Log the flushed SVG chunk to stderr.
       Depending on your patch, 'closure' can be used if you need extra context;
       otherwise, it can be ignored (or set to NULL). */
    if (fwrite(data, 1, length, stderr) != length)
        return CAIRO_STATUS_WRITE_ERROR;
    fflush(stderr);
    return CAIRO_STATUS_SUCCESS;
}

/*
 * Implements a drop-in replacement for the cairo tee surface functions.
 *
 * This function creates an Xlib surface as the primary target using the
 * provided arguments (Display, Drawable, Visual, width, height). It then checks
 * for the "GDIPLUS_TEE_SVG_FILE" environment variable.
 *
 * If that variable is set and nonempty, and if the build of Cairo includes tee
 * support (i.e. CAIRO_HAS_TEE_SURFACE is defined), the function creates a tee
 * surface from the primary surface and attaches an SVG surface (with the same
 * width/height) as the replica.
 *
 * Otherwise, it returns the primary surface unchanged.
 *
 * NOTE: The argument positions are _not_ altered.
 */
cairo_surface_t *tee_surface_create(Display *display, Drawable drawable,
                                    Visual *visual, int width, int height) {
    fprintf(stderr, "Tee surface: begin checks.\n");
    cairo_surface_t *primary =
        cairo_xlib_surface_create(display, drawable, visual, width, height);
    fprintf(stderr, "Cairo: Xlib: %s\n",
            cairo_status_to_string(cairo_surface_status(primary)));
    const char *tee_filename = getenv("GDIPLUS_TEE_SVG_FILE");
    if (!tee_filename || tee_filename[0] == '\0') {
        return primary;
    }

#if defined(CAIRO_HAS_TEE_SURFACE)
    // Only record the global surface if it's not already set.
    if (!g_window_surface_created) {
        if (cairo_xlib_surface_get_width(primary) < 800 ||
            cairo_xlib_surface_get_height(primary) < 600 ) {
            fprintf(stderr,
                    "[DEBUG] tee global: %p | status: %d | type: %d | content: "
                    "%d | dims: %dx%d\n",
                    (void *)primary, cairo_surface_status(primary),
                    (int)cairo_surface_get_type(primary),
                    (int)cairo_surface_get_content(primary),
                    cairo_xlib_surface_get_width(primary),
                    cairo_xlib_surface_get_height(primary));
            fprintf(stderr, "Surface too small, not cloning\n");
            return primary;
        }
        cairo_surface_t *tee = cairo_tee_surface_create(primary);
        fprintf(stderr, "Tee surface: attempted to create tee, (%d) (%d)x(%d).\n", &tee, width, height);
        if (cairo_surface_get_type(tee) == CAIRO_SURFACE_TYPE_TEE)
            fprintf(
                stderr,
                "Tee surface: attempted to create tee, it is TYPE_TEE now.\n");
        cairo_surface_t *svg =
            cairo_svg_surface_create(tee_filename, width, height);
        cairo_surface_t *svg_log =
            cairo_svg_surface_create_for_stream(patched_svg_write_callback, NULL, width, height);
        cairo_tee_surface_add(tee, svg_log);

        fprintf(stderr, "SVG ADDED?.\n");
        if (!tee || cairo_surface_status(tee) != CAIRO_STATUS_SUCCESS) {
            fprintf(stderr, "Tee surface: attempted to create tee FAILED.\n");
            fprintf(
                stderr,
                "Warning: Tee surface creation failed (%s). Falling back to "
                "primary surface.\n",
                cairo_status_to_string(cairo_surface_status(tee)));
            return primary;
        }
        fprintf(stderr, "Tee surface created successfully.\n");
        if (cairo_surface_get_type(tee) != CAIRO_SURFACE_TYPE_TEE ||
            cairo_surface_status(tee) != CAIRO_STATUS_SUCCESS) {
            fprintf(stderr, "Tee surface creation failed; falling back to "
                            "primary surface.\n");
            return primary;
        }
        g_window_surface_created = true;
        g_window_surface = svg;
        fprintf(stderr,
                "[DEBUG] tee global: %p | status: %d | type: %d | content: %d "
                "| dims: %dx%d\n",
                (void *)tee, cairo_surface_status(tee),
                (int)cairo_surface_get_type(tee),
                (int)cairo_surface_get_content(tee),
                cairo_xlib_surface_get_width(primary),
                cairo_xlib_surface_get_height(primary));
        fprintf(stderr, "[DEBUG] Full-window surface created: %p\n",
                (void *)g_window_surface);
        return tee;
    } else {
        fprintf(stderr,
                "[DEBUG] create_window_surface called but global surface "
                "already set: %p\n",
                (void *)g_window_surface);
        return primary;
    }
#else
    fprintf(stderr, "Tee surface not enabled :(.\n");
    /* Tee support not available: fall back to the primary surface */
    return primary;
#endif
}

void tee_surface_add(cairo_surface_t *abstract_surface,
                     cairo_surface_t *target) {
#if defined(CAIRO_HAS_TEE_SURFACE)
    cairo_tee_surface_add(abstract_surface, target);
#else
    (void)abstract_surface;
    (void)target;
#endif
}

cairo_surface_t *tee_surface_index(cairo_surface_t *abstract_surface,
                                   unsigned int index) {
#if defined(CAIRO_HAS_TEE_SURFACE)
    return cairo_tee_surface_index(abstract_surface, index);
#else
    return (index == 0) ? abstract_surface : NULL;
#endif
}

void tee_surface_remove(cairo_surface_t *abstract_surface,
                        cairo_surface_t *target) {
#if defined(CAIRO_HAS_TEE_SURFACE)
    cairo_tee_surface_remove(abstract_surface, target);
#else
    (void)abstract_surface;
    (void)target;
#endif
}

/*
 * Flushes the surface by calling cairo_surface_finish(), ensuring that any
 * pending drawing commands are written out to the underlying targets (for
 * example, finishing the SVG file).
 */
void flush_tee_surface(cairo_surface_t *surface) {
    cairo_surface_finish(surface);
}
