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
outfile.write ("#include <grub/unicode.h>\n")
outfile.write ("\n")
outfile.write ("struct grub_unicode_compact_range grub_unicode_compact[] = {\n")

begincode = -2
lastcode = -2
lastbiditype = "X"
lastcombtype = -1
for line in infile:
    sp = line.split (";")
    curcode = int (sp[0], 16)
    curbiditype = sp[4]
    curcombtype = int (sp[3], 10)
    if curcombtype <= 255 and curcombtype >= 253:
        print ("UnicodeData.txt uses combination type %d. Conflict." \
                   % curcombtype)
        raise
    if curcombtype == 0 and sp[2] == "Me":
        curcombtype = 253
    if curcombtype == 0 and sp[2] == "Mc":
        curcombtype = 254
    if curcombtype == 0 and sp[2] == "Mn":
        curcombtype = 255
    if lastcode + 1 != curcode or curbiditype != lastbiditype \
            or curcombtype != lastcombtype:
        if begincode != -2 and (lastbiditype != "L" or lastcombtype != 0):
            outfile.write (("{0x%x, 0x%x, GRUB_BIDI_TYPE_%s, %d},\n" \
                                % (begincode, lastcode, lastbiditype, \
                                       lastcombtype)))
        begincode = curcode
    lastcode = curcode
    lastbiditype = curbiditype
    lastcombtype = curcombtype
if lastbiditype != "L" or lastcombtype != 0:
    outfile.write (("{0x%x, 0x%x, GRUB_BIDI_TYPE_%s, %d},\n" \
                        % (begincode, lastcode, lastbiditype, lastcombtype)))
outfile.write ("{0, 0, 0, 0},\n")

outfile.write ("};")
