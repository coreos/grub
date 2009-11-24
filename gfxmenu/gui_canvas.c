/* gui_canvas.c - GUI container allowing manually placed components. */
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

#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/gui.h>
#include <grub/gui_string_util.h>

/* TODO Add layering so that components can be properly overlaid. */

struct component_node
{
  grub_gui_component_t component;
  struct component_node *next;
};

struct grub_gui_canvas
{
  struct grub_gui_container_ops *container;

  grub_gui_container_t parent;
  grub_video_rect_t bounds;
  char *id;
  int preferred_width;
  int preferred_height;
  /* Component list (dummy head node).  */
  struct component_node components;
};

typedef struct grub_gui_canvas *grub_gui_canvas_t;

static void
canvas_destroy (void *vself)
{
  grub_gui_canvas_t self = vself;
  struct component_node *cur;
  struct component_node *next;
  for (cur = self->components.next; cur; cur = next)
    {
      /* Copy the 'next' pointer, since we need it for the next iteration,
         and we're going to free the memory it is stored in.  */
      next = cur->next;
      /* Destroy the child component.  */
      cur->component->ops->destroy (cur->component);
      /* Free the linked list node.  */
      grub_free (cur);
    }
  grub_free (self);
}

static const char *
canvas_get_id (void *vself)
{
  grub_gui_canvas_t self = vself;
  return self->id;
}

static int
canvas_is_instance (void *vself __attribute__((unused)), const char *type)
{
  return (grub_strcmp (type, "component") == 0
          || grub_strcmp (type, "container") == 0);
}

static void
canvas_paint (void *vself, const grub_video_rect_t *region)
{
  grub_gui_canvas_t self = vself;
  struct component_node *cur;
  grub_video_rect_t vpsave;

  grub_gui_set_viewport (&self->bounds, &vpsave);
  for (cur = self->components.next; cur; cur = cur->next)
    {
      int pw;
      int ph;
      grub_video_rect_t r;
      grub_gui_component_t comp;

      comp = cur->component;

      /* Give the child its preferred size.  */
      comp->ops->get_preferred_size (comp, &pw, &ph);
      comp->ops->get_bounds (comp, &r);
      if (r.width != pw || r.height != ph)
        {
          r.width = pw;
          r.height = ph;
          comp->ops->set_bounds (comp, &r);
        }

      /* Paint the child.  */
      if (grub_video_have_common_points (region, &r))
	comp->ops->paint (comp, region);
    }
  grub_gui_restore_viewport (&vpsave);
}

static void
canvas_set_parent (void *vself, grub_gui_container_t parent)
{
  grub_gui_canvas_t self = vself;
  self->parent = parent;
}

static grub_gui_container_t
canvas_get_parent (void *vself)
{
  grub_gui_canvas_t self = vself;
  return self->parent;
}

static void
canvas_set_bounds (void *vself, const grub_video_rect_t *bounds)
{
  grub_gui_canvas_t self = vself;
  self->bounds = *bounds;
}

static void
canvas_get_bounds (void *vself, grub_video_rect_t *bounds)
{
  grub_gui_canvas_t self = vself;
  *bounds = self->bounds;
}

static void
canvas_get_preferred_size (void *vself, int *width, int *height)
{
  grub_gui_canvas_t self = vself;
  *width = 0;
  *height = 0;

  /* Allow preferred dimensions to override the empty dimensions.  */
  if (self->preferred_width >= 0)
    *width = self->preferred_width;
  if (self->preferred_height >= 0)
    *height = self->preferred_height;
}

static grub_err_t
canvas_set_property (void *vself, const char *name, const char *value)
{
  grub_gui_canvas_t self = vself;
  if (grub_strcmp (name, "id") == 0)
    {
      grub_free (self->id);
      if (value)
        {
          self->id = grub_strdup (value);
          if (! self->id)
            return grub_errno;
        }
      else
        self->id = 0;
    }
  else if (grub_strcmp (name, "preferred_size") == 0)
    {
      int w;
      int h;
      if (grub_gui_parse_2_tuple (value, &w, &h) != GRUB_ERR_NONE)
        return grub_errno;
      self->preferred_width = w;
      self->preferred_height = h;
    }
  return grub_errno;
}

static void
canvas_add (void *vself, grub_gui_component_t comp)
{
  grub_gui_canvas_t self = vself;
  struct component_node *node;
  node = grub_malloc (sizeof (*node));
  if (! node)
    return;   /* Note: probably should handle the error.  */
  node->component = comp;
  node->next = self->components.next;
  self->components.next = node;
  comp->ops->set_parent (comp, (grub_gui_container_t) self);
}

static void
canvas_remove (void *vself, grub_gui_component_t comp)
{
  grub_gui_canvas_t self = vself;
  struct component_node *cur;
  struct component_node *prev;
  prev = &self->components;
  for (cur = self->components.next; cur; prev = cur, cur = cur->next)
    {
      if (cur->component == comp)
        {
          /* Unlink 'cur' from the list.  */
          prev->next = cur->next;
          /* Free the node's memory (but don't destroy the component).  */
          grub_free (cur);
          /* Must not loop again, since 'cur' would be dereferenced!  */
          return;
        }
    }
}

static void
canvas_iterate_children (void *vself,
                         grub_gui_component_callback cb, void *userdata)
{
  grub_gui_canvas_t self = vself;
  struct component_node *cur;
  for (cur = self->components.next; cur; cur = cur->next)
    cb (cur->component, userdata);
}

static struct grub_gui_container_ops canvas_ops =
{
  .component =
    {
      .destroy = canvas_destroy,
      .get_id = canvas_get_id,
      .is_instance = canvas_is_instance,
      .paint = canvas_paint,
      .set_parent = canvas_set_parent,
      .get_parent = canvas_get_parent,
      .set_bounds = canvas_set_bounds,
      .get_bounds = canvas_get_bounds,
      .get_preferred_size = canvas_get_preferred_size,
      .set_property = canvas_set_property
    },
  .add = canvas_add,
  .remove = canvas_remove,
  .iterate_children = canvas_iterate_children
};

grub_gui_container_t
grub_gui_canvas_new (void)
{
  grub_gui_canvas_t canvas;
  canvas = grub_malloc (sizeof (*canvas));
  if (! canvas)
    return 0;
  canvas->container = &canvas_ops;
  canvas->parent = 0;
  canvas->bounds.x = 0;
  canvas->bounds.y = 0;
  canvas->bounds.width = 0;
  canvas->bounds.height = 0;
  canvas->id = 0;
  canvas->preferred_width = -1;
  canvas->preferred_height = -1;
  canvas->components.component = 0;
  canvas->components.next = 0;
  return (grub_gui_container_t) canvas;
}
