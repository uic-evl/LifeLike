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
* Filename: LLFFTSound.h
* -----------------------------------------------------------------------------
* Notes:    Sound class - FFT analysis for lipsynch
* -----------------------------------------------------------------------------
*/

#ifndef __LLFFTSOUND_H__
#define __LLFFTSOUND_H__

#ifdef _WINDOWS
#include <windows.h>
#endif

#include <vector>


#include "LLCallBack.h"
#include "LLCharacter.h"

#ifndef BANDS
#define BANDS 28
#endif

enum tEVENT;

typedef std::vector <LLEvent*>	iSoundEvent_Vec;

//sound class
class LLFFTSound
{
public:
	LLFFTSound();
	~LLFFTSound();

	bool	loadSound(const char* filename, const char* speech, bool looping=false);
	void	playSound();
	void	stopSound();
	void	setLoop(bool loop);
	void	update(float addedTime);

	float	getLevel(int channel = 0);
	float	getLevelLeft() { return getLevel(0);}
	float	getLevelRight() { return getLevel(1);}
	float	getBand(int bid);
	bool	isPlaying() { return m_bPlaying;}
	
	char*	getSpeech() { return m_Speech;}
	
	void	addEvent(float etime, tEVENT type, const char* param);
	void	resetEvent();

	TCallback<LLCharacter> i_CharacterEventCallback;

private:
	
	void destroySound();

	char	m_Speech[256];
	bool	m_bInitialized;
	bool	m_bPlaying;
	DWORD	m_dChannel;
	float	m_fCurrLevelLeft;
	float	m_fCurrLevelRight;
	float	m_fBands[BANDS];

	iSoundEvent_Vec	m_vEventVec;
	int				m_iNumEvents;
	int				m_iNextEvent;
	float			m_fNextEventTime;
	bool			m_bEventDone;
};

#endif
