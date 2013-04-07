#*
#*  GRUB  --  GRand Unified Bootloader
#*  Copyright (C) 2013  Free Software Foundation, Inc.
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
import codecs
import datetime

if len (sys.argv) < 3:
    print ("Usage: %s SOURCE DESTINATION" % sys.argv[0])
    exit (0)
dtcdir = sys.argv[1]
indir = os.path.join (dtcdir, "libfdt/")
outdir = os.path.join (sys.argv[2], "lib/dtc-grub/libfdt/")
try:
    os.makedirs (outdir)
except:
    print ("WARNING: %s already exists" % outdir)

conf = codecs.open (os.path.join ("grub-core/", "Makefile.libfdt.def"), "w", "utf-8")
conf.write ("AutoGen definitions Makefile.tpl;\n\n")
conf.write ("module = {\n")
conf.write ("  name = fdt;\n")
conf.write ("  common = lib/dtc-grub/libfdt/fdt.c;\n")
conf.write ("  common = lib/dtc-grub/libfdt/fdt_ro.c;\n")
conf.write ("  common = lib/dtc-grub/libfdt/fdt_rw.c;\n")
conf.write ("  common = lib/dtc-grub/libfdt/fdt_strerror.c;\n")
conf.write ("  common = lib/dtc-grub/libfdt/fdt_sw.c;\n")
conf.write ("  common = lib/dtc-grub/libfdt/fdt_wip.c;\n")
conf.write ("  cppflags = '$(CPPFLAGS_POSIX) $(CPPFLAGS_LIBFDT)';\n")
conf.write ("\n")
conf.write ("  enable = fdt;\n")
conf.write ("};\n")

conf.close ();


libfdt_files = sorted (os.listdir (indir))
chlog = ""

for libfdt_file in libfdt_files:
    infile = os.path.join (indir, (libfdt_file))
    outfile = os.path.join (outdir, (libfdt_file))

    if not re.match (".*\.[ch]$", libfdt_file):
        chlog = "%s	* %s: Removed\n" % (chlog, libfdt_file)
        continue

#    print ("file: %s, infile: %s, outfile: %s" % (libfdt_file, infile, outfile))

    f = codecs.open (infile, "r", "utf-8")
    fw = codecs.open (outfile, "w", "utf-8")

    lineno = 1

    fw.write ("/* This file was automatically imported with \n")
    fw.write ("   import_libfdt.py. Please don't modify it */\n")
    fw.write ("#include <grub/dl.h>\n")

    # libfdt is dual-licensed: BSD or GPLv2+
    if re.match (".*\.c$", libfdt_file):
        fw.write ("GRUB_MOD_LICENSE (\"GPLv2+\");\n")

    lines = f.readlines()

    for line in lines:
        fw.write (line)
        
    f.close ()
    fw.close ()

patchfile = os.path.join (dtcdir, "libfdt-grub.diff")
#print "Patchfile: %s\n" % patchfile
ret = os.system("patch -d %s -p1 < %s" % (outdir, patchfile))
if ret:
    chlog = "%s	* Applied Grub build patch\n" % chlog


dt = datetime.date.today ()
fw = codecs.open (os.path.join (outdir, "ImportLog"), "w", "utf-8")
fw.write ("%04d-%02d-%02d  Automatic import tool\n" % \
          (dt.year,dt.month, dt.day))
fw.write ("\n")
fw.write ("	Imported libfdt to GRUB\n")
fw.write ("\n")
fw.write (chlog)
fw.close ()
