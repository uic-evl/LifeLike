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
* Filename: LLSoundManager.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/
#include <stdio.h>
#include "LLSoundManager.h"
#include "LLdefine.h"

//-----------------------------------------------------------------------------
LLSoundManager::LLSoundManager()
{
	m_bInitialized = false;

#ifdef _WINDOWS
	m_pSoundDevice = NULL;
	m_pSoundContext = NULL;
#endif

	initSoundSystem();
}

//-----------------------------------------------------------------------------
LLSoundManager::~LLSoundManager(void)
{
	if (!m_bInitialized)
		return;

	//shutdown sound
	int isound;
	isound = (int)m_vBGSoundVec.size();
	for (int i=0; i<isound; i++)
	{
		LLSound* ls = m_vBGSoundVec[i];
		delete ls;
		m_vBGSoundVec[i] = NULL;
	}
	isound = (int)m_vEffectSoundVec.size();
	for (int i=0; i<isound; i++)
	{
		LLSound* ls = m_vEffectSoundVec[i];
		delete ls;
		m_vEffectSoundVec[i] = NULL;
	}

	m_vBGSoundVec.clear();
	m_vEffectSoundVec.clear();

#ifdef _WINDOWS
	// clean up device and context
	if (alcGetCurrentContext() != NULL)
	{
		// clean up stuff here
	}
	if (m_pSoundContext)
	{
		alcMakeContextCurrent(NULL);
		alcDestroyContext(m_pSoundContext);
	}
	if (m_pSoundDevice)
		alcCloseDevice(m_pSoundDevice);

#endif
}

//-----------------------------------------------------------------------------
void LLSoundManager::initSoundSystem()
{
#ifdef _WINDOWS
	// changed to share OpenAL context with others
	// especially if any plugin uses OpenAL, just play nice with that
	ALCcontext*	pOldContext = alcGetCurrentContext();
	if (pOldContext==NULL)
	{
		m_pSoundDevice = alcOpenDevice("DirectSound3D");
		if (m_pSoundDevice)
		{
			m_pSoundContext=alcCreateContext(m_pSoundDevice,NULL);
			if (m_pSoundContext==NULL)
			{
				alcCloseDevice(m_pSoundDevice);
				return;
			}
			alcMakeContextCurrent(m_pSoundContext);
			m_bInitialized = true;
		}
	}
	else
		m_bInitialized = true;

	// Clear Error Code
	ALuint e = alGetError();
#endif	
}

//-----------------------------------------------------------------------------
void LLSoundManager::update(float addedTime)
{
	int isound;
	isound = (int)m_vBGSoundVec.size();
	for (int i=0; i<isound; i++)
		m_vBGSoundVec[i]->update(addedTime);
	isound = (int)m_vEffectSoundVec.size();
	for (int i=0; i<isound; i++)
		m_vEffectSoundVec[i]->update(addedTime);
	
}

//-----------------------------------------------------------------------------
void LLSoundManager::loadSoundSource(char * sourcename, bool bg)
{
	if(!m_bInitialized)
		return;

	LLSound* tSound;
	tSound = new LLSound();
	if (bg)
	{
		tSound->loadSound(sourcename, true);
		m_vBGSoundVec.push_back(tSound);
	}
	else
	{
		tSound->loadSound(sourcename, false);
		m_vEffectSoundVec.push_back(tSound);
	}

}

//-----------------------------------------------------------------------------
void LLSoundManager::playBGSound(int id, bool play)
{
	if(!m_bInitialized || id > (int)m_vBGSoundVec.size() - 1)
		return;

	if (play)
		m_vBGSoundVec[id]->playSound();
	else
		m_vBGSoundVec[id]->stopSound();

}

//-----------------------------------------------------------------------------
void LLSoundManager::playEffectSound(int id, bool play)
{

	if(!m_bInitialized || id > (int)m_vEffectSoundVec.size() - 1)
		return;

	// 
	if (play)
		m_vEffectSoundVec[id]->playSound();
	else
		m_vEffectSoundVec[id]->stopSound();

}

//-----------------------------------------------------------------------------
void LLSoundManager::setSoundAnimation(int id, float start, float end, float duration)
{
	if(!m_bInitialized || id > (int)m_vBGSoundVec.size() - 1)
		return;

	m_vBGSoundVec[id]->setSoundChange(start, end, duration);
}

//-----------------------------------------------------------------------------
void LLSoundManager::stopAllSound()
{
	int isound = (int)m_vEffectSoundVec.size();
	for (int i=0; i<isound; i++)
		m_vEffectSoundVec[i]->stopSound();

}

//-----------------------------------------------------------------------------
bool LLSoundManager::eventHandler(void *param)
{
	LLEvent* se = (LLEvent*)param;

	// this event must be sound related type
	if (se->type != EVT_SOUND)
		return false;

	// play sound
	playEffectSound(se->ids[0]);

	return true;
}
