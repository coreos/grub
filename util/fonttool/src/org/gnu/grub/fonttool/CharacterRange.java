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

/**
 * @author chaac
 *
 */
public class CharacterRange {
	private int start;
	private int end;
	
	public CharacterRange(int start, int end) {
		this.start = start;
		this.end = end;
	}

	public int getStart() {
		return start;
	}

	public int getEnd() {
		return end;
	}
	
	protected boolean isCombinable(CharacterRange range) {
		if (getStart() <= range.getStart() && range.getStart() <= getEnd())
			return true;
		
		if (range.getStart() <= getStart() && getStart() <= range.getEnd())
			return true;

		return false;
	}
	
	public boolean isWithinRange(int value) {
		if (value >= start && value <= end)
			return true;
		
		return false;
	}
	
	public boolean combine(CharacterRange range) {
		int start;
		int end;
		
		if (! isCombinable(range))
			return false;
		
		start = getStart();
		if (range.getStart() < start)
			start = range.getStart();
		
		end = getEnd();
		if (range.getEnd() > end)
			end = range.getEnd();
		
		return true;
	}
}
