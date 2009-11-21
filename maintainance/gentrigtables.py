#!/usr/bin/python
# Script to generate trigonometric function tables.
#
#  GRUB  --  GRand Unified Bootloader
#  Copyright (C) 2008, 2009  Free Software Foundation, Inc.
#
#  GRUB is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  GRUB is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.

from math import *
from sys import stdout

def write(x):
    stdout.write(x)

def writeTable(arr, name):
    indent = ' ' * 4
    write("short ")
    write(name)
    write("[] =\n{\n")
    write(indent)
    for i in range(len(arr)):
        if i != 0:
            write(",")
            if i % 10 == 0:
                write("\n")
                write(indent)
        write("%d" % arr[i])
    write("\n};\n")

def main():
    sintab = []
    costab = []
    angle_max = 256
    fraction_scale = 16384
    for i in range(angle_max):
        # Convert to an angle in 1/256 of a circle.
        x = i * 2 * pi / angle_max
        sintab.append(int(round(sin(x) * fraction_scale)))
        costab.append(int(round(cos(x) * fraction_scale)))

    write("#define TRIG_ANGLE_MAX " + str (angle_max) + "\n")
    write("#define TRIG_FRACTION_SCALE " + str (fraction_scale) + "\n")
    writeTable(sintab, "sintab")
    writeTable(costab, "costab")

if __name__ == "__main__":
    main()

# vim:ai et sw=4 ts=4
