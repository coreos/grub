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

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.TreeMap;

class CharDefs {
    private boolean debug = "1".equals(System.getProperty("fonttool.debug"));
    private TreeMap<Integer, Glyph> glyphs;
    private ByteArrayOutputStream charDefsData;
    private int maxCharWidth;
    private int maxCharHeight;
    private HashMap<Integer, CharStorageInfo> charIndex;

    public CharDefs(Font font) {
        this.glyphs = font.getGlyphs();
        this.charIndex = null;
        this.charDefsData = null;

        calculateMaxSizes();
    }

    private void calculateMaxSizes() {
        maxCharWidth = 0;
        maxCharHeight = 0;
        for (Glyph glyph : glyphs.values()) {
            final int w = glyph.getWidth();
            final int h = glyph.getHeight();
            if (w > maxCharWidth)
                maxCharWidth = w;
            if (h > maxCharHeight)
                maxCharHeight = h;
        }
    }

    void buildDefinitions(List<CharacterRange> rangeList) {
        charIndex = new HashMap<Integer, CharStorageInfo>();
        HashMap<CharDef, Long> charDefIndex = new HashMap<CharDef, Long>();
        charDefsData = new ByteArrayOutputStream();
        DataOutputStream charDefs = new DataOutputStream(charDefsData);
        try {
            // Loop through all the glyphs, writing the glyph data to the
            // in-memory byte stream, collapsing duplicate glyphs, and
            // constructing index information.
            for (Glyph glyph : glyphs.values()) {
            	// Determine if glyph should be included in written file
				if (rangeList.size() > 0) {
	            	boolean skip = true;
	            	
					for (Iterator<CharacterRange> iter = rangeList.iterator(); iter
							.hasNext();) {
						CharacterRange item = iter.next();

						if (item.isWithinRange(glyph.getCodePoint())) {
							skip = false;
							break;
						}
					}

					if (skip) {
						continue;
					}
				}
            	
                CharDef charDef = new CharDef(glyph.getWidth(),
                                              glyph.getHeight(),
                                              glyph.getBitmap());

                if (charDefIndex.containsKey(charDef)) {
                    // Use already-written glyph.
                    if (debug)
                        System.out.printf("Duplicate glyph for character U+%04X%n",
                                          glyph.getCodePoint());
                    final int charOffset = charDefIndex.get(charDef).intValue();
                    final CharStorageInfo info =
                            new CharStorageInfo(glyph.getCodePoint(), charOffset);
                    charIndex.put(glyph.getCodePoint(), info);
                } else {
                    // Write glyph data.
                    final int charOffset = charDefs.size();
                    final CharStorageInfo info =
                            new CharStorageInfo(glyph.getCodePoint(), charOffset);
                    charIndex.put(glyph.getCodePoint(), info);

                    charDefIndex.put(charDef, (long) charOffset);

                    charDefs.writeShort(glyph.getWidth());
                    charDefs.writeShort(glyph.getHeight());
                    charDefs.writeShort(glyph.getBbox());
                    charDefs.writeShort(glyph.getBboy());
                    charDefs.writeShort(glyph.getDeviceWidth());
                    charDefs.write(glyph.getBitmap());
                }
            }
        } catch (IOException e) {
            throw new RuntimeException("Error writing to in-memory byte stream", e);
        }
    }

    public int getMaxCharWidth() {
        return maxCharWidth;
    }

    public int getMaxCharHeight() {
        return maxCharHeight;
    }

    public HashMap<Integer, CharStorageInfo> getCharIndex() {
        if (charIndex == null) throw new IllegalStateException();
        return charIndex;
    }

    public byte[] getDefinitionData() {
        if (charDefsData == null) throw new IllegalStateException();
        return charDefsData.toByteArray();
    }


    private static class CharDef {
        private final int width;
        private final int height;
        private final byte[] data;

        public CharDef(int width, int height, byte[] data) {
            this.width = width;
            this.height = height;
            this.data = data;
        }

        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            CharDef charDef = (CharDef) o;

            if (height != charDef.height) return false;
            if (width != charDef.width) return false;
            if (!Arrays.equals(data, charDef.data)) return false;

            return true;
        }

        public int hashCode() {
            int result;
            result = width;
            result = 31 * result + height;
            result = 31 * result + Arrays.hashCode(data);
            return result;
        }
    }


	public int getIndexSize() {
		if (charIndex == null)
			return 0;
		
		return charIndex.size();
	}
}
