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

public class Glyph {
    private final int codePoint;
    private final int width;
    private final int height;

    // These define the amounts to shift the character bitmap by
    // before drawing it.  See
    //    http://www.adobe.com/devnet/font/pdfs/5005.BDF_Spec.pdf
    // and
    //    http://www.linuxbabble.com/documentation/x/bdf/
    // for explanatory figures.
    private final int bbox;
    private final int bboy;

    // Number of pixels to advance horizontally from this character's origin
    // to the origin of the next character.
    private final int deviceWidth;

    // Row-major order, no padding.  Rows can break within a byte.
    // MSb is first (leftmost/uppermost) pixel.
    private final byte[] bitmap;

    public Glyph(int codePoint, int width, int height, int bbox, int bboy, int deviceWidth) {
        this(codePoint, width, height, bbox, bboy, deviceWidth,
             new byte[(width * height + 7) / 8]);
    }

    public Glyph(int codePoint, int width, int height,
                 int bbox, int bboy, int deviceWidth,
                 byte[] bitmap) {
        this.codePoint = codePoint;
        this.width = width;
        this.height = height;
        this.bboy = bboy;
        this.bbox = bbox;
        this.deviceWidth = deviceWidth;
        this.bitmap = bitmap;
    }

    public void setPixel(int x, int y, boolean value) {
        if (x < 0 || y < 0 || x >= width || y >= height)
            throw new IllegalArgumentException(
                    "Invalid pixel location (" + x + ", " + y + ") for "
                    + width + "x" + height + " glyph");

        int bitIndex = y * width + x;
        int byteIndex = bitIndex / 8;
        int bitPos = bitIndex % 8;
        int v = value ? 0x80 >>> bitPos : 0;
        int mask = ~(0x80 >>> bitPos);
        bitmap[byteIndex] = (byte) ((bitmap[byteIndex] & mask) | v);
    }

    public int getCodePoint() {
        return codePoint;
    }

    public int getWidth() {
        return width;
    }

    public int getHeight() {
        return height;
    }

    public int getBbox() {
        return bbox;
    }

    public int getBboy() {
        return bboy;
    }

    public int getDeviceWidth() {
        return deviceWidth;
    }

    public byte[] getBitmap() {
        return bitmap;
    }
}
