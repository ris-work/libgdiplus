#ifndef CAIRO_PRIVATE_TEE_H
#define CAIRO_PRIVATE_TEE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <cairo-svg.h>

/*
 * Drop-in replacement for the Cairo tee surface functions.
 *
 * DO NOT ALTER THE POSITION OF THE ORIGINAL ARGUMENTS.
 * This header declares the functions expected by libgdiplus:
 *
 *   cairo_surface_t* tee_surface_create(Display* display, Drawable drawable,
 *                                       Visual* visual, int width, int height);
 *   void           tee_surface_add(cairo_surface_t* abstract_surface, cairo_surface_t* target);
 *   cairo_surface_t* tee_surface_index(cairo_surface_t* abstract_surface, unsigned int index);
 *   void           tee_surface_remove(cairo_surface_t* abstract_surface, cairo_surface_t* target);
 *   void           flush_tee_surface(cairo_surface_t* surface);
 */

cairo_surface_t* tee_surface_create(Display* display, Drawable drawable,
                                    Visual* visual, int width, int height);
void tee_surface_add(cairo_surface_t* abstract_surface, cairo_surface_t* target);
cairo_surface_t* tee_surface_index(cairo_surface_t* abstract_surface, unsigned int index);
void tee_surface_remove(cairo_surface_t* abstract_surface, cairo_surface_t* target);
void flush_tee_surface(cairo_surface_t* surface);

#ifdef __cplusplus
}
#endif

#endif /* CAIRO_PRIVATE_TEE_H */

