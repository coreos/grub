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

import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * Program to convert BDF fonts into PFF2 fonts for use with GRUB.
 */
public class Converter {
    public static void main(String[] args) {
        if (args.length < 1) {
            printUsageAndExit();
        }

        String in = null;
        String out = null;
        List <CharacterRange> rangeList = new ArrayList<CharacterRange>();

        try {
            for (String arg : args) {
				if (arg.startsWith("--")) {
					String option;
					String value;
					int equalsPos = arg.indexOf('=');
					if (equalsPos < 0) {
						option = arg.substring(2);
						value = null;
					} else {
						option = arg.substring(2, equalsPos);
						value = arg.substring(equalsPos + 1);
					}

					if ("in".equals(option)) {
						if (value == null)
							throw new CommandLineException(option
									+ " option requires a value.");
						in = value;
					} else if ("out".equals(option)) {
						if (value == null)
							throw new CommandLineException(option
									+ " option requires a value.");
						out = value;
					}
				} else if (arg.startsWith("0x")) {
					// Range specifier
					String strRange[] = arg.split("-");

					if (strRange.length > 0) {
						boolean validRange = true;
						int start;
						int end;

						if (strRange.length > 2) {
							validRange = false;
						} else if (strRange.length == 2
								&& !strRange[1].startsWith("0x")) {
							validRange = false;
						} else
						{
							try {
								start = Integer.parseInt(strRange[0]
										.substring(2), 16);
								end = start;

								if (strRange.length == 2) {
									end = Integer.parseInt(strRange[1]
											.substring(2), 16);
								}
								
								CharacterRange range = new CharacterRange(
										start, end);
								boolean add = true;
								
								// First, try to combine range to existing ranges
								for (Iterator<CharacterRange> iter = rangeList.iterator(); iter.hasNext(); )
								{
									CharacterRange item = iter.next();
									
									if (range.equals(item))
									{
										add = false;
										continue;
									}
									
									if (item.combine(range))
									{
										// Start from beginning of list using combined range
										range = item;
										iter = rangeList.iterator();
										add = false;
									}
								}
								
								// If range could not be combined or no matching range, add it to the list
								if (add)
								{
									rangeList.add(range);
								}

							} catch (NumberFormatException e) {
								validRange = false;
							}

						}

						if (!validRange) {
							throw new CommandLineException("Invalid range `"
									+ arg + "'.");
						}
					}
				} else {
                    throw new CommandLineException("Non-option argument `" + arg + "'.");
                }
            }
            if (in == null || out == null) {
                throw new CommandLineException("Both --in=X and --out=Y must be specified.");
            }
        } catch (CommandLineException e) {
            System.err.println("Error: " + e.getMessage());
            System.exit(1);
        }

        try {
            // Read BDF.
            Font font = BDFLoader.loadFontFile(in);

            // Write PFF2.
            new PFF2Writer(out).writeFont(font, rangeList);
        } catch (IOException e) {
            System.err.println("I/O error converting font: " + e);
            e.printStackTrace();
            System.exit(1);
        }
    }

    private static class CommandLineException extends Exception {
        public CommandLineException(String message) {
            super(message);
        }
    }

    private static void printUsageAndExit() {
        System.err.println("GNU GRUB Font Conversion Tool");
        System.err.println();
        System.err.println("Usage: Converter --in=IN.bdf --out=OUT.pf2");
        System.err.println();
        System.exit(1);
    }
}
