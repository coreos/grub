#*
#*  GRUB  --  GRand Unified Bootloader
#*  Copyright (C) 2010  Free Software Foundation, Inc.
#*
#*  GRUB is free software: you can redistribute it and/or modify
#*  it under the terms of the GNU General Public License as published by
#*  the Free Software Foundation, either version 3 of the License, or
#*  (at your option) any later version.
#*
#*  GRUB is distributed in the hope that it will be useful,
#*  but WITHOUT ANY WARRANTY; without even the implied warranty of
#*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#*  GNU General Public License for more details.
#*
#*  You should have received a copy of the GNU General Public License
#*  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
#*

import re
import sys
import os
import datetime

if len (sys.argv) < 3:
    print ("Usage: %s SOURCE DESTINATION" % sys.argv[0])
    exit (0)
infile = open (sys.argv[1], "r")
outfile = open (sys.argv[2], "w")
outfile.write ("#include <grub/bidi.h>\n")
outfile.write ("\n")
outfile.write ("struct grub_bidi_compact_range grub_bidi_compact[] = {\n")

begin = -2
last = -2
lasttype = "X"
for line in infile:
    sp = line.split (";")
    cur = int (sp[0], 16)
    curtype = sp[4]
    if last + 1 != cur or curtype != lasttype:
        if begin != -2 and lasttype != "L":
            outfile.write (("{0x%x, 0x%x, GRUB_BIDI_TYPE_%s},\n" \
                                % (begin, last, lasttype)))
        begin = cur
    last = cur
    lasttype = curtype
if lasttype != "L":
    outfile.write (("{0x%x, 0x%x, GRUB_BIDI_TYPE_%s},\n" \
                        % (begin, last, lasttype)))
outfile.write ("{0, 0, 0},\n")

outfile.write ("};")
