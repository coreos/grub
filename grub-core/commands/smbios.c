/* smbios.c - retrieve smbios information. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013,2014,2015  Free Software Foundation, Inc.
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
#include <grub/env.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/smbios.h>

GRUB_MOD_LICENSE ("GPLv3+");


/* Locate the SMBIOS entry point structure depending on the hardware. */
struct grub_smbios_eps *
grub_smbios_get_eps (void)
{
  static struct grub_smbios_eps *eps = NULL;
  if (eps != NULL)
    return eps;
  eps = grub_machine_smbios_get_eps ();
  return eps;
}

/* Locate the SMBIOS3 entry point structure depending on the hardware. */
struct grub_smbios_eps3 *
grub_smbios_get_eps3 (void)
{
  static struct grub_smbios_eps3 *eps = NULL;
  if (eps != NULL)
    return eps;
  eps = grub_machine_smbios_get_eps3 ();
  return eps;
}

/* Abstract useful values found in either the SMBIOS3 or SMBIOS EPS. */
static struct {
  grub_addr_t start;
  grub_addr_t end;
  grub_uint16_t structures;
} table_desc = {0, 0, 0};


/*
 * These functions convert values from the various SMBIOS structure field types
 * into a string formatted to be returned to the user.  They expect that the
 * structure and offset were already validated.  The given buffer stores the
 * newly formatted string if needed.  When the requested data is successfully
 * retrieved and formatted, the pointer to the string is returned; otherwise,
 * NULL is returned on failure.
 */

static const char *
grub_smbios_format_byte (char *buffer, grub_size_t size,
                         const grub_uint8_t *structure, grub_uint8_t offset)
{
  grub_snprintf (buffer, size, "%u", structure[offset]);
  return (const char *)buffer;
}

static const char *
grub_smbios_format_word (char *buffer, grub_size_t size,
                         const grub_uint8_t *structure, grub_uint8_t offset)
{
  grub_uint16_t value = grub_get_unaligned16 (structure + offset);
  grub_snprintf (buffer, size, "%u", value);
  return (const char *)buffer;
}

static const char *
grub_smbios_format_dword (char *buffer, grub_size_t size,
                          const grub_uint8_t *structure, grub_uint8_t offset)
{
  grub_uint32_t value = grub_get_unaligned32 (structure + offset);
  grub_snprintf (buffer, size, "%" PRIuGRUB_UINT32_T, value);
  return (const char *)buffer;
}

static const char *
grub_smbios_format_qword (char *buffer, grub_size_t size,
                          const grub_uint8_t *structure, grub_uint8_t offset)
{
  grub_uint64_t value = grub_get_unaligned64 (structure + offset);
  grub_snprintf (buffer, size, "%" PRIuGRUB_UINT64_T, value);
  return (const char *)buffer;
}

/* The matching string pointer is returned directly to avoid extra copying. */
static const char *
grub_smbios_get_string (char *buffer __attribute__ ((unused)),
                        grub_size_t size __attribute__ ((unused)),
                        const grub_uint8_t *structure, grub_uint8_t offset)
{
  const grub_uint8_t *ptr = structure + structure[1];
  const grub_uint8_t *table_end = (const grub_uint8_t *)table_desc.end;
  const grub_uint8_t referenced_string_number = structure[offset];
  grub_uint8_t i;

  /* A string referenced with zero is interpreted as unset. */
  if (referenced_string_number == 0)
    return NULL;

  /* Search the string set. */
  for (i = 1; *ptr != 0 && ptr < table_end; i++)
    if (i == referenced_string_number)
      {
        const char *str = (const char *)ptr;
        while (*ptr++ != 0)
          if (ptr >= table_end)
            return NULL; /* The string isn't terminated. */
        return str;
      }
    else
      while (*ptr++ != 0 && ptr < table_end);

  /* The string number is greater than the number of strings in the set. */
  return NULL;
}

static const char *
grub_smbios_format_uuid (char *buffer, grub_size_t size,
                         const grub_uint8_t *structure, grub_uint8_t offset)
{
  const grub_uint8_t *f = structure + offset; /* little-endian fields */
  const grub_uint8_t *g = f + 8; /* byte-by-byte fields */
  grub_snprintf (buffer, size,
                 "%02x%02x%02x%02x-%02x%02x-%02x%02x-"
                 "%02x%02x-%02x%02x%02x%02x%02x%02x",
                 f[3], f[2], f[1], f[0], f[5], f[4], f[7], f[6],
                 g[0], g[1], g[2], g[3], g[4], g[5], g[6], g[7]);
  return (const char *)buffer;
}


