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
* Filename: LLFFTSound.cpp
* -----------------------------------------------------------------------------
* Notes:    Sound class - FFT analysis for lipsynch
* -----------------------------------------------------------------------------
*/


#pragma warning(disable:4244)

#include <stdio.h>
#include <math.h>
#include <malloc.h>

#ifdef _WINDOWS
#include "bass.h"
#endif

#include "LLFFTSound.h"
#include "LLdefine.h"

static bool bBassInit = false;

//-----------------------------------------------------------------------------
LLFFTSound::LLFFTSound()
{
	m_bInitialized = false;
	m_bPlaying = false;
	m_fCurrLevelLeft = 0.0f;
	m_fCurrLevelRight = 0.0f;
	memset(m_Speech, 0, 256);
	for (int i=0; i< BANDS; i++)
		m_fBands[i] = 0.0f;

	m_iNumEvents = 0;
	m_iNextEvent = -1;
	m_fNextEventTime = 0.0f;
	m_bEventDone = false;

}

//-----------------------------------------------------------------------------
LLFFTSound::~LLFFTSound()
{
	destroySound();

#ifdef _WINDOWS
	BASS_Free();
#endif	
	// clean up event vector
	int size = (int)m_vEventVec.size();
	for (int i=0; i<size; i++)
	{
		LLEvent* se = m_vEventVec[i];
		delete se;
		m_vEventVec[i] = NULL;
	}
	
	m_vEventVec.clear();

}

//-----------------------------------------------------------------------------
bool LLFFTSound::loadSound(const char* filename, const char* speech, bool looping)
{
	// set speech contents
	strcpy(m_Speech, speech);

#ifdef _WINDOWS
	// initialize BASS
	if (!bBassInit)
	{
		if (!BASS_Init(-1,44100,0,0,NULL)) {
			//Error("Can't initialize device");
			return false;
		}
		bBassInit = true;
	}
	

	if (looping)
	{
		if (!(m_dChannel=BASS_StreamCreateFile(FALSE,filename,0,0,BASS_SAMPLE_LOOP))
			&& !(m_dChannel=BASS_MusicLoad(FALSE,filename,0,0,BASS_MUSIC_RAMP|BASS_SAMPLE_LOOP,0))) {

			return false; // Can't load the file
		}
	}
	else
	{
		if (!(m_dChannel=BASS_StreamCreateFile(FALSE,filename,0,0,0))
			&& !(m_dChannel=BASS_MusicLoad(FALSE,filename,0,0,BASS_MUSIC_RAMP|BASS_SAMPLE_LOOP,0))) {

			return false; // Can't load the file
		}
	}
	
	m_bInitialized = true;
	return true;
#else
	return false;
#endif
	
}

//-----------------------------------------------------------------------------
void LLFFTSound::playSound()
{
	if(!m_bInitialized)
		return;

#ifdef _WINDOWS
	BASS_ChannelPlay(m_dChannel,TRUE);
#endif
	m_bPlaying = true;
}

//-----------------------------------------------------------------------------
void LLFFTSound::stopSound()
{
	if(!m_bInitialized || !m_bPlaying)
		return;

#ifdef _WINDOWS
	BASS_ChannelStop(m_dChannel);
#endif
	m_bPlaying = false;

	// reset events queue
	resetEvent();
}

//-----------------------------------------------------------------------------
void LLFFTSound::destroySound()
{

}

//-----------------------------------------------------------------------------
void LLFFTSound::setLoop(bool loop)
{
	if (loop == false)
		;
	else
		;
	
}

//-----------------------------------------------------------------------------
void LLFFTSound::addEvent(float etime, tEVENT type, const char* param)
{
	LLEvent* se = new LLEvent;
	se->eTime = etime;
	se->type = type;
	char pstr[128];
	strcpy(pstr, param);

	// several special case
	strcpy(se->param, param);
	if (type == EVT_ACTION)
	{
		se->ids[0] = CAT_IDLE;			// just default

		if (strcmp(param, "CAT_IDLE") == 0)
			se->ids[0] = CAT_IDLE;
		else if (strcmp(param, "CAT_READ") == 0)
			se->ids[0] = CAT_READ;
		else if (strcmp(param, "CAT_LISTEN") == 0)
			se->ids[0] = CAT_LISTEN;
		else if (strcmp(param, "CAT_SPEAK") == 0)
			se->ids[0] = CAT_SPEAK;
		else if (strcmp(param, "CAT_WRITE") == 0)
			se->ids[0] = CAT_WRITE;
		else if (strcmp(param, "CAT_DRINK") == 0)
			se->ids[0] = CAT_DRINK;

	}
	else if (type == EVT_SHAPE)
	{
		// shape id, duration, max, mid, maxduration
		// need to tokenize
		char *p_token;
		char seps[] = " ,\t\n";
		p_token = strtok(pstr, seps);
		se->ids[1] = atof(p_token);		// id
		
		// parameters: duration, max, mid, maxduration
		p_token = strtok(NULL, seps);
		int idx = 0;
		while(p_token != NULL)
		{
			se->args[idx] = atof(p_token);
			p_token = strtok(NULL, seps);
			idx++;
		}
		
		// a bit of validation
		if (se->args[0] == 0.0f || se->args[2] > se->args[0] || se->args[0] < se->args[3])
		{
			delete se;
			return;
		}
	}
	else if (type == EVT_SOUND)
	{
		se->ids[0] = atoi(pstr);
	}
	else if (type == EVT_ATTRACT)
	{
		// param has x,y coordinate
		char *p_token;
		char seps[] = " ,\t\n";
		p_token = strtok(pstr, seps);
		se->args[0] = atof(p_token);	// x coord
		p_token = strtok(NULL, seps);
		se->args[1] = atof(p_token);	// y coord
		p_token = strtok(NULL, seps);
		se->args[2] = atof(p_token);	// speed
	}
	
	m_vEventVec.push_back(se);

	m_iNumEvents++;
	
	if (m_iNextEvent == -1)
	{
		m_iNextEvent = 0;
		m_fNextEventTime = etime;
	}

}

//-----------------------------------------------------------------------------
void LLFFTSound::resetEvent()
{
	if (m_iNumEvents)
	{
		m_iNextEvent = 0;
		m_fNextEventTime = m_vEventVec[m_iNextEvent]->eTime;
		m_bEventDone = false;
	}

}

//-----------------------------------------------------------------------------
float LLFFTSound::getLevel(int channel)
{
	if (channel == 0)
		return m_fCurrLevelLeft;
	else if (channel == 1)
		return m_fCurrLevelRight;
}

//-----------------------------------------------------------------------------
float LLFFTSound::getBand(int bid)
{
	if (bid > BANDS -1)
		return 0.0f;
	else
		return m_fBands[bid];
}

//-----------------------------------------------------------------------------
void LLFFTSound::update(float addedTime)
{
	if(!m_bInitialized || !m_bPlaying)
		return;

#ifdef _WINDOWS
	
	// update current waveform level
#if BASSVERSION == 0x204
	DWORD pos = BASS_ChannelGetPosition(m_dChannel, BASS_POS_BYTE);
	if (pos == BASS_ChannelGetLength(m_dChannel, BASS_POS_BYTE))
#else
	DWORD pos = BASS_ChannelGetPosition(m_dChannel);
	if (pos == BASS_ChannelGetLength(m_dChannel))
#endif
	{
		m_fCurrLevelLeft = 0.0f;
		m_fCurrLevelRight = 0.0f;
		m_bPlaying = false;
		BASS_ChannelStop(m_dChannel);
		// reset bands values
		for (int i=0; i< BANDS; i++)
			m_fBands[i] = 0.0f;

		// reset events queue
		resetEvent();
	}
	else
	{
		// update lever for each channel
		DWORD level = BASS_ChannelGetLevel(m_dChannel);
		m_fCurrLevelLeft = ((double)LOWORD(level) / 32768);		// range 0 to 1: left channel is low word
		m_fCurrLevelRight = ((double)HIWORD(level) / 32768);	// range 0 to 1: right channel is high word

		// update band spectrum
		float fft[1024];
		BASS_ChannelGetData(m_dChannel,fft,BASS_DATA_FFT2048); // get the FFT data

		int b0=0;
		int x,y;
#define FFTCAP 256.0f

		for (x=0; x<BANDS; x++) {
			float sum=0;
			int sc,b1 = pow(2,x*10.0/(BANDS-1));
			if (b1>1023) b1=1023;
			if (b1<=b0) b1=b0+1; // make sure it uses at least 1 FFT bin
			sc = 10 + b1 - b0;
			
			for (;b0<b1;b0++) sum+=fft[1+b0];
			
			y=(sqrt(sum/log10((double)sc))*1.7*FFTCAP)-4; // scale it
			
			if (y>FFTCAP) y=FFTCAP; // cap it
			
			// store band value
			m_fBands[x] = y;	
			m_fBands[x] /= FFTCAP;	// normalized band value (0.0 - 1.0)
		}

		// check event associated with this sound file: use pos variable to detect timing...
		float elapsedTime = BASS_ChannelBytes2Seconds(m_dChannel,pos); 

		if (m_iNumEvents && !m_bEventDone)
		{
			if (elapsedTime > 0.0f)
			{
				// there is possibility that multiple events have the same timestamp...
				// should do some kind of iteration.
				bool running = true;
				while(running)
				{
					if (m_fNextEventTime < elapsedTime)
					{
						// execute callback function
						i_CharacterEventCallback.execute(m_vEventVec[m_iNextEvent]);

						// move to next event
						m_iNextEvent++;
						if (m_iNextEvent == m_iNumEvents)
						{
							// reached the last event
							m_iNextEvent = 0;
							m_bEventDone = true;
							running = false;
						}
						m_fNextEventTime = m_vEventVec[m_iNextEvent]->eTime;

					}
					else
						running = false;
				}
			}
		}


	}
#endif
}


