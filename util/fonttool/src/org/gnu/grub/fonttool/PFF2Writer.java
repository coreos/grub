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

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.util.List;

// TODO Add DEFLATE compressed blocks of characters.
public class PFF2Writer {
    private RandomAccessFile f;
    private String currentSection;
    private long currentSectionStart;

    public PFF2Writer(String filename) throws FileNotFoundException {
        this.f = new RandomAccessFile(filename, "rw");
        this.currentSection = null;
        this.currentSectionStart = -1;
    }

    public void writeFont(Font font, List<CharacterRange> rangeList) throws IOException {
        // Clear existing file.
    	f.setLength(0);
    	
        // Write file type ID header.
        writeSection(PFF2Sections.FILE, "PFF2");

        writeSection(PFF2Sections.FONT_NAME, font.getStandardName());
        writeSection(PFF2Sections.FONT_FAMILY, font.getFamily());
        writeSection(PFF2Sections.FONT_WEIGHT, font.isBold() ? "bold" : "normal");
        writeSection(PFF2Sections.FONT_SLANT, font.isItalic() ? "italic" : "normal");
        if (font.getPointSize() != Font.UNKNOWN_POINT_SIZE)
            writeShortSection(PFF2Sections.FONT_POINT_SIZE, font.getPointSize());

        // Construct character definitions.
        CharDefs charDefs = new CharDefs(font);
        charDefs.buildDefinitions(rangeList);

        // Write max character width and height metrics.
        writeShortSection(PFF2Sections.MAX_CHAR_WIDTH, charDefs.getMaxCharWidth());
        writeShortSection(PFF2Sections.MAX_CHAR_HEIGHT, charDefs.getMaxCharHeight());
        writeShortSection(PFF2Sections.FONT_ASCENT, font.getAscent());
        writeShortSection(PFF2Sections.FONT_DESCENT, font.getDescent());

        // Write character index with pointers to the character definitions.
        beginSection(PFF2Sections.CHAR_INDEX);

        // Determine the size of the index, so we can properly refer to the
        // character definition offset in the index.  The actual number of
        // bytes written is compared to the calculated value to ensure we
        // are correct.
        final int indexStart = (int) f.getFilePointer();
        final int calculatedIndexLength =
                charDefs.getIndexSize() * (4 + 1 + 4);
        final int charDefStart = indexStart + calculatedIndexLength + 8;

        for (CharStorageInfo storageInfo : charDefs.getCharIndex().values()) {
            f.writeInt(storageInfo.getCodePoint());
            f.writeByte(0);   // Storage flags: bits 1..0 = 00b : uncompressed.
            f.writeInt(charDefStart + storageInfo.getFileOffset());
        }

        final int indexEnd = (int) f.getFilePointer();
        if (indexEnd - indexStart != calculatedIndexLength) {
            throw new RuntimeException("Incorrect index length calculated, calc="
                                       + calculatedIndexLength
                                       + " actual=" + (indexEnd - indexStart));
        }
        endSection(PFF2Sections.CHAR_INDEX);

        f.writeBytes(PFF2Sections.REMAINDER_IS_DATA);
        f.writeInt(-1);     // Data takes up the rest of the file.
        f.write(charDefs.getDefinitionData());

        f.close();
    }

    private void beginSection(String sectionName) throws IOException {
        verifyOkToBeginSection(sectionName);

        f.writeBytes(sectionName);
        f.writeInt(-1);     // Placeholder for the section length.
        currentSection = sectionName;
        currentSectionStart = f.getFilePointer();
    }

    private void endSection(String sectionName) throws IOException {
        verifyOkToEndSection(sectionName);

        long sectionEnd = f.getFilePointer();
        long sectionLength = sectionEnd - currentSectionStart;
        f.seek(currentSectionStart - 4);
        f.writeInt((int) sectionLength);
        f.seek(sectionEnd);
        currentSection = null;
        currentSectionStart = -1;
    }

    private void verifyOkToBeginSection(String sectionName) {
        if (sectionName.length() != 4)
            throw new IllegalArgumentException(
                    "Section names must be 4 characters: `" + sectionName + "'.");
        if (currentSection != null)
            throw new IllegalStateException(
                    "Attempt to start `" + sectionName
                    + "' section before ending the previous section `"
                    + currentSection + "'.");
    }

    private void verifyOkToEndSection(String sectionName) {
        if (sectionName.length() != 4)
            throw new IllegalStateException("Invalid section name '" + sectionName
                                            + "'; must be 4 characters.");
        if (currentSection == null)
            throw new IllegalStateException(
                    "Attempt to end section `" + sectionName
                    + "' when no section active.");
        if (!sectionName.equals(currentSection))
            throw new IllegalStateException(
                    "Attempt to end `" + sectionName
                    + "' section during active section `"
                    + currentSection + "'.");
    }

    private void writeSection(String sectionName, String contents) throws IOException {
        verifyOkToBeginSection(sectionName);
        f.writeBytes(sectionName);
        f.writeInt(contents.length());
        f.writeBytes(contents);
    }

    private void writeShortSection(String sectionName, int value) throws IOException {
        verifyOkToBeginSection(sectionName);
        f.writeBytes(sectionName);
        f.writeInt(2);
        f.writeShort(value);
    }
}
