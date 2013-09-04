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
* Filename: LLMicMonitor.cpp
* -----------------------------------------------------------------------------
* Notes:    Spectrum drawing routine refers to BASS lib example (livespec)
* -----------------------------------------------------------------------------
*/


#pragma warning(disable:4244)

#include "LLMicMonitor.h"

#ifdef _WINDOWS
#include "bass.h"
#include <mmdeviceapi.h>
#endif

#include <math.h>
#include "LLScreenLog.h"
  

#define SPECWIDTH 256	// display width
#define SPECHEIGHT 256	// height (changing requires palette adjustments too)
#define BUFSTEP 200000	// memory allocation unit

bool	bMRecording = false;
char*	pMRecBuf = NULL;

#ifdef _WINDOWS
DWORD	dMRecLen;
HRECORD rMchan=0;		// recording channel
#endif

//-----------------------------------------------------------------------------
LLMicMonitor::LLMicMonitor()
{
#ifdef _WINDOWS
	m_iDeviceId = -1;
	m_fCurrLevelLeft = 0.0f;
	m_fCurrLevelRight = 0.0f;
	m_iDrawMode = 1;			// mode 0: bar meter, 1: normal fft 2: logarithmic 3: waveform
	m_bInitialized = initDevice();

	m_pImgBuf = new unsigned char[SPECWIDTH * SPECHEIGHT* 4];
	memset(m_pImgBuf, 0, SPECWIDTH * SPECHEIGHT * 4);

	m_pWaveImage = new Image;
	m_pWaveImage->loadDynamicImage((unsigned char*) m_pImgBuf, 
					SPECWIDTH, SPECWIDTH, Ogre::PF_R8G8B8A8);

	m_pWaveTex = TextureManager::getSingleton().loadImage( "MicWaveImage",
					Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, *m_pWaveImage, Ogre::TEX_TYPE_2D );

	m_bEnableRecording = false;

	// mic mute testing...
	m_fMicVolume = 0;
	endpointVolume = NULL;
	CoInitialize(NULL);
	IMMDeviceEnumerator *deviceEnumerator = NULL;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
	IMMDevice *defaultDevice = NULL;

	hr = deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &defaultDevice);
	deviceEnumerator->Release();
	deviceEnumerator = NULL;

	//IAudioEndpointVolume *endpointVolume = NULL;
	hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
	defaultDevice->Release();
	defaultDevice = NULL; 

	endpointVolume->GetMasterVolumeLevel(&m_fMicVolume);

	LLScreenLog::getSingleton().addText("************************* Mic monitor current volumn: " + StringConverter::toString(m_fMicVolume));
#endif
}

//-----------------------------------------------------------------------------
LLMicMonitor::~LLMicMonitor()
{
#ifdef _WINDOWS
	if(m_bInitialized)
		BASS_RecordFree();
	
	m_pWaveTex.setNull();
	delete m_pWaveImage;
	delete [] m_pImgBuf;

	if (pMRecBuf) { // free old recording
		free(pMRecBuf);
		pMRecBuf=NULL;
	}

	if (endpointVolume)
		endpointVolume->Release();

	CoUninitialize();
#endif
}

#ifdef _WINDOWS
//-----------------------------------------------------------------------------
// Recording callback - not doing anything with the data
BOOL CALLBACK DuffRecording(HRECORD handle, const void *buffer, DWORD length, void* user)
{
	if (bMRecording)
	{
		// increase buffer size if needed
		if ((dMRecLen%BUFSTEP)+length>=BUFSTEP) {
			pMRecBuf=(char*)realloc(pMRecBuf,((dMRecLen+length)/BUFSTEP+1)*BUFSTEP);
			if (!pMRecBuf) {
				rMchan=0;
				return FALSE; // stop recording
			}
		}
		// buffer the data
		memcpy(pMRecBuf+dMRecLen,buffer,length);
		dMRecLen+=length;
	}

	return TRUE; // continue recording
}
#endif

//-----------------------------------------------------------------------------
bool LLMicMonitor::initDevice()
{
#ifdef _WINDOWS
	// initialize BASS recording (default device)
	if (!BASS_RecordInit(m_iDeviceId)) {	// mic
	//if (!BASS_RecordInit(2)) {		// stereo mixer
		return false;
	}

	// start recording (44100hz mono 16-bit)
	if (!(m_dChannel=BASS_RecordStart(44100,1,0,&DuffRecording,0))) {
		return false;
	}

	rMchan = m_dChannel;
#endif
	return true;
}

