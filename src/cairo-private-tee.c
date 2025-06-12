#include "cairo-private-tee.h"
#include <stdlib.h>
#include "cairo-tee.h"

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
    fprintf(stderr, "Cairo: Xlib: %s\n", cairo_status_to_string(cairo_surface_status(primary)));
    const char *tee_filename = getenv("GDIPLUS_TEE_SVG_FILE");
    if (!tee_filename || tee_filename[0] == '\0') {
        return primary;
    }

#if defined(CAIRO_HAS_TEE_SURFACE)
    cairo_surface_t *tee = cairo_tee_surface_create(primary);
    fprintf(stderr, "Tee surface: attempted to create tee, (%d).\n", &tee);
    if (cairo_surface_get_type(tee) == CAIRO_SURFACE_TYPE_TEE)
    fprintf(stderr, "Tee surface: attempted to create tee, it is TYPE_TEE now.\n");
    cairo_surface_t *svg =
        cairo_svg_surface_create(tee_filename, width, height);
    cairo_tee_surface_add(tee, svg);
    fprintf(stderr, "SVG ADDED?.\n");
    if (!tee || cairo_surface_status(tee) != CAIRO_STATUS_SUCCESS) {
    fprintf(stderr, "Tee surface: attempted to create tee FAILED.\n");
        fprintf(stderr,
                "Warning: Tee surface creation failed (%s). Falling back to "
                "primary surface.\n",
                cairo_status_to_string(cairo_surface_status(tee)));
        return primary;
    }
    fprintf(stderr, "Tee surface created successfully.\n");
    if (cairo_surface_get_type(tee) != CAIRO_SURFACE_TYPE_TEE ||
        cairo_surface_status(tee) != CAIRO_STATUS_SUCCESS) {
        fprintf(
            stderr,
            "Tee surface creation failed; falling back to primary surface.\n");
        return primary;
    }
    /*cairo_surface_t *svg2 =
        cairo_svg_surface_create(tee_filename, width, height);
    if (cairo_surface_status(svg2) != CAIRO_STATUS_SUCCESS) {
        fprintf(
            stderr,
            "SVG surface creation failed; falling back to primary surface.\n");
        return primary;
    }*/
    //cairo_tee_surface_add(tee, svg2);

    return tee;
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
