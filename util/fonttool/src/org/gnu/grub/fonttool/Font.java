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

import java.util.TreeMap;

public class Font {
    public static final int UNKNOWN_POINT_SIZE = -1;

    private TreeMap<Integer, Glyph> glyphs;
    private String family;
    private boolean bold;
    private boolean italic;
    private int pointSize;
    private int maxCharWidth;
    private int maxCharHeight;
    private int ascent;
    private int descent;

    public Font() {
        glyphs = new TreeMap<Integer, Glyph>();
    }

    public String getFamily() {
        return family;
    }

    public void setFamily(String family) {
        this.family = family;
    }

    public boolean isBold() {
        return bold;
    }

    public void setBold(boolean bold) {
        this.bold = bold;
    }

    public boolean isItalic() {
        return italic;
    }

    public void setItalic(boolean italic) {
        this.italic = italic;
    }

    public int getPointSize() {
        return pointSize;
    }

    public void setPointSize(int pointSize) {
        this.pointSize = pointSize;
    }

    public int getMaxCharWidth() {
        return maxCharWidth;
    }

    public void setMaxCharWidth(int maxCharWidth) {
        this.maxCharWidth = maxCharWidth;
    }

    public int getMaxCharHeight() {
        return maxCharHeight;
    }

    public void setMaxCharHeight(int maxCharHeight) {
        this.maxCharHeight = maxCharHeight;
    }

    public int getAscent() {
        return ascent;
    }

    public void setAscent(int ascent) {
        this.ascent = ascent;
    }

    public int getDescent() {
        return descent;
    }

    public void setDescent(int descent) {
        this.descent = descent;
    }

    public void putGlyph(int codePoint, Glyph glyph) {
        glyphs.put(codePoint, glyph);
    }

    public TreeMap<Integer, Glyph> getGlyphs() {
        return glyphs;
    }

    public Glyph getGlyph(int codePoint) {
        return glyphs.get(codePoint);
    }

    public String getStandardName() {
        StringBuilder name = new StringBuilder(getFamily());
        if (isBold()) name.append(" Bold");
        if (isItalic()) name.append(" Italic");
        name.append(' ');
        name.append(getPointSize());
        return name.toString();
    }
}