//-----------------------------------------------------------------------------
void LLMicMonitor::startRecording()
{
#ifdef _WINDOWS
	if (!m_bEnableRecording)
	{
		printf("start recording called but not enabled!\n");
		return;
	}

	printf("start recording...\n");
	if (!bMRecording)
	{
		WAVEFORMATEX *wf;
		if (pMRecBuf) { // free old recording
			free(pMRecBuf);
			pMRecBuf=NULL;
		}
		// allocate initial buffer and make space for WAVE header
		pMRecBuf=(char*)malloc(BUFSTEP);
		dMRecLen=44;
		// fill the WAVE header
		memcpy(pMRecBuf,"RIFF\0\0\0\0WAVEfmt \20\0\0\0",20);
		memcpy(pMRecBuf+36,"data\0\0\0\0",8);
		wf=(WAVEFORMATEX*)(pMRecBuf+20);
		wf->wFormatTag=1;
		wf->nChannels=1;
		wf->wBitsPerSample=16;
		wf->nSamplesPerSec=44100;
		wf->nBlockAlign=wf->nChannels*wf->wBitsPerSample/8;
		wf->nAvgBytesPerSec=wf->nSamplesPerSec*wf->nBlockAlign;
		bMRecording = true;

		// wave file name
		struct tm *pTime;
		time_t ctTime; time(&ctTime);
		pTime = localtime( &ctTime );
		sprintf(m_sRecordFileName, "log\\user_%i\-%02i\-%02i_%02i\-%02i\-%02i.wav", (1900+pTime->tm_year), (1+pTime->tm_mon), pTime->tm_mday, pTime->tm_hour, pTime->tm_min, pTime->tm_sec);
		printf("audio recoding filename: %s\n", m_sRecordFileName);

	}
	else
		;// this is error: multiple calls of start recording...
#endif
}


//-----------------------------------------------------------------------------
void LLMicMonitor::stopRecording()
{
#ifdef _WINDOWS
	if (!m_bEnableRecording)
		return;

	printf("stop recording...\n");
	if (bMRecording)
	{
		bMRecording = false;
		
		// complete the WAVE header
		*(DWORD*)(pMRecBuf+4)=dMRecLen-8;
		*(DWORD*)(pMRecBuf+40)=dMRecLen-44;

		// write out wave file
		FILE *fp;
		if (!(fp=fopen(m_sRecordFileName,"wb"))) {
			printf("Can't create the file: %s\n", m_sRecordFileName);
			return;
		}
		fwrite(pMRecBuf,dMRecLen,1,fp);
		fclose(fp);

	}
	else
		;// this is error: calling stop even not recording here
#endif
}

//-----------------------------------------------------------------------------
void LLMicMonitor::update(float addedTime)
{
#ifdef _WINDOWS
	if (!m_bInitialized)
		return;

	DWORD level = BASS_ChannelGetLevel(m_dChannel);

	m_fCurrLevelLeft = ((double)LOWORD(level) / 32768);		// range 0 to 1: left channel is low word
	m_fCurrLevelRight = ((double)HIWORD(level) / 32768);	// range 0 to 1: right channel is high word

	updateSpectrum();
#endif
}

//-----------------------------------------------------------------------------
float LLMicMonitor::getMicLevel(int chan)
{
	if (chan ==0)
		return m_fCurrLevelLeft;
	else
		return m_fCurrLevelRight;
}

//-----------------------------------------------------------------------------
float LLMicMonitor::getBand(int bid)
{
	if (bid > BANDS -1)
		return 0.0f;
	else
		return m_fBands[bid];
}

//-----------------------------------------------------------------------------
void LLMicMonitor::changeMode()
{
	m_iDrawMode = (m_iDrawMode + 1) % 3;

	
	if (m_iDrawMode == 0)
	{
		memset(m_pImgBuf,0,SPECWIDTH*SPECHEIGHT*4);
		m_pWaveImage->loadDynamicImage((unsigned char*) m_pImgBuf, 
						SPECWIDTH, SPECWIDTH, Ogre::PF_R8G8B8A8);

		Box bx( 0, 0, 0, m_pWaveImage->getWidth(), m_pWaveImage->getHeight(), 1);
		m_pWaveTex->getBuffer()->blitFromMemory( m_pWaveImage->getPixelBox(), bx );
	}
}

