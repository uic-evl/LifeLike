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
* Filename: LLAudioMonitor.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#ifdef _WINDOWS

#pragma warning(disable:4244)

#include "LLAudioMonitor.h"
#include "bass.h"
#include <math.h>

#define SPECWIDTH 256	// display width
#define SPECHEIGHT 256	// height (changing requires palette adjustments too)
#define BUFSTEP 200000	// memory allocation unit

bool	bARecording = false;
char*	pARecBuf = NULL;
DWORD	dARecLen;
HRECORD rAchan=0;		// recording channel

//-----------------------------------------------------------------------------
LLAudioMonitor::LLAudioMonitor(char* devicename)
{
	m_iDeviceId = -1;
	m_fCurrLevelLeft = 0.0f;
	m_fCurrLevelRight = 0.0f;
	m_fSmoothLevelLeft = 0.0f;
	m_fSmoothLevelRight = 0.0f;
	m_bInitialized = initDevice(devicename);

	m_bEnableRecording = false;
}

//-----------------------------------------------------------------------------
LLAudioMonitor::~LLAudioMonitor()
{
	if(m_bInitialized)
		BASS_RecordFree();

	if (pARecBuf) { // free old recording
		free(pARecBuf);
		pARecBuf=NULL;
	}

}

//-----------------------------------------------------------------------------
// Recording callback - not doing anything with the data
BOOL CALLBACK DuffRecordingAudio(HRECORD handle, const void *buffer, DWORD length, void* user)
{
	if (bARecording)
	{
		// increase buffer size if needed
		if ((dARecLen%BUFSTEP)+length>=BUFSTEP) {
			pARecBuf=(char*)realloc(pARecBuf,((dARecLen+length)/BUFSTEP+1)*BUFSTEP);
			if (!pARecBuf) {
				rAchan=0;
				return FALSE; // stop recording
			}
		}
		// buffer the data
		memcpy(pARecBuf+dARecLen,buffer,length);
		dARecLen+=length;
	}

	return TRUE; // continue recording
}

//-----------------------------------------------------------------------------
bool LLAudioMonitor::initDevice(char* devicename)
{
	char* desc;
	int count=0; // the device counter
#if BASSVERSION == 0x204
	while (desc=(char*)BASS_RecordGetInputName(count))
#else
	while (desc=(char*)BASS_RecordGetDeviceDescription(count))
#endif
	{
		if (strcmp(devicename, desc) == 0)
			m_iDeviceId = count;
		count++;
	}

	// just in case fail to find given named recording device and system has something else
	// use the first one instead
	if (m_iDeviceId==-1 && count!=0)
		m_iDeviceId = 0;

#if BASSVERSION == 0x204
	BASS_RecordSetInput(m_iDeviceId, BASS_INPUT_ON, -1);
#else
	BASS_RecordSetInput(m_iDeviceId, BASS_INPUT_ON);
#endif

	// initialize BASS recording (default device)
	//if (!BASS_RecordInit(m_iDeviceId)) {	// mic
	//if (!BASS_RecordInit(2)) {		// stereo mixer "Realtek HD Audio Input"
	if (!BASS_RecordInit(m_iDeviceId)) {		// stereo mixer "Realtek HD Audio Input"
		return false;
	}

	// start recording (44100hz mono 16-bit)
	if (!(m_dChannel=BASS_RecordStart(44100,1,0,&DuffRecordingAudio,0))) {
		return false;
	}

	rAchan = m_dChannel;

	return true;
}

//-----------------------------------------------------------------------------
void LLAudioMonitor::startRecording()
{
	if (!m_bEnableRecording)
	{
		printf("start recording called but not enabled!\n");
		return;
	}

	printf("start recording...\n");
	if (!bARecording)
	{
		WAVEFORMATEX *wf;
		if (pARecBuf) { // free old recording
			free(pARecBuf);
			pARecBuf=NULL;
		}
		// allocate initial buffer and make space for WAVE header
		pARecBuf=(char*)malloc(BUFSTEP);
		dARecLen=44;
		// fill the WAVE header
		memcpy(pARecBuf,"RIFF\0\0\0\0WAVEfmt \20\0\0\0",20);
		memcpy(pARecBuf+36,"data\0\0\0\0",8);
		wf=(WAVEFORMATEX*)(pARecBuf+20);
		wf->wFormatTag=1;
		wf->nChannels=1;
		wf->wBitsPerSample=16;
		wf->nSamplesPerSec=44100;
		wf->nBlockAlign=wf->nChannels*wf->wBitsPerSample/8;
		wf->nAvgBytesPerSec=wf->nSamplesPerSec*wf->nBlockAlign;
		// start recording @ 44100hz 16-bit stereo
		/*if (!(rchan=BASS_RecordStart(44100,2,0,&RecordingCallback,0))) {
			free(pRecBuf);
			pRecBuf=0;
			return;
		}*/
		bARecording = true;

		// wave file name
		struct tm *pTime;
		time_t ctTime; time(&ctTime);
		pTime = localtime( &ctTime );
		sprintf(m_sRecordFileName, "audio_log\\avatar_%i\-%02i\-%02i_%02i\-%02i\-%02i.wav", (1900+pTime->tm_year), (1+pTime->tm_mon), pTime->tm_mday, pTime->tm_hour, pTime->tm_min, pTime->tm_sec);
		printf("audio recoding filename: %s\n", m_sRecordFileName);

	}
	else
		;// this is error: multiple calls of start recording...
}


