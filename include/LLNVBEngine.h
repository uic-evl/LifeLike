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
* Filename: LLNVBEngine.h
* -----------------------------------------------------------------------------
* Notes: this class evaluate various input to emotional input values
*        for instance, speech text will be evaluated to elicit emotion
* -----------------------------------------------------------------------------
*/

#ifndef __LLNVBENGINE_H_
#define __LLNVBENGINE_H_

#include "LLdefine.h"

class LLCharacter;
class LLHeadController;
class LLCRFTagger;

// NVB speech event type
#define NVBEVT_SEMOTION		0		// affect expression
#define NVBEVT_SEXPRESSION	1		// expression not related to emotion (i.e. brow down)
#define NVBEVT_SHEADNOD		2		// short (single) or long(multiple) nodding by value
#define NVBEVT_SHEADSHAKE	3		// short & long
#define NVBEVT_SHEADSWEEP	4		// single motion from left to right

struct NVBSpeechEvent
{
	int		type;		// event type: emotion, head nod/shake, etc
	int		wordpos;	// nth word associated with this event
	float	values[5];	// associated values

	NVBSpeechEvent() 
	{
		values[0] = 0.0f;
		values[1] = 0.0f;
		values[2] = 0.0f;
		values[3] = 0.0f;
		values[4] = 0.0f;
	};
};

class LLNVBEngine
{
public:
	LLNVBEngine(LLCharacter* owner, LLHeadController* hcontrol);
	~LLNVBEngine();

	// speech related functions
	void	processSpeechInput0(fSpeechAffect& affect);		// old verison with only rule-based parsing
	void	processSpeechInput(fSpeechAffect& affect);		// new version. CRF + rule-based
	void	notifyWordDetection(int wordpos);
	void	loadCRFTagger(const char* modelfile, int type=0);
	void	loadCRFTagger(const char* modelfile, const char* type);
	void	setUseEmotionalNVB(bool bUse) { m_bUseEmotionalNVB = bUse;}

protected:

	LLCharacter*		m_pOwner;
	LLHeadController*	m_pHeadController;
	bool				m_bUseEmotionalNVB;

	std::list<NVBSpeechEvent*>	m_lSpeechEvents;

	// data-driven classifier
	LLCRFTagger*		m_pCRFTagger;


};

#endif