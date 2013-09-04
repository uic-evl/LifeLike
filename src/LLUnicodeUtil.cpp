/******************************************************************************
 *
 * LifeLike - LifeLike computer interfaces
 * Copyright (C) 2007 Sangyoon Lee, Electronic Visualization Laboratory, 
 * University of Illinois at Chicago
 *
 * This software is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either Version 2.1 of the License, or 
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public 
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser Public License along
 * with this software; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Questions or comments about LifeLike should be directed to 
 * cavern@evl.uic.edu
 *
 *****************************************************************************/

/*
* -----------------------------------------------------------------------------
* Filename: LLUniCodeUtil.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#include "LLUnicodeUtil.h"

#ifdef _WINDOWS
//-----------------------------------------------------------------------------
// convert char to unicode
bool ConvertStringToUnicode(const char* cstr, wchar_t* wstr, int size)
{
	// validate strings
	if(!cstr || cstr[0] == '\0') return false;
	if(!wstr || size <= 0) return false;

	// count number of elements to convert
	int elem = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cstr, -1, NULL, 0);
	if(!elem) return false;

	// convert string, clamping elem if buffer is size too small
	if(size < elem) elem = size;
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cstr, -1, wstr, elem);
	return true;
}
#endif