//-----------------------------------------------------------------------------
void LLAudioMonitor::stopRecording()
{
	if (!m_bEnableRecording)
		return;

	printf("stop recording...\n");
	if (bARecording)
	{
		bARecording = false;
		
		// complete the WAVE header
		*(DWORD*)(pARecBuf+4)=dARecLen-8;
		*(DWORD*)(pARecBuf+40)=dARecLen-44;

		// write out wave file
		FILE *fp;
		if (!(fp=fopen(m_sRecordFileName,"wb"))) {
			//Error("Can't create the file");
			printf("Can't create the file: %s\n", m_sRecordFileName);
			return;
		}
		fwrite(pARecBuf,dARecLen,1,fp);
		fclose(fp);

	}
	else
		;// this is error: calling stop even not recording here
}

//-----------------------------------------------------------------------------
void LLAudioMonitor::update(float addedTime)
{
	if (!m_bInitialized)
		return;

	DWORD level = BASS_ChannelGetLevel(m_dChannel);

	m_fCurrLevelLeft = ((double)LOWORD(level) / 32768);		// range 0 to 1: left channel is low word
	m_fCurrLevelRight = ((double)HIWORD(level) / 32768);	// range 0 to 1: right channel is high word

	m_fSmoothLevelLeft = m_fSmoothLevelLeft + (m_fCurrLevelLeft - m_fSmoothLevelLeft) * 0.2f;
	m_fSmoothLevelRight = m_fSmoothLevelRight + (m_fCurrLevelRight - m_fSmoothLevelRight) * 0.2f;

	updateSpectrum();
}

//-----------------------------------------------------------------------------
float LLAudioMonitor::getAudioLevel(int chan)
{
	if (chan ==0)
		return m_fCurrLevelLeft;
	else
		return m_fCurrLevelRight;
}

//-----------------------------------------------------------------------------
float LLAudioMonitor::getAudioLevelSmooth(int chan)
{
	if (chan ==0)
		return m_fSmoothLevelLeft;
	else
		return m_fSmoothLevelRight;
}

//-----------------------------------------------------------------------------
float LLAudioMonitor::getBand(int bid)
{
	if (bid > BANDS -1)
		return 0.0f;
	else
		return m_fBands[bid];
}

//-----------------------------------------------------------------------------
void LLAudioMonitor::updateSpectrum()
{
	// this updateSpectrum code borrowed from BASS lib example (livespec app)
	int x,y,y1,i,idx,numpix;
	unsigned char r,g;

	{
		float fft[1024];
		BASS_ChannelGetData(m_dChannel,fft,BASS_DATA_FFT2048); // get the FFT data

		int boundmax = (SPECWIDTH*SPECHEIGHT*4-4);
		int b0=0;
		numpix = floor(0.75*(SPECWIDTH/BANDS));
		int steppix = numpix + 2;

		for (x=0;x<BANDS;x++) {
			float sum=0;
			int sc,b1=pow(2,x*10.0/(BANDS-1));
			if (b1>1023) b1=1023;
			if (b1<=b0) b1=b0+1; // make sure it uses at least 1 FFT bin
			sc=10+b1-b0;
			
			for (;b0<b1;b0++) sum+=fft[1+b0];
			
			y=(sqrt(sum/log10((double)sc))*1.7*SPECHEIGHT)-4; // scale it
			
			if (y>SPECHEIGHT) y=SPECHEIGHT; // cap it
			
			// store band value
			m_fBands[x] = y;	
			m_fBands[x] /= (float)SPECHEIGHT;	// normalized band value (0.0 - 1.0)

		}
	}
}

#endif
