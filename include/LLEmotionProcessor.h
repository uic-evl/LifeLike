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
* Filename: LLEmotionProcessor.h
* -----------------------------------------------------------------------------
* Notes: PAD space model, FACS AU info, FACS DB   
* -----------------------------------------------------------------------------
*/

#ifndef __LLEMOTIONPROCESSOR_H_
#define __LLEMOTIONPROCESSOR_H_


#include "LLdefine.h"

#include <Ogre.h>
using namespace Ogre;

//-----------------------------------------------------------------------------
//  Emotion (internal emotional state based on PAD space model)
//-----------------------------------------------------------------------------
class LLEmotionProcessor
{
public:
	LLEmotionProcessor();
	~LLEmotionProcessor();

	// calculate stimulus to PAD vector
	void	processStimulus(sPAD* stimul);
	void	processStimulus(int& emotion, float& intensity, float& lifetime);

	// update PAD
	void	update(float elapsedTime);
	int		evaluateMood(float elapsedTime);

	// setter
	void	setPersonality(float p, float a, float d);
	void	setPersonality(float o, float c, float e, float a, float n);
	void	resetMood();
	void	setActive(bool bActive) { m_bActive = bActive; }

	// getters
	void	getPAD(sPAD & pad);
	void	getMood(sPAD & mood);
	void	getPersonality(sPAD & personality);
	int		getEmotions(float val[]);
	int		getEmotinoID(char* emo);
	float	getMoodPleasure() { return m_sMood.p;}
	float	getMoodArousal() { return m_sMood.a;}
	float	getMoodDominance() { return m_sMood.d;}

	// debug info file
	void	startLogFile();
	void	recordLog(int emotion=-1, float intensity=0.0f, float lifetime=0.0f);

	void	setUseEmotionalExpression(bool bUse) { m_bUseEmotionalExpression = bUse;}

protected:

	// Personality
	sPAD		m_sPersonality;

	// Mood
	sPAD		m_sTargetMood;
	sPAD		m_sMood;

	// PAD vector value: accumulated active emotions
	sPAD		m_sPAD;

	// coefficient for mood calculation
	float		m_fPrimaryFactor;
	float		m_fSecondaryFactor;

	
	// center of emotion: P A D values
	sPADCategory m_sEmotion[10];

	// list of active emotional states
	iFPADList	m_lPADList;

	// accumulated current emotion & its intensity
	// should make it possible to have blended emotion?
	// i.e. Angry 0.5, Sad 0.5
	// according to PAD space model, emotion can be smooth transition instead of discreet event
	int			m_iEmotion;				// -1 for neutral
	float		m_fEmotions[8];

	// some variables for emtotion category
	// i.e. center position in PAD space and range (radius?)

	// emotion name to id map
	LLMap_si	m_mEmotionMap;

	// debugging
	FILE*		m_logFile;
	float		m_fLogTimer;
	char		m_cDebugString[128];
	HardwarePixelBufferSharedPtr m_pTexBuf;

	// for control case of facial expression
	bool		m_bUseEmotionalExpression;
	bool		m_bActive;
};

#endif