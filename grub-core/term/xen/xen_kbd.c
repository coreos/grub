/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/term.h>
#include <grub/keyboard_layouts.h>
#include <grub/time.h>
#include <grub/loader.h>
#include <grub/xen.h>
#include <xen/io/kbdif.h>

GRUB_MOD_LICENSE ("GPLv3+");

static short xen_keyboard_status = 0;

static const grub_uint8_t mapping[128] =
  {
    /* 0x00 */ 0 /* Unused  */,               GRUB_KEYBOARD_KEY_ESCAPE, 
    /* 0x02 */ GRUB_KEYBOARD_KEY_1,           GRUB_KEYBOARD_KEY_2, 
    /* 0x04 */ GRUB_KEYBOARD_KEY_3,           GRUB_KEYBOARD_KEY_4, 
    /* 0x06 */ GRUB_KEYBOARD_KEY_5,           GRUB_KEYBOARD_KEY_6, 
    /* 0x08 */ GRUB_KEYBOARD_KEY_7,           GRUB_KEYBOARD_KEY_8, 
    /* 0x0a */ GRUB_KEYBOARD_KEY_9,           GRUB_KEYBOARD_KEY_0, 
    /* 0x0c */ GRUB_KEYBOARD_KEY_DASH,        GRUB_KEYBOARD_KEY_EQUAL, 
    /* 0x0e */ GRUB_KEYBOARD_KEY_BACKSPACE,   GRUB_KEYBOARD_KEY_TAB, 
    /* 0x10 */ GRUB_KEYBOARD_KEY_Q,           GRUB_KEYBOARD_KEY_W, 
    /* 0x12 */ GRUB_KEYBOARD_KEY_E,           GRUB_KEYBOARD_KEY_R, 
    /* 0x14 */ GRUB_KEYBOARD_KEY_T,           GRUB_KEYBOARD_KEY_Y, 
    /* 0x16 */ GRUB_KEYBOARD_KEY_U,           GRUB_KEYBOARD_KEY_I, 
    /* 0x18 */ GRUB_KEYBOARD_KEY_O,           GRUB_KEYBOARD_KEY_P, 
    /* 0x1a */ GRUB_KEYBOARD_KEY_LBRACKET,    GRUB_KEYBOARD_KEY_RBRACKET, 
    /* 0x1c */ GRUB_KEYBOARD_KEY_ENTER,       GRUB_KEYBOARD_KEY_LEFT_CTRL, 
    /* 0x1e */ GRUB_KEYBOARD_KEY_A,           GRUB_KEYBOARD_KEY_S, 
    /* 0x20 */ GRUB_KEYBOARD_KEY_D,           GRUB_KEYBOARD_KEY_F, 
    /* 0x22 */ GRUB_KEYBOARD_KEY_G,           GRUB_KEYBOARD_KEY_H, 
    /* 0x24 */ GRUB_KEYBOARD_KEY_J,           GRUB_KEYBOARD_KEY_K, 
    /* 0x26 */ GRUB_KEYBOARD_KEY_L,           GRUB_KEYBOARD_KEY_SEMICOLON, 
    /* 0x28 */ GRUB_KEYBOARD_KEY_DQUOTE,      GRUB_KEYBOARD_KEY_RQUOTE, 
    /* 0x2a */ GRUB_KEYBOARD_KEY_LEFT_SHIFT,  GRUB_KEYBOARD_KEY_BACKSLASH, 
    /* 0x2c */ GRUB_KEYBOARD_KEY_Z,           GRUB_KEYBOARD_KEY_X, 
    /* 0x2e */ GRUB_KEYBOARD_KEY_C,           GRUB_KEYBOARD_KEY_V, 
    /* 0x30 */ GRUB_KEYBOARD_KEY_B,           GRUB_KEYBOARD_KEY_N, 
    /* 0x32 */ GRUB_KEYBOARD_KEY_M,           GRUB_KEYBOARD_KEY_COMMA, 
    /* 0x34 */ GRUB_KEYBOARD_KEY_DOT,         GRUB_KEYBOARD_KEY_SLASH, 
    /* 0x36 */ GRUB_KEYBOARD_KEY_RIGHT_SHIFT, GRUB_KEYBOARD_KEY_NUMMUL, 
    /* 0x38 */ GRUB_KEYBOARD_KEY_LEFT_ALT,    GRUB_KEYBOARD_KEY_SPACE, 
    /* 0x3a */ GRUB_KEYBOARD_KEY_CAPS_LOCK,   GRUB_KEYBOARD_KEY_F1, 
    /* 0x3c */ GRUB_KEYBOARD_KEY_F2,          GRUB_KEYBOARD_KEY_F3, 
    /* 0x3e */ GRUB_KEYBOARD_KEY_F4,          GRUB_KEYBOARD_KEY_F5, 
    /* 0x40 */ GRUB_KEYBOARD_KEY_F6,          GRUB_KEYBOARD_KEY_F7, 
    /* 0x42 */ GRUB_KEYBOARD_KEY_F8,          GRUB_KEYBOARD_KEY_F9, 
    /* 0x44 */ GRUB_KEYBOARD_KEY_F10,         GRUB_KEYBOARD_KEY_NUM_LOCK, 
    /* 0x46 */ GRUB_KEYBOARD_KEY_SCROLL_LOCK, GRUB_KEYBOARD_KEY_NUM7, 
    /* 0x48 */ GRUB_KEYBOARD_KEY_NUM8,        GRUB_KEYBOARD_KEY_NUM9, 
    /* 0x4a */ GRUB_KEYBOARD_KEY_NUMMINUS,    GRUB_KEYBOARD_KEY_NUM4, 
    /* 0x4c */ GRUB_KEYBOARD_KEY_NUM5,        GRUB_KEYBOARD_KEY_NUM6, 
    /* 0x4e */ GRUB_KEYBOARD_KEY_NUMPLUS,     GRUB_KEYBOARD_KEY_NUM1, 
    /* 0x50 */ GRUB_KEYBOARD_KEY_NUM2,        GRUB_KEYBOARD_KEY_NUM3, 
    /* 0x52 */ GRUB_KEYBOARD_KEY_NUMDOT,      GRUB_KEYBOARD_KEY_NUMDOT, 
    /* 0x54 */ 0,                             0, 
    /* 0x56 */ GRUB_KEYBOARD_KEY_102ND,       GRUB_KEYBOARD_KEY_F11, 
    /* 0x58 */ GRUB_KEYBOARD_KEY_F12,         0,
    /* 0x5a */ 0,                             0,
    /* 0x5c */ 0,                             0,
    /* 0x5e */ 0,                             0,
    /* 0x60 */ 0,                             GRUB_KEYBOARD_KEY_RIGHT_CTRL,
    /* 0x62 */ 0,                             0,
    /* 0x64 */ GRUB_KEYBOARD_KEY_RIGHT_ALT,   0,
    /* 0x66 */ GRUB_KEYBOARD_KEY_HOME,        GRUB_KEYBOARD_KEY_UP,
    /* 0x68 */ GRUB_KEYBOARD_KEY_PPAGE,       GRUB_KEYBOARD_KEY_LEFT,
    /* 0x6a */ GRUB_KEYBOARD_KEY_RIGHT,       GRUB_KEYBOARD_KEY_END,
    /* 0x6c */ GRUB_KEYBOARD_KEY_DOWN,        GRUB_KEYBOARD_KEY_NPAGE,
    /* 0x6e */ GRUB_KEYBOARD_KEY_INSERT,      GRUB_KEYBOARD_KEY_DELETE,
    /* 0x70 */ 0,                             0,
    /* 0x72 */ 0,                             GRUB_KEYBOARD_KEY_JP_RO,
    /* 0x74 */ 0,                             0,
    /* 0x76 */ 0,                             0,
    /* 0x78 */ 0,                             0,
    /* 0x7a */ 0,                             0,
    /* 0x7c */ 0,                             GRUB_KEYBOARD_KEY_JP_YEN,
    /* 0x7e */ GRUB_KEYBOARD_KEY_KPCOMMA
  };

struct virtkbd
{
  int handle;
  char *fullname;
  char *backend_dir;
  char *frontend_dir;
  struct xenkbd_page *kbdpage;
  grub_xen_grant_t grant;
  grub_xen_evtchn_t evtchn;
};

struct virtkbd vkbd;

static grub_xen_mfn_t
grub_xen_ptr2mfn (void *ptr)
{
  grub_xen_mfn_t *mfn_list =
    (grub_xen_mfn_t *) grub_xen_start_page_addr->mfn_list;
  return mfn_list[(grub_addr_t) ptr >> GRUB_XEN_LOG_PAGE_SIZE];
}

static int
fill (const char *dir, void *data __attribute__ ((unused)))
{
  domid_t dom;
  /* "dir" is just a number, at most 19 characters. */
  char fdir[200];
  char num[20];
  grub_err_t err;
  void *buf;
  struct evtchn_alloc_unbound alloc_unbound;

  if (vkbd.kbdpage)
    return 1;
  vkbd.handle = grub_strtoul (dir, 0, 10);
  if (grub_errno)
    {
      grub_errno = 0;
      return 0;
    }
  vkbd.fullname = 0;
  vkbd.backend_dir = 0;

  grub_snprintf (fdir, sizeof (fdir), "device/vkbd/%s/backend", dir);
  vkbd.backend_dir = grub_xenstore_get_file (fdir, NULL);
  if (!vkbd.backend_dir)
    goto out_fail_1;

  grub_snprintf (fdir, sizeof (fdir), "%s/dev",
		 vkbd.backend_dir);

  grub_snprintf (fdir, sizeof (fdir), "device/vkbd/%s/backend-id", dir);
  buf = grub_xenstore_get_file (fdir, NULL);
  if (!buf)
    goto out_fail_1;

  dom = grub_strtoul (buf, 0, 10);
  grub_free (buf);
  if (grub_errno)
    goto out_fail_1;

  vkbd.kbdpage =
    grub_xen_alloc_shared_page (dom, &vkbd.grant);
  if (!vkbd.kbdpage)
    goto out_fail_1;

  grub_snprintf (fdir, sizeof (fdir), "device/vkbd/%s/page-ref", dir);
  grub_snprintf (num, sizeof (num), "%llu", (unsigned long long) grub_xen_ptr2mfn (vkbd.kbdpage));
  err = grub_xenstore_write_file (fdir, num, grub_strlen (num));
  if (err)
    goto out_fail_3;

  grub_snprintf (fdir, sizeof (fdir), "device/vkbd/%s/protocol", dir);
  err = grub_xenstore_write_file (fdir, XEN_IO_PROTO_ABI_NATIVE,
				  grub_strlen (XEN_IO_PROTO_ABI_NATIVE));
  if (err)
    goto out_fail_3;

  alloc_unbound.dom = DOMID_SELF;
  alloc_unbound.remote_dom = dom;

  grub_xen_event_channel_op (EVTCHNOP_alloc_unbound, &alloc_unbound);
  vkbd.evtchn = alloc_unbound.port;

  grub_snprintf (fdir, sizeof (fdir), "device/vkbd/%s/event-channel", dir);
  grub_snprintf (num, sizeof (num), "%u", vkbd.evtchn);
  err = grub_xenstore_write_file (fdir, num, grub_strlen (num));
  if (err)
    goto out_fail_3;


  struct gnttab_dump_table dt;
  dt.dom = DOMID_SELF;
  grub_xen_grant_table_op (GNTTABOP_dump_table, (void *) &dt, 1);

  grub_snprintf (fdir, sizeof (fdir), "device/vkbd/%s", dir);

  vkbd.frontend_dir = grub_strdup (fdir);

  vkbd.kbdpage->in_cons = 0;
  vkbd.kbdpage->in_prod = 0;
  vkbd.kbdpage->out_cons = 0;
  vkbd.kbdpage->out_prod = 0;

  grub_snprintf (fdir, sizeof (fdir), "device/vkbd/%s/state", dir);
  err = grub_xenstore_write_file (fdir, "3", 1);
  if (err)
    goto out_fail_3;

  while (1)
    {
      grub_snprintf (fdir, sizeof (fdir), "%s/state",
		     vkbd.backend_dir);
      buf = grub_xenstore_get_file (fdir, NULL);
      if (!buf)
	goto out_fail_3;
      if (grub_strcmp (buf, "2") != 0)
	break;
      grub_free (buf);
      grub_xen_sched_op (SCHEDOP_yield, 0);
    }
  grub_dprintf ("xen", "state=%s\n", (char *) buf);
  grub_free (buf);

  return 1;

out_fail_3:
  grub_xen_free_shared_page (vkbd.kbdpage);
out_fail_1:

  vkbd.kbdpage = 0;
  grub_free (vkbd.backend_dir);
  grub_free (vkbd.fullname);

  grub_errno = 0;
  return 0;
}

static void
vkbd_fini (void)
{
  char fdir[200];

  char *buf;
  struct evtchn_close close_op = {.port = vkbd.evtchn };

  if (!vkbd.kbdpage)
    return;

  grub_snprintf (fdir, sizeof (fdir), "%s/state",
		 vkbd.frontend_dir);
  grub_xenstore_write_file (fdir, "6", 1);

  while (1)
    {
      grub_snprintf (fdir, sizeof (fdir), "%s/state",
		     vkbd.backend_dir);
      buf = grub_xenstore_get_file (fdir, NULL);
      grub_dprintf ("xen", "state=%s\n", (char *) buf);

      if (!buf || grub_strcmp (buf, "6") == 0)
	break;
      grub_free (buf);
      grub_xen_sched_op (SCHEDOP_yield, 0);
    }
  grub_free (buf);

  grub_snprintf (fdir, sizeof (fdir), "%s/page-ref",
		 vkbd.frontend_dir);
  grub_xenstore_write_file (fdir, NULL, 0);

  grub_snprintf (fdir, sizeof (fdir), "%s/event-channel",
		 vkbd.frontend_dir);
  grub_xenstore_write_file (fdir, NULL, 0);

  grub_xen_free_shared_page (vkbd.kbdpage);
  vkbd.kbdpage = 0;

  grub_xen_event_channel_op (EVTCHNOP_close, &close_op);

  /* Prepare for handoff.  */
  grub_snprintf (fdir, sizeof (fdir), "%s/state",
		 vkbd.frontend_dir);
  grub_xenstore_write_file (fdir, "1", 1);
}

static grub_err_t
vkbd_init (void)
{
  if (vkbd.kbdpage)
    vkbd_fini ();
  grub_xenstore_dir ("device/vkbd", fill, NULL);
  if (vkbd.kbdpage)
    grub_errno = 0;
  if (!vkbd.kbdpage)
    return grub_error (GRUB_ERR_IO, "couldn't init vkbd");
  return GRUB_ERR_NONE;
}

static int
fetch_key (int *is_break)
{
  grub_uint32_t prod;

  if (!vkbd.kbdpage)
    {
      *is_break = 0;
      return -1;
    }

  mb ();

  prod = vkbd.kbdpage->in_prod;

  mb ();

  for (;vkbd.kbdpage->in_cons < prod; vkbd.kbdpage->in_cons++)
    {
      grub_uint32_t keycode;
      int ret = 0;
      if (XENKBD_IN_RING_REF(vkbd.kbdpage, vkbd.kbdpage->in_cons).type != XENKBD_TYPE_KEY)
	continue;
      keycode = XENKBD_IN_RING_REF(vkbd.kbdpage, vkbd.kbdpage->in_cons).key.keycode;
      *is_break = !XENKBD_IN_RING_REF(vkbd.kbdpage, vkbd.kbdpage->in_cons).key.pressed;

      if (keycode < ARRAY_SIZE (mapping))
	ret = mapping[keycode];
      if (ret == 0)
	{
	  grub_printf ("unknown keycode = %lx\n", (unsigned long) keycode);
	  continue;
	}
      vkbd.kbdpage->in_cons++;
      return ret;
    }
  *is_break = 0;
  return -1;
}

