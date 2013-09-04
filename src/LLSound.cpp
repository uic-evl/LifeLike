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
* Filename: LLSound.cpp
* -----------------------------------------------------------------------------
* Notes:    Sound class - simple sound source playback
* -----------------------------------------------------------------------------
*/

#pragma warning(disable:4996)

#ifdef _WINDOWS
#include <windows.h>
#endif

#include <math.h>
#include <stdio.h>
//#include <conio.h>
#include <stdlib.h>
#include "LLSound.h"

//-----------------------------------------------------------------------------
LLSound::LLSound()
{
	m_bChangingGain = false;
	m_fCurrGain = 0.0f;
	m_fMaxGain = 1.0f;
	m_fDeltaGain = 0.0f;
}

//-----------------------------------------------------------------------------
LLSound::~LLSound()
{
	destroySound();
}

//-----------------------------------------------------------------------------
void LLSound::loadSound(char fname[40], bool looping)
{
/*
	//load our sound
	ALboolean loop;
	//loop = looping;
	alutLoadWAVFile(fname,&alFormatBuffer, (void **) &alBuffer,&alBufferLen, &alFreqBuffer, &loop);

	alGenSources(1, &alSource);
	alGenBuffers(1, &alSampleSet);
	alBufferData(alSampleSet, alFormatBuffer, alBuffer, alBufferLen, alFreqBuffer);
	alSourcei(alSource, AL_BUFFER, alSampleSet);

	alutUnloadWAV(alFormatBuffer, alBuffer, alBufferLen, alFreqBuffer);			

	//set the pitch
	alSourcef(alSource,AL_PITCH,1.0f);
	//set the gain
	alSourcef(alSource,AL_GAIN,1.0f);
	//set looping to true
	if (looping == 0)
		alSourcei(alSource,AL_LOOPING,AL_FALSE);
	else
		alSourcei(alSource,AL_LOOPING,AL_TRUE);
*/
}

//-----------------------------------------------------------------------------
void LLSound::setProperties(float x, float y, float z, float vx, float vy, float vz)
{
/*
	//set the sounds position and velocity
	alSource3f(alSource,AL_POSITION,x,y,z);
	alSource3f(alSource,AL_VELOCITY,vx,vy,vz);
*/
}

//-----------------------------------------------------------------------------
//this function makes a sound source relative so all direction and velocity
//parameters become relative to the source rather than the listener
//useful for background music that you want to stay constant relative to the listener
//no matter where they go
void LLSound::setSourceRelative()
{
	//alSourcei(alSource,AL_SOURCE_RELATIVE,AL_TRUE);
}

//-----------------------------------------------------------------------------
void LLSound::playSound()
{
	//alSourcePlay(alSource);
}

//-----------------------------------------------------------------------------
void LLSound::stopSound()
{
	//alSourceStop(alSource);
}

//-----------------------------------------------------------------------------
void LLSound::destroySound()
{
	//alDeleteSources(1,&alSource);
	//alDeleteBuffers(1,&alSampleSet);
}

//-----------------------------------------------------------------------------
void LLSound::changeGain(float value)
{
	//alSourcef(alSource,AL_GAIN,value);
}

//-----------------------------------------------------------------------------
void LLSound::setLoop(bool loop)
{
/*
	if (loop == false)
		alSourcei(alSource,AL_LOOPING,AL_FALSE);
	else
		alSourcei(alSource,AL_LOOPING,AL_TRUE);
*/	
}

//-----------------------------------------------------------------------------
void LLSound::update(float addedTime)
{
	if (!m_bChangingGain)
		return;

	float newgain = m_fCurrGain + m_fDeltaGain * addedTime;
	if (m_fDeltaGain>0)
	{
		// volume up
		if (newgain > m_fMaxGain)
		{
			changeGain(m_fMaxGain);
			m_bChangingGain = false;
			m_fCurrGain = m_fMaxGain;
		}
		else
		{
			changeGain(newgain);
			m_fCurrGain = newgain;
		}
	}
	else
	{
		// volume down
		if (newgain < m_fMaxGain)
		{
			changeGain(m_fMaxGain);
			m_bChangingGain = false;
			m_fCurrGain = m_fMaxGain;
			if (m_fCurrGain == 0)
				stopSound();
		}
		else
		{
			changeGain(newgain);
			m_fCurrGain = newgain;
		}
	}
}

//-----------------------------------------------------------------------------
void LLSound::setSoundChange(float start, float end, float duration)
{
	if (m_fCurrGain == 0)
		playSound();

	m_fCurrGain = start;
	m_fMaxGain = end;
	m_fDeltaGain = (end - start) / duration;
	m_bChangingGain = true;
	changeGain(start);
	
}

