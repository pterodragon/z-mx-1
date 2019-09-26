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

// ZLib version

#ifndef VERSION_H
#define VERSION_H

#define Z_VULONG(major, minor, build) (((major)<<24) | ((minor)<<16) | (build))

#define Z_VERSION Z_VULONG(8,21,0)	// <-- manually maintained
#define Z_VERNAME "8.21.0"		// <-- ''
#define Z_VERMSRC 8,21,0,0		// <-- ''

#define Z_VMAJOR(n) (((unsigned long)n)>>24)
#define Z_VMINOR(n) ((((unsigned long)n)>>16) & 0xff)
#define Z_VPATCH(n) (((unsigned long)n) & 0xffff)

#endif /* VERSION_H */
