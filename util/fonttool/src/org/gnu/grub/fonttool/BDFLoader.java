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

import java.io.*;
import java.util.StringTokenizer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class BDFLoader {
    private final BufferedReader in;
    private final Font font;
    private int maxCharWidth;
    private int maxCharHeight;

    BDFLoader(BufferedReader in) {
        this.in = in;
        this.font = new Font();
        this.maxCharWidth = 0;
        this.maxCharHeight = 0;
    }

    public static boolean isBDFFile(String filename) {
        DataInputStream in = null;
        try {
            in = new DataInputStream(new FileInputStream(filename));
            final String signature = "STARTFONT ";
            byte[] b = new byte[signature.length()];
            in.readFully(b);
            in.close();

            String s = new String(b, "US-ASCII");
            return signature.equals(s);
        } catch (IOException e) {
            if (in != null) {
                try {
                    in.close();
                } catch (IOException e1) {
                    // Ignore.
                }
            }
            return false;
        }
    }

    public static Font loadFontResource(String resourceName) throws IOException {
        InputStream in = BDFLoader.class.getClassLoader().getResourceAsStream(resourceName);
        if (in == null)
            throw new FileNotFoundException("Font resource " + resourceName + " not found");
        return loadFontFromStream(in);
    }

    public static Font loadFontFile(String filename) throws IOException {
        InputStream in = new FileInputStream(filename);
        return loadFontFromStream(in);
    }

    private static Font loadFontFromStream(InputStream inStream) throws IOException {
        BufferedReader in;
        try {
            in = new BufferedReader(new InputStreamReader(inStream, "ISO-8859-1"));
        } catch (UnsupportedEncodingException e) {
            throw new RuntimeException("Encoding not supported: " + e.getMessage(), e);
        }
        return new BDFLoader(in).loadFont();
    }

    private Font loadFont() throws IOException {
        loadFontInfo();
        while (loadChar()) {
            /* Loop. */
        }

        font.setMaxCharWidth(maxCharWidth);
        font.setMaxCharHeight(maxCharHeight);
        return font;
    }

    private void loadFontInfo() throws IOException {
        final Pattern stringSettingPattern = Pattern.compile("^(\\w+)\\s+\"([^\"]+)\"$");
        String line;
        // Load the global font information that appears before CHARS.
        final int UNSET = Integer.MIN_VALUE;
        font.setAscent(UNSET);
        font.setDescent(UNSET);
        font.setFamily("Unknown");
        font.setBold(false);
        font.setItalic(false);
        font.setPointSize(Font.UNKNOWN_POINT_SIZE);
        do {
            line = in.readLine();
            if (line == null)
                throw new IOException("BDF format error: end of file while " +
                                      "reading global font information");

            StringTokenizer st = new StringTokenizer(line);
            if (st.hasMoreTokens()) {
                String name = st.nextToken();
                if (name.equals("FONT_ASCENT")) {
                    if (!st.hasMoreTokens())
                        throw new IOException("BDF format error: " +
                                              "no tokens after " + name);
                    font.setAscent(Integer.parseInt(st.nextToken()));
                } else if (name.equals("FONT_DESCENT")) {
                    if (!st.hasMoreTokens())
                        throw new IOException("BDF format error: " +
                                              "no tokens after " + name);
                    font.setDescent(Integer.parseInt(st.nextToken()));
                } else if (name.equals("POINT_SIZE")) {
                    if (!st.hasMoreTokens())
                        throw new IOException("BDF format error: " +
                                              "no tokens after " + name);
                    // Divide by 10, since it is stored X10.
                    font.setPointSize(Integer.parseInt(st.nextToken()) / 10);
                } else if (name.equals("FAMILY_NAME")) {
                    Matcher matcher = stringSettingPattern.matcher(line);
                    if (!matcher.matches())
                        throw new IOException("BDF format error: " +
                                              "line doesn't match string " +
                                              "setting pattern: " + line);
                    font.setFamily(matcher.group(2));
                } else if (name.equals("WEIGHT_NAME")) {
                    Matcher matcher = stringSettingPattern.matcher(line);
                    if (!matcher.matches())
                        throw new IOException("BDF format error: " +
                                              "line doesn't match string " +
                                              "setting pattern: " + line);
                    String weightName = matcher.group(2);
                    font.setBold("bold".equalsIgnoreCase(weightName));
                } else if (name.equals("SLANT")) {
                    Matcher matcher = stringSettingPattern.matcher(line);
                    if (!matcher.matches())
                        throw new IOException("BDF format error: " +
                                              "line doesn't match string " +
                                              "setting pattern: " + line);
                    String slantType = matcher.group(2);
                    font.setItalic(!"R".equalsIgnoreCase(slantType));
                } else if (name.equals("CHARS")) {
                    // This is the end of the global font information and
                    // the beginning of the character definitions.
                    break;
                } else {
                    // Skip other fields.
                }
            }
        } while (true);

        if (font.getAscent() == UNSET)
            throw new IOException("BDF format error: no FONT_ASCENT property");
        if (font.getDescent() == UNSET)
            throw new IOException("BDF format error: no FONT_DESCENT property");
    }

    private boolean loadChar() throws IOException {
        String line;
        // Find start of character
        do {
            line = in.readLine();
            if (line == null)
                return false;
            StringTokenizer st = new StringTokenizer(line);
            if (st.hasMoreTokens() && st.nextToken().equals("STARTCHAR")) {
                if (!st.hasMoreTokens())
                    throw new IOException("BDF format error: no character name after STARTCHAR");
                break;
            }
        } while (true);

        // Find properties
        final int UNSET = Integer.MIN_VALUE;
        int codePoint = UNSET;
        int bbx = UNSET;
        int bby = UNSET;
        int bbox = UNSET;
        int bboy = UNSET;
        int dwidth = UNSET;
        do {
            line = in.readLine();
            if (line == null)
                return false;
            StringTokenizer st = new StringTokenizer(line);
            if (st.hasMoreTokens()) {
                String field = st.nextToken();
                if (field.equals("ENCODING")) {
                    if (!st.hasMoreTokens())
                        throw new IOException("BDF format error: no encoding # after ENCODING");
                    String codePointStr = st.nextToken();
                    codePoint = Integer.parseInt(codePointStr);
                } else if (field.equals("BBX")) {
                    if (!st.hasMoreTokens())
                        throw new IOException("BDF format error: no tokens after BBX");
                    bbx = Integer.parseInt(st.nextToken());
                    bby = Integer.parseInt(st.nextToken());
                    bbox = Integer.parseInt(st.nextToken());
                    bboy = Integer.parseInt(st.nextToken());
                } else if (field.equals("DWIDTH")) {
                    if (!st.hasMoreTokens())
                        throw new IOException("BDF format error: no tokens after DWIDTH");
                    dwidth = Integer.parseInt(st.nextToken());
                    int dwidthY = Integer.parseInt(st.nextToken());
                    // The DWIDTH Y value should be zero for any normal font.
                    if (dwidthY != 0) {
                        throw new IOException("BDF format error: dwidth Y value" +
                                              "is nonzero (" + dwidthY + ") " +
                                              "for char " + codePoint + ".");
                    }
                } else if (field.equals("BITMAP")) {
                    break; // now read the bitmap
                }
            }
        } while (true);

        if (codePoint == UNSET)
            throw new IOException("BDF format error: " +
                                  "no code point set");
        if (bbx == UNSET || bby == UNSET)
            throw new IOException("BDF format error: " +
                                  "bbx/bby missing: " + bbx + ", " + bby +
                                  " for char " + codePoint);

        if (bbox == UNSET || bboy == UNSET)
            throw new IOException("BDF format error: " +
                                  "bbox/bboy missing: " + bbox + ", " + bboy +
                                  " for char " + codePoint);

        if (dwidth == UNSET)
            throw new IOException("BDF format error: " +
                                  "dwidth missing for char " + codePoint);

        final int glyphWidth = bbx;
        final int glyphHeight = bby;
        if (glyphWidth > maxCharWidth)
            maxCharWidth = glyphWidth;
        if (glyphHeight > maxCharHeight)
            maxCharHeight = glyphHeight;

        // Read the bitmap
        Glyph glyph = new Glyph(codePoint, glyphWidth, glyphHeight, bbox, bboy, dwidth);
        for (int y = 0; y < glyphHeight; y++) {
            line = in.readLine();
            if (line == null)
                return false;
            for (int b = 0; b < line.length(); b++) {
                int v = Integer.parseInt(Character.toString(line.charAt(b)), 16);
                for (int x = b * 4, i = 0; i < 4 && x < glyphWidth; x++, i++) {
                    boolean set = (v & 0x8) != 0;
                    v <<= 1;
                    glyph.setPixel(x, y, set);
                }
            }
        }

        font.putGlyph(codePoint, glyph);
        return true;
    }
}
