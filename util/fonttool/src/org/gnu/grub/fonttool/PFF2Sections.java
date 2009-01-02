/**
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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
package org.gnu.grub.fonttool;

/**
 * Section name constants for the PFF2 file format.
 */
public class PFF2Sections {
    static final String FILE = "FILE";
    static final String FONT_NAME = "NAME";
    static final String FONT_FAMILY = "FAMI";
    static final String FONT_WEIGHT = "WEIG";
    static final String FONT_SLANT = "SLAN";
    static final String FONT_POINT_SIZE = "PTSZ";
    static final String MAX_CHAR_WIDTH = "MAXW";
    static final String MAX_CHAR_HEIGHT = "MAXH";
    static final String FONT_ASCENT = "ASCE";
    static final String FONT_DESCENT = "DESC";
    static final String CHAR_INDEX = "CHIX";
    static final String REMAINDER_IS_DATA = "DATA";
}
