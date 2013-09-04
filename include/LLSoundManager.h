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
* Filename: LLSoundManager.h
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#ifndef __LLSOUNDMANAGER_H_
#define __LLSOUNDMANAGER_H_

#include <vector>
#include "LLSound.h"

typedef std::vector <LLSound*>		iSound_Vec;

//-----------------------------------------------------------------------------
//  LLSoundManager
//-----------------------------------------------------------------------------
class LLSoundManager
{
public:
	// -------------------------------------------------------------------------
	// Constructors and Destructor
	// -------------------------------------------------------------------------
	LLSoundManager();
	~LLSoundManager();

	void	initSoundSystem();
	void	loadSoundSource(char* source, bool bg=false);
	void	update(float addedTime);

	void	playBGSound(int id = 0, bool play = true);
	void	playEffectSound(int id = 0, bool play = true);
	void	setSoundAnimation(int id = 0, float start=0, float end=1, float duration=1);
	void	stopAllSound();

	// event handler for animation and fft speech
	bool	eventHandler(void *param);

protected:
	
	iSound_Vec		m_vEffectSoundVec;
	iSound_Vec		m_vBGSoundVec;
	bool			m_bInitialized;

#ifdef _WINDOWS
	ALCcontext*		m_pSoundContext;
	ALCdevice*		m_pSoundDevice;
#endif

};

#endif