//-----------------------------------------------------------------------------
void LLMicMonitor::updateSpectrum()
{
#ifdef _WINDOWS
	// this updateSpectrum code borrowed from BASS lib example (livespec app)
	int x,y,y1,i,idx,numpix;
	unsigned char r,g;

	int boundmax = (SPECWIDTH*SPECHEIGHT*4-4);

	if (m_iDrawMode==3) { // waveform
		short buf[SPECWIDTH];
		memset(m_pImgBuf,0,SPECWIDTH*SPECHEIGHT*4);
		BASS_ChannelGetData(m_dChannel,buf,SPECWIDTH*sizeof(short)); // get the sample data
		for (x=0;x<SPECWIDTH;x++) {
			int v=(32767-buf[x])*SPECHEIGHT/65536; // invert and scale to fit display
			if (!x) y=v;
			do { // draw line from previous sample...
				if (y<v) y++;
				else if (y>v) y--;
				
				int value = abs(y-SPECHEIGHT/2)*2+1;
				idx = (y*SPECWIDTH+x)*4;

				if (idx > boundmax || idx < 0)
							break;

				m_pImgBuf[idx] = 255;
				m_pImgBuf[idx +2] = 255;
				m_pImgBuf[idx +3] = 255;
			
			} while (y!=v);
		}
	} 
	else {
		float fft[1024];
		BASS_ChannelGetData(m_dChannel,fft,BASS_DATA_FFT2048); // get the FFT data

		//int boundmax = (SPECWIDTH*SPECHEIGHT*4-4);

		if (m_iDrawMode==2) { // "normal" FFT
			memset(m_pImgBuf,0,SPECWIDTH*SPECHEIGHT*4);
			for (x=0;x<SPECWIDTH/2;x++) {
#if 1
				y=sqrt(fft[x+1])*3*SPECHEIGHT-4; // scale it (sqrt to make low values more visible)
#else
				y=fft[x+1]*10*SPECHEIGHT; // scale it (linearly)
#endif
				if (y>SPECHEIGHT) y=SPECHEIGHT; // cap it
				if (x && (y1=(y+y1)/2)) // interpolate from previous to make the display smoother
					while (--y1>=0)
					{
						idx = ((SPECHEIGHT-y1)*SPECWIDTH+x*2-1)*4;
						
						if (idx > boundmax || idx < 0)
							break;

						m_pImgBuf[idx] = 255;
						m_pImgBuf[idx +2] = SPECHEIGHT - y1;
						m_pImgBuf[idx +3] = y1;
					}
				y1=y;
				
				while (--y>=0) 
				{
					idx = ((SPECHEIGHT-y)*SPECWIDTH+x*2)*4;
					
					if (idx > boundmax || idx < 0)
						break;

					m_pImgBuf[idx] = 255;
					m_pImgBuf[idx +2] = SPECHEIGHT - y;
					m_pImgBuf[idx +3] = y;
				}
			}
		} 
		else if (m_iDrawMode==1) { // logarithmic, acumulate & average bins
			int b0=0;
			memset(m_pImgBuf,0,SPECWIDTH*SPECHEIGHT*4);
			
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

				// draw bar
				// calc color based on y value
				while (--y >= 0)
				{
					r = y;
					g = SPECHEIGHT - y;
					//idx = ((SPECHEIGHT-y)*SPECWIDTH + x*(SPECWIDTH/BANDS))*4;
					idx = ((SPECHEIGHT-y)*SPECWIDTH + x*steppix)*4;
					
					for (i=0; i<numpix; i++)
					{
						if (idx > boundmax || idx < 0)
							break;

						m_pImgBuf[idx] = 255;
						m_pImgBuf[idx +2] = g;
						m_pImgBuf[idx +3] = r;
						idx = idx + 4;
					}
					
				}
			}
		}
	}
	
	m_pWaveImage->loadDynamicImage((unsigned char*) m_pImgBuf, 
					SPECWIDTH, SPECHEIGHT, Ogre::PF_R8G8B8A8);

	Box bx( 0, 0, 0, m_pWaveImage->getWidth(), m_pWaveImage->getHeight(), 1);
	m_pWaveTex->getBuffer()->blitFromMemory( m_pWaveImage->getPixelBox(), bx );
#endif
}


//-----------------------------------------------------------------------------
void LLMicMonitor::muteMic()
{
#ifdef _WINDOWS
	endpointVolume->SetMasterVolumeLevelScalar(0.0f, NULL);
	LLScreenLog::getSingleton().addText("************************* Mic monitor muted");
#endif
}

//-----------------------------------------------------------------------------
void LLMicMonitor::unmuteMic()
{
#ifdef _WINDOWS
	endpointVolume->SetMasterVolumeLevel(m_fMicVolume, NULL);
	LLScreenLog::getSingleton().addText("************************* Mic monitor unmuted");
#endif
}