static int
grub_keyboard_isr (grub_keyboard_key_t key, int is_break)
{
  if (!is_break)
    switch (key)
      {
      case GRUB_KEYBOARD_KEY_LEFT_SHIFT:
	xen_keyboard_status |= GRUB_TERM_STATUS_LSHIFT;
	return 1;
      case GRUB_KEYBOARD_KEY_RIGHT_SHIFT:
	xen_keyboard_status |= GRUB_TERM_STATUS_RSHIFT;
	return 1;
      case GRUB_KEYBOARD_KEY_LEFT_CTRL:
	xen_keyboard_status |= GRUB_TERM_STATUS_LCTRL;
	return 1;
      case GRUB_KEYBOARD_KEY_RIGHT_CTRL:
	xen_keyboard_status |= GRUB_TERM_STATUS_RCTRL;
	return 1;
      case GRUB_KEYBOARD_KEY_RIGHT_ALT:
	xen_keyboard_status |= GRUB_TERM_STATUS_RALT;
	return 1;
      case GRUB_KEYBOARD_KEY_LEFT_ALT:
	xen_keyboard_status |= GRUB_TERM_STATUS_LALT;
	return 1;
      default:
	return 0;
      }
  else
    switch (key)
      {
      case GRUB_KEYBOARD_KEY_LEFT_SHIFT:
	xen_keyboard_status &= ~GRUB_TERM_STATUS_LSHIFT;
	return 1;
      case GRUB_KEYBOARD_KEY_RIGHT_SHIFT:
	xen_keyboard_status &= ~GRUB_TERM_STATUS_RSHIFT;
	return 1;
      case GRUB_KEYBOARD_KEY_LEFT_CTRL:
	xen_keyboard_status &= ~GRUB_TERM_STATUS_LCTRL;
	return 1;
      case GRUB_KEYBOARD_KEY_RIGHT_CTRL:
	xen_keyboard_status &= ~GRUB_TERM_STATUS_RCTRL;
	return 1;
      case GRUB_KEYBOARD_KEY_RIGHT_ALT:
	xen_keyboard_status &= ~GRUB_TERM_STATUS_RALT;
	return 1;
      case GRUB_KEYBOARD_KEY_LEFT_ALT:
	xen_keyboard_status &= ~GRUB_TERM_STATUS_LALT;
	return 1;
      default:
	return 0;
      }
}

/* If there is a raw key pending, return it; otherwise return -1.  */
static int
grub_keyboard_getkey (void)
{
  int key;
  int is_break = 0;

  key = fetch_key (&is_break);
  if (key == -1)
    return -1;

  if (grub_keyboard_isr (key, is_break))
    return -1;
  if (is_break)
    return -1;
  return key;
}


/* If there is a character pending, return it;
   otherwise return GRUB_TERM_NO_KEY.  */
static int
grub_xen_keyboard_getkey (struct grub_term_input *term __attribute__ ((unused)))
{
  int code;

  code = grub_keyboard_getkey ();
  if (code == -1)
    return GRUB_TERM_NO_KEY;
  switch (code)
    {
      case GRUB_KEYBOARD_KEY_CAPS_LOCK:
	xen_keyboard_status ^= GRUB_TERM_STATUS_CAPS;
	return GRUB_TERM_NO_KEY;
      case GRUB_KEYBOARD_KEY_NUM_LOCK:
	xen_keyboard_status ^= GRUB_TERM_STATUS_NUM;
	return GRUB_TERM_NO_KEY;
      case GRUB_KEYBOARD_KEY_SCROLL_LOCK:
	xen_keyboard_status ^= GRUB_TERM_STATUS_SCROLL;
	return GRUB_TERM_NO_KEY;
      default:
	return grub_term_map_key (code, xen_keyboard_status);
    }
}

static grub_err_t
grub_keyboard_controller_init (struct grub_term_input *term __attribute__ ((unused)))
{
  vkbd_init ();
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_keyboard_controller_fini (struct grub_term_input *term __attribute__ ((unused)))
{
  vkbd_fini ();
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_xen_fini_hw (int noreturn __attribute__ ((unused)))
{
  vkbd_fini ();
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_xen_restore_hw (void)
{
  vkbd_init ();
  return GRUB_ERR_NONE;
}


static struct grub_term_input grub_xen_keyboard_term =
  {
    .name = "xen_keyboard",
    .init = grub_keyboard_controller_init,
    .fini = grub_keyboard_controller_fini,
    .getkey = grub_xen_keyboard_getkey
  };

void
grub_xen_keyboard_init (void)
{
  grub_term_register_input ("xen_keyboard", &grub_xen_keyboard_term);
  grub_loader_register_preboot_hook (grub_xen_fini_hw, grub_xen_restore_hw,
				     GRUB_LOADER_PREBOOT_HOOK_PRIO_CONSOLE);
}

GRUB_MOD_FINI(xen_keyboard)
{
  grub_keyboard_controller_fini (NULL);
  grub_term_unregister_input (&grub_xen_keyboard_term);
}
