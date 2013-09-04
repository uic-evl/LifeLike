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
* Filename: LLAudioMonitor.h
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#ifndef __LLAUDIOMONITOR_H__
#define __LLAUDIOMONITOR_H__

#ifdef _WINDOWS

#include <windows.h>
#include <Ogre.h>

using namespace Ogre;

#define BANDS 28

class LLAudioMonitor
{
public:
	LLAudioMonitor(char* devicename);
	~LLAudioMonitor();

	bool		initDevice(char* devicename);
	void		update(float addedTime);
	void		updateSpectrum();
	float		getAudioLevel(int chan = 0);
	float		getAudioLevelSmooth(int chan = 0);
	float		getBand(int bid);

	// recording function
	void		startRecording();
	void		stopRecording();
	void		enableRecording() { m_bEnableRecording = true;}

private:
	
	int			m_iDeviceId;
	DWORD		m_dChannel;
	float		m_fCurrLevelLeft;
	float		m_fCurrLevelRight;
	float		m_fBands[BANDS];
	bool		m_bInitialized;
	bool		m_bEnableRecording;
	char		m_sRecordFileName[MAX_PATH];

	float		m_fSmoothLevelLeft;
	float		m_fSmoothLevelRight;

};

#endif

#endif
