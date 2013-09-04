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
* Filename: LLGUIManager.h
* -----------------------------------------------------------------------------
* Notes:    do every stuff related GUI. for now, we do not use it yet
*			need to come up with generic interface to control GUI
*			how to handle gui related event?
* -----------------------------------------------------------------------------
*/

#ifndef __LLGUIMANAGER_H_
#define __LLGUIMANAGER_H_

#include <CEGUI.h>

class LLGUIManager
{
public:
	LLGUIManager();
	~LLGUIManager();

	// Visibility
	void	setVisible(char* name, bool visible);
	void	show(char* name);
	void	hide(char* name);

	// Text contents
	void	setText(char* name, char* text);
	void	appendText(char* name, char* text);

	// retreive gui window
	CEGUI::Window*	findWindow(char* name);

protected:

};

#endif
