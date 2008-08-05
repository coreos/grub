/* kern/i386/tsc.c - x86 TSC time source implementation
 * Requires Pentium or better x86 CPU that supports the RDTSC instruction.
 * This module uses the RTC (via grub_get_rtc()) to calibrate the TSC to 
 * real time.
 *
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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
#include <grub/time.h>
#include <grub/misc.h>
#include <grub/i386/tsc.h>

/* Calibrated reference for TSC=0.  This defines the time since the epoch in 
   milliseconds that TSC=0 refers to. */
static grub_uint64_t tsc_boot_time;

/* Calibrated TSC rate.  (In TSC ticks per millisecond.) */
static grub_uint64_t tsc_ticks_per_ms;


grub_uint64_t
grub_tsc_get_time_ms (void)
{
  return tsc_boot_time + grub_divmod64 (grub_get_tsc (), tsc_ticks_per_ms, 0);
}


/* How many RTC ticks to use for calibration loop. (>= 1) */
#define CALIBRATION_TICKS 2

/* Calibrate the TSC based on the RTC.  */
static void
calibrate_tsc (void)
{
  /* First calbrate the TSC rate (relative, not absolute time). */
  grub_uint64_t start_tsc;
  grub_uint64_t end_tsc;
  grub_uint32_t initial_tick;
  grub_uint32_t start_tick;
  grub_uint32_t end_tick;

  /* Wait for the start of the next tick;
     we'll base out timing off this edge. */
  initial_tick = grub_get_rtc ();
  do
    {
      start_tick = grub_get_rtc ();
    }
  while (start_tick == initial_tick);
  start_tsc = grub_get_tsc ();

  /* Wait for the start of the next tick.  This will
     be the end of the 1-tick period. */
  do
    {
      end_tick = grub_get_rtc ();
    }
  while (end_tick - start_tick < CALIBRATION_TICKS);
  end_tsc = grub_get_tsc ();

  tsc_ticks_per_ms =
    grub_divmod64 (grub_divmod64
                   (end_tsc - start_tsc, end_tick - start_tick, 0)
                   * GRUB_TICKS_PER_SECOND, 1000, 0);

  /* Reference the TSC zero (boot time) to the epoch to 
     get an absolute real time reference. */
  grub_uint64_t ms_since_boot = grub_divmod64 (end_tsc, tsc_ticks_per_ms, 0);
  grub_uint64_t mstime_now = grub_divmod64 ((grub_uint64_t) 1000 * end_tick,
                                            GRUB_TICKS_PER_SECOND,
                                            0);
  tsc_boot_time = mstime_now - ms_since_boot;
}

void
grub_tsc_init (void)
{
  if (grub_cpu_is_tsc_supported ())
    {
      calibrate_tsc ();
      grub_install_get_time_ms (grub_tsc_get_time_ms);
    }
  else
    {
      grub_install_get_time_ms (grub_rtc_get_time_ms);
    }
}
