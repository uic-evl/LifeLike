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
* Filename: LLGUIManager.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#include "LLGUIManager.h"

// ----------------------------------------------------------------------------
LLGUIManager::LLGUIManager()
{
	// initialize gui components
	
	// initialize scheme

	// ogre tray for stats

	// usual gui components?
	// microphone, debug overlay, tooltip(caption)
	// 
}

// ----------------------------------------------------------------------------
LLGUIManager::~LLGUIManager()
{
	// any clean up necessary?

}

// ----------------------------------------------------------------------------
void LLGUIManager::setVisible(char* name, bool visible)
{
	// find gui component and set visibility
	CEGUI::Window* win = findWindow(name);
	if (!win)
		return;

}


// ----------------------------------------------------------------------------
void LLGUIManager::show(char* name)
{
	// find gui component and 
	CEGUI::Window* win = findWindow(name);
	if (!win)
		return;

}

// ----------------------------------------------------------------------------
void LLGUIManager::hide(char* name)
{
	// find gui component and 
	CEGUI::Window* win = findWindow(name);
	if (!win)
		return;

}


// ----------------------------------------------------------------------------
void LLGUIManager::setText(char* name, char* text)
{
	// find gui component and 
	CEGUI::Window* win = findWindow(name);
	if (!win)
		return;

}

// ----------------------------------------------------------------------------
void LLGUIManager::appendText(char* name, char* text)
{
	// find gui component and 
	CEGUI::Window* win = findWindow(name);
	if (!win)
		return;

}

// ----------------------------------------------------------------------------
CEGUI::Window* LLGUIManager::findWindow(char* name)
{
	CEGUI::Window* win = NULL;
	
	try
	{
		win = CEGUI::WindowManager::getSingleton().getWindow(name);
	}
	catch (CEGUI::UnknownObjectException  e)
	{
		// well do nothing here
	}

	return win;
}
