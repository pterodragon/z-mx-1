/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

// MxT Version

#ifndef VERSION_H
#define VERSION_H

#define MXT_VULONG(major, minor, build) \
	((((unsigned long)(major))<<24) | \
	 (((unsigned long)(minor))<<16) | \
	 ((unsigned long)(build)))

#define MXT_VERSION MXT_VULONG(2,2,1)	// <-- manually maintained
#define MXT_VERNAME "2.2.1"		// <-- ''
#define MXT_VERMSRC 2,2,1,0		// <-- ''

#define MXT_VMAJOR(n) (((unsigned long)n)>>24)
#define MXT_VMINOR(n) ((((unsigned long)n)>>16) & 0xff)
#define MXT_VPATCH(n) (((unsigned long)n) & 0xffff)

#endif /* VERSION_H */