/* List the field formatting functions and the number of bytes they need. */
#define MAXIMUM_FORMAT_LENGTH (sizeof ("ffffffff-ffff-ffff-ffff-ffffffffffff"))
static const struct {
  const char *(*format) (char *buffer, grub_size_t size,
                         const grub_uint8_t *structure, grub_uint8_t offset);
  grub_uint8_t field_length;
} field_extractors[] = {
  {grub_smbios_format_byte, 1},
  {grub_smbios_format_word, 2},
  {grub_smbios_format_dword, 4},
  {grub_smbios_format_qword, 8},
  {grub_smbios_get_string, 1},
  {grub_smbios_format_uuid, 16}
};

/* List command options, with structure field getters ordered as above. */
#define FIRST_GETTER_OPT (3)
#define SETTER_OPT (FIRST_GETTER_OPT + ARRAY_SIZE(field_extractors))
static const struct grub_arg_option options[] = {
  {"type",       't', 0, N_("Match entries with the given type."),
                         N_("type"), ARG_TYPE_INT},
  {"handle",     'h', 0, N_("Match entries with the given handle."),
                         N_("handle"), ARG_TYPE_INT},
  {"match",      'm', 0, N_("Select a structure when several match."),
                         N_("match"), ARG_TYPE_INT},
  {"get-byte",   'b', 0, N_("Get the byte's value at the given offset."),
                         N_("offset"), ARG_TYPE_INT},
  {"get-word",   'w', 0, N_("Get two bytes' value at the given offset."),
                         N_("offset"), ARG_TYPE_INT},
  {"get-dword",  'd', 0, N_("Get four bytes' value at the given offset."),
                         N_("offset"), ARG_TYPE_INT},
  {"get-qword",  'q', 0, N_("Get eight bytes' value at the given offset."),
                         N_("offset"), ARG_TYPE_INT},
  {"get-string", 's', 0, N_("Get the string specified at the given offset."),
                         N_("offset"), ARG_TYPE_INT},
  {"get-uuid",   'u', 0, N_("Get the UUID's value at the given offset."),
                         N_("offset"), ARG_TYPE_INT},
  {"set",       '\0', 0, N_("Store the value in the given variable name."),
                         N_("variable"), ARG_TYPE_STRING},
  {0, 0, 0, 0, 0, 0}
};


/*
 * Return a matching SMBIOS structure.
 *
 * This method can use up to three criteria for selecting a structure:
 *   - The "type" field                  (use -1 to ignore)
 *   - The "handle" field                (use -1 to ignore)
 *   - Which to return if several match  (use  0 to ignore)
 *
 * The return value is a pointer to the first matching structure.  If no
 * structures match the given parameters, NULL is returned.
 */
static const grub_uint8_t *
grub_smbios_match_structure (const grub_int16_t type,
                             const grub_int32_t handle,
                             const grub_uint16_t match)
{
  const grub_uint8_t *ptr = (const grub_uint8_t *)table_desc.start;
  const grub_uint8_t *table_end = (const grub_uint8_t *)table_desc.end;
  grub_uint16_t structures = table_desc.structures;
  grub_uint16_t structure_count = 0;
  grub_uint16_t matches = 0;

  while (ptr < table_end
         && ptr[1] >= 4 /* Valid structures include the 4-byte header. */
         && (structure_count++ < structures || structures == 0))
    {
      grub_uint16_t structure_handle = grub_get_unaligned16 (ptr + 2);
      grub_uint8_t structure_type = ptr[0];

      if ((handle < 0 || handle == structure_handle)
          && (type < 0 || type == structure_type)
          && (match == 0 || match == ++matches))
        return ptr;

      else
        {
          ptr += ptr[1];
          while ((*ptr++ != 0 || *ptr++ != 0) && ptr < table_end);
        }

      if (structure_type == GRUB_SMBIOS_TYPE_END_OF_TABLE)
        break;
    }

  return NULL;
}


