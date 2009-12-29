/* gui.h - GUI components header file. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/types.h>
#include <grub/err.h>
#include <grub/video.h>
#include <grub/bitmap.h>
#include <grub/gfxmenu_view.h>

#ifndef GRUB_GUI_H
#define GRUB_GUI_H 1

/* A representation of a color.  Unlike grub_video_color_t, this
   representation is independent of any video mode specifics.  */
typedef struct grub_gui_color
{
  grub_uint8_t red;
  grub_uint8_t green;
  grub_uint8_t blue;
  grub_uint8_t alpha;
} grub_gui_color_t;

typedef struct grub_gui_component *grub_gui_component_t;
typedef struct grub_gui_container *grub_gui_container_t;
typedef struct grub_gui_list *grub_gui_list_t;

typedef void (*grub_gui_component_callback) (grub_gui_component_t component,
                                             void *userdata);

/* Component interface.  */

struct grub_gui_component_ops
{
  void (*destroy) (void *self);
  const char * (*get_id) (void *self);
  int (*is_instance) (void *self, const char *type);
  void (*paint) (void *self, const grub_video_rect_t *bounds);
  void (*set_parent) (void *self, grub_gui_container_t parent);
  grub_gui_container_t (*get_parent) (void *self);
  void (*set_bounds) (void *self, const grub_video_rect_t *bounds);
  void (*get_bounds) (void *self, grub_video_rect_t *bounds);
  void (*get_minimal_size) (void *self, unsigned *width, unsigned *height);
  grub_err_t (*set_property) (void *self, const char *name, const char *value);
  void (*repaint) (void *self, int second_pass);
};

struct grub_gui_container_ops
{
  void (*add) (void *self, grub_gui_component_t comp);
  void (*remove) (void *self, grub_gui_component_t comp);
  void (*iterate_children) (void *self,
                            grub_gui_component_callback cb, void *userdata);
};

struct grub_gui_list_ops
{
  void (*set_view_info) (void *self,
                         grub_gfxmenu_view_t view);
};

typedef grub_uint32_t grub_fixed_unsigned_t;
#define GRUB_FIXED_1 0x10000

static inline unsigned
grub_fixed_ufu_divide (grub_uint32_t a, grub_fixed_unsigned_t b)
{
  return (a << 16) / b;
}

static inline grub_fixed_unsigned_t
grub_fixed_fuf_divide (grub_fixed_unsigned_t a, grub_uint32_t b)
{
  return a / b;
}

static inline unsigned
grub_fixed_ufu_multiply (grub_uint32_t a, grub_fixed_unsigned_t b)
{
  return (a * b) >> 16;
}

static inline unsigned
grub_fixed_to_unsigned (grub_fixed_unsigned_t in)
{
  return in >> 16;
}

static inline grub_fixed_unsigned_t
grub_unsigned_to_fixed (unsigned in)
{
  return in << 16;
}

struct grub_gui_component
{
  struct grub_gui_component_ops *ops;
  int isxfrac:1;
  int isyfrac:1;
  int iswfrac:1;
  int ishfrac:1;
  union {
    unsigned x;
    grub_fixed_unsigned_t xfrac;
  };
  union {
    unsigned y;
    grub_fixed_unsigned_t yfrac;
  };
  union {
    unsigned w;
    grub_fixed_unsigned_t wfrac;
  };
  union {
    unsigned h;
    grub_fixed_unsigned_t hfrac;
  };
};

struct grub_gui_container
{
  struct grub_gui_component component;
  struct grub_gui_container_ops *ops;
};

struct grub_gui_list
{
  struct grub_gui_component component;
  struct grub_gui_list_ops *ops;
};


/* Interfaces to concrete component classes.  */

grub_gui_container_t grub_gui_canvas_new (void);
grub_gui_container_t grub_gui_vbox_new (void);
grub_gui_container_t grub_gui_hbox_new (void);
grub_gui_component_t grub_gui_label_new (void);
grub_gui_component_t grub_gui_image_new (void);
grub_gui_component_t grub_gui_progress_bar_new (void);
grub_gui_component_t grub_gui_list_new (void);
grub_gui_component_t grub_gui_circular_progress_new (void);

/* Manipulation functions.  */

/* Visit all components with the specified ID.  */
void grub_gui_find_by_id (grub_gui_component_t root,
                          const char *id,
                          grub_gui_component_callback cb,
                          void *userdata);

/* Visit all components.  */
void grub_gui_iterate_recursively (grub_gui_component_t root,
                                   grub_gui_component_callback cb,
                                   void *userdata);

/* Helper functions.  */

static __inline void
grub_gui_save_viewport (grub_video_rect_t *r)
{
  grub_video_get_viewport ((unsigned *) &r->x,
                           (unsigned *) &r->y,
                           (unsigned *) &r->width,
                           (unsigned *) &r->height);
}

static __inline void
grub_gui_restore_viewport (const grub_video_rect_t *r)
{
  grub_video_set_viewport (r->x, r->y, r->width, r->height);
}

/* Set a new viewport relative the the current one, saving the current
   viewport in OLD so it can be later restored.  */
static __inline void
grub_gui_set_viewport (const grub_video_rect_t *r, grub_video_rect_t *old)
{
  grub_gui_save_viewport (old);
  grub_video_set_viewport (old->x + r->x,
                           old->y + r->y,
                           r->width,
                           r->height);
}

static __inline grub_gui_color_t
grub_gui_color_rgb (int r, int g, int b)
{
  grub_gui_color_t c;
  c.red = r;
  c.green = g;
  c.blue = b;
  c.alpha = 255;
  return c;
}

static __inline grub_video_color_t
grub_gui_map_color (grub_gui_color_t c)
{
  return grub_video_map_rgba (c.red, c.green, c.blue, c.alpha);
}

static inline int
grub_video_have_common_points (const grub_video_rect_t *a,
			       const grub_video_rect_t *b)
{
  if (!((a->x <= b->x && b->x <= a->x + a->width)
	|| (b->x <= a->x && a->x <= b->x + b->width)))
    return 0;
  if (!((a->y <= b->y && b->y <= a->y + a->height)
	|| (b->y <= a->y && a->y <= b->y + b->height)))
    return 0;
  return 1;
}

#endif /* ! GRUB_GUI_H */
