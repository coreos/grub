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
outfile = open (sys.argv[3], "w")
outfile.write ("#include <grub/unicode.h>\n")
outfile.write ("\n")
outfile.write ("struct grub_unicode_compact_range grub_unicode_compact[] = {\n")

begincode = -2
lastcode = -2
lastbiditype = "X"
lastmirrortype = False
lastcombtype = -1
for line in infile:
    sp = line.split (";")
    curcode = int (sp[0], 16)
    curcombtype = int (sp[3], 10)
    curbiditype = sp[4]
    curmirrortype = (sp[9] == "Y")
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
            or curcombtype != lastcombtype or curmirrortype != lastmirrortype:
        if begincode != -2 and (lastbiditype != "L" or lastcombtype != 0 or \
                                    lastmirrortype):
            outfile.write (("{0x%x, 0x%x, GRUB_BIDI_TYPE_%s, %d, %d},\n" \
                                % (begincode, lastcode, lastbiditype, \
                                       lastcombtype, lastmirrortype)))
        begincode = curcode
    lastcode = curcode
    lastbiditype = curbiditype
    lastcombtype = curcombtype
    lastmirrortype = curmirrortype
if lastbiditype != "L" or lastcombtype != 0 or lastmirrortype:
    outfile.write (("{0x%x, 0x%x, GRUB_BIDI_TYPE_%s, %d, %d},\n" \
                        % (begincode, lastcode, lastbiditype, lastcombtype, \
                               lastmirrortype)))
outfile.write ("{0, 0, 0, 0, 0},\n")

outfile.write ("};\n")

infile.close ()

infile = open (sys.argv[2], "r")

outfile.write ("struct grub_unicode_bidi_pair grub_unicode_bidi_pairs[] = {\n")

for line in infile:
    line = re.sub ("#.*$", "", line)
    line = line.replace ("\n", "")
    line = line.replace (" ", "")
    if len (line) == 0 or line[0] == '\n':
        continue
    sp = line.split (";")
    code1 = int (sp[0], 16)
    code2 = int (sp[1], 16)
    outfile.write ("{0x%x, 0x%x},\n" % (code1, code2))
outfile.write ("{0, 0},\n")
outfile.write ("};\n")

