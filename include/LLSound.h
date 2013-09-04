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
* Filename: LLSound.h
* -----------------------------------------------------------------------------
* Notes:    Sound class - simple sound source playback
* -----------------------------------------------------------------------------
*/

#ifndef __LLSOUND_H__
#define __LLSOUND_H__


#ifdef _WINDOWS
#include <al.h>
#include <AL/alut.h>
#include <alc.h>
#endif

//sound class
class LLSound
{
public:
	LLSound();
	~LLSound();

	void loadSound(char fname[40], bool looping=false);
	void setProperties(float x, float y, float z, float vx, float vy, float vz);
	void setSourceRelative();
	void playSound();
	void stopSound();
	void changeGain(float value);
	void setLoop(bool loop);
	void update(float addedTime);
	void setSoundChange(float start = 0, float end = 1, float duration = 1);

private:

	void destroySound();

	char*			alBuffer;
#ifdef _WINDOWS
	ALenum			alFormatBuffer;
	ALsizei			alFreqBuffer;
	ALsizei			alBufferLen;
	ALboolean		alLoop;
#endif
	unsigned int	alSource;
	unsigned int	alSampleSet;

	float			m_fMaxGain;
	float			m_fCurrGain;
	float			m_fDeltaGain;
	bool			m_bChangingGain;
};

#endif
