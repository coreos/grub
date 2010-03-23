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
infile = open (sys.argv[3], "r")
joining = {}
for line in infile:
    line = re.sub ("#.*$", "", line)
    line = line.replace ("\n", "")
    line = line.replace (" ", "")
    if len (line) == 0 or line[0] == '\n':
        continue
    sp = line.split (";")
    curcode = int (sp[0], 16)
    if sp[2] == "U":
        joining[curcode] = "GRUB_JOIN_TYPE_NONJOINING"
    elif sp[2] == "L":
        joining[curcode] = "GRUB_JOIN_TYPE_LEFT"
    elif sp[2] == "R":
        joining[curcode] = "GRUB_JOIN_TYPE_RIGHT"
    elif sp[2] == "D":
        joining[curcode] = "GRUB_JOIN_TYPE_DUAL"
    elif sp[2] == "C":
        joining[curcode] = "GRUB_JOIN_TYPE_CAUSING"
    else:
        print ("Unknown joining type '%s'" % sp[2])
        exit (1)
infile.close ()

infile = open (sys.argv[1], "r")
outfile = open (sys.argv[4], "w")
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
    if sp[2] != "Lu" and sp[2] != "Ll" and sp[2] != "Lt" and sp[2] != "Lm" \
            and sp[2] != "Lo"\
            and sp[2] != "Me" and sp[2] != "Mc" and sp[2] != "Mn" \
            and sp[2] != "Nd" and sp[2] != "Nl" and sp[2] != "No" \
            and sp[2] != "Pc" and sp[2] != "Pd" and sp[2] != "Ps" \
            and sp[2] != "Pe" and sp[2] != "Pi" and sp[2] != "Pf" \
            and sp[2] != "Po" \
            and sp[2] != "Sm" and sp[2] != "Sc" and sp[2] != "Sk" \
            and sp[2] != "So"\
            and sp[2] != "Zs" and sp[2] != "Zl" and sp[2] != "Zp" \
            and sp[2] != "Cc" and sp[2] != "Cf" and sp[2] != "Cs" \
            and sp[2] != "Co":
        print ("WARNING: Unknown type %s" % sp[2])
    if curcombtype == 0 and sp[2] == "Me":
        curcombtype = 253
    if curcombtype == 0 and sp[2] == "Mc":
        curcombtype = 254
    if curcombtype == 0 and sp[2] == "Mn":
        curcombtype = 255
    if (curcombtype >= 2 and curcombtype <= 6) \
            or (curcombtype >= 37 and curcombtype != 84 and curcombtype != 91  and curcombtype != 103 and curcombtype != 107 and curcombtype != 118 and curcombtype != 122 and curcombtype != 129 and curcombtype != 130 and curcombtype != 132 and curcombtype != 202 and \
                curcombtype != 214 and curcombtype != 216 and \
                curcombtype != 218 and curcombtype != 220 and \
                curcombtype != 222 and curcombtype != 224 and curcombtype != 226 and curcombtype != 228 and \
                curcombtype != 230 and curcombtype != 232 and curcombtype != 233 and \
                curcombtype != 234 and \
                curcombtype != 240 and curcombtype != 253 and \
                curcombtype != 254 and curcombtype != 255):
        print ("WARNING: Unknown combining type %d" % curcombtype)
    if curcode in joining:
        curjoin = joining[curcode]
    elif sp[2] == "Me" or sp[2] == "Mn" or sp[2] == "Cf":
        curjoin = "GRUB_JOIN_TYPE_TRANSPARENT"
    else:
        curjoin = "GRUB_JOIN_TYPE_NONJOINING"
    if lastcode + 1 != curcode or curbiditype != lastbiditype \
            or curcombtype != lastcombtype or curmirrortype != lastmirrortype \
            or curjoin != lastjoin:
        if begincode != -2 and (lastbiditype != "L" or lastcombtype != 0 or \
                                    lastmirrortype):
            outfile.write (("{0x%x, 0x%x, GRUB_BIDI_TYPE_%s, %d, %d, %s},\n" \
                                % (begincode, lastcode, lastbiditype, \
                                       lastcombtype, lastmirrortype, \
                                       lastjoin)))
        begincode = curcode
    lastcode = curcode
    lastjoin = curjoin
    lastbiditype = curbiditype
    lastcombtype = curcombtype
    lastmirrortype = curmirrortype
if lastbiditype != "L" or lastcombtype != 0 or lastmirrortype:
    outfile.write (("{0x%x, 0x%x, GRUB_BIDI_TYPE_%s, %d, %d, %s},\n" \
                        % (begincode, lastcode, lastbiditype, lastcombtype, \
                               lastmirrortype, lastjoin)))
outfile.write ("{0, 0, 0, 0, 0, 0},\n")

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