static grub_err_t
grub_cmd_smbios (grub_extcmd_context_t ctxt,
                 int argc __attribute__ ((unused)),
                 char **argv __attribute__ ((unused)))
{
  struct grub_arg_list *state = ctxt->state;

  grub_int16_t type = -1;
  grub_int32_t handle = -1;
  grub_uint16_t match = 0;
  grub_uint8_t offset = 0;

  const grub_uint8_t *structure;
  const char *value;
  char buffer[MAXIMUM_FORMAT_LENGTH];
  grub_int32_t option;
  grub_int8_t field_type = -1;
  grub_uint8_t i;

  if (table_desc.start == 0)
    return grub_error (GRUB_ERR_IO,
                       N_("the SMBIOS entry point structure was not found"));

  /* Read the given filtering options. */
  if (state[0].set)
    {
      option = grub_strtol (state[0].arg, NULL, 0);
      if (option < 0 || option > 255)
        return grub_error (GRUB_ERR_BAD_ARGUMENT,
                           N_("the type must be between 0 and 255"));
      type = (grub_int16_t)option;
    }
  if (state[1].set)
    {
      option = grub_strtol (state[1].arg, NULL, 0);
      if (option < 0 || option > 65535)
        return grub_error (GRUB_ERR_BAD_ARGUMENT,
                           N_("the handle must be between 0 and 65535"));
      handle = (grub_int32_t)option;
    }
  if (state[2].set)
    {
      option = grub_strtol (state[2].arg, NULL, 0);
      if (option <= 0)
        return grub_error (GRUB_ERR_BAD_ARGUMENT,
                           N_("the match must be a positive integer"));
      match = (grub_uint16_t)option;
    }

  /* Determine the data type of the structure field to retrieve. */
  for (i = 0; i < ARRAY_SIZE(field_extractors); i++)
    if (state[FIRST_GETTER_OPT + i].set)
      {
        if (field_type >= 0)
          return grub_error (GRUB_ERR_BAD_ARGUMENT,
                             N_("only one --get option is usable at a time"));
        field_type = i;
      }

  /* Require a choice of a structure field to return. */
  if (field_type < 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT,
                       N_("one of the --get options is required"));

  /* Locate a matching SMBIOS structure. */
  structure = grub_smbios_match_structure (type, handle, match);
  if (structure == NULL)
    return grub_error (GRUB_ERR_IO,
                       N_("no structure matched the given options"));

  /* Ensure the requested byte offset is inside the structure. */
  option = grub_strtol (state[FIRST_GETTER_OPT + field_type].arg, NULL, 0);
  if (option < 0 || option >= structure[1])
    return grub_error (GRUB_ERR_OUT_OF_RANGE,
                       N_("the given offset is outside the structure"));

  /* Ensure the requested data type at the offset is inside the structure. */
  offset = (grub_uint8_t)option;
  if (offset + field_extractors[field_type].field_length > structure[1])
    return grub_error (GRUB_ERR_OUT_OF_RANGE,
                       N_("the field ends outside the structure"));

  /* Format the requested structure field into a readable string. */
  value = field_extractors[field_type].format (buffer, sizeof (buffer),
                                               structure, offset);
  if (value == NULL)
    return grub_error (GRUB_ERR_IO,
                       N_("failed to retrieve the structure field"));

  /* Store or print the formatted value. */
  if (state[SETTER_OPT].set)
    grub_env_set (state[SETTER_OPT].arg, value);
  else
    grub_printf ("%s\n", value);

  return GRUB_ERR_NONE;
}


static grub_extcmd_t cmd;

GRUB_MOD_INIT(smbios)
{
  struct grub_smbios_eps3 *eps3;
  struct grub_smbios_eps *eps;

  if ((eps3 = grub_smbios_get_eps3 ()))
    {
      table_desc.start = (grub_addr_t)eps3->table_address;
      table_desc.end = table_desc.start + eps3->maximum_table_length;
      table_desc.structures = 0; /* SMBIOS3 drops the structure count. */
    }
  else if ((eps = grub_smbios_get_eps ()))
    {
      table_desc.start = (grub_addr_t)eps->intermediate.table_address;
      table_desc.end = table_desc.start + eps->intermediate.table_length;
      table_desc.structures = eps->intermediate.structures;
    }

  cmd = grub_register_extcmd ("smbios", grub_cmd_smbios, 0,
                              N_("[-t type] [-h handle] [-m match] "
                                 "(-b|-w|-d|-q|-s|-u) offset "
                                 "[--set variable]"),
                              N_("Retrieve SMBIOS information."), options);
}

GRUB_MOD_FINI(smbios)
{
  grub_unregister_extcmd (cmd);
}
