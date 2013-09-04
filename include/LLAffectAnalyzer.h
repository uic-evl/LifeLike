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
* Filename: LLAffectAnalyzer.h
* -----------------------------------------------------------------------------
* Notes: this class evaluate various input to emotional input values
*        for instance, speech text will be evaluated to elicit emotion
* -----------------------------------------------------------------------------
*/

#ifndef __LLAFFECTANALYZER_H_
#define __LLAFFECTANALYZER_H_

class LLEmotionProcessor;
class LLCharacter;
class LLSpeechAffectAnalyzer;

//-----------------------------------------------------------------------------
//  LLAffectAnalyzer
//-----------------------------------------------------------------------------
class LLAffectAnalyzer
{
public:
	LLAffectAnalyzer(LLCharacter* owner, LLEmotionProcessor* mp);
	~LLAffectAnalyzer();

	// evaluate speech string : affect extraction from text
	// return: emotionid, intensity, keyword positions & intensities (nth word)
	// what if there are more than one words associated with evaluated emotion?
	// likely we want to show multiple facial expression here and there
	sSpeechAffect	processSpeechInput(std::string & speech, int dialogAct=DACT_NON);

	// how about user's reponse (user's response in speech)
	// how to evaluate this?
	// can it be processed independently?
	// likely we need to know dialog context to properly evaluate this one
	// well, we can try out something simple

protected:

	//
	LLCharacter*			m_pOwner;
	LLEmotionProcessor*		m_pEmotion;
	LLSpeechAffectAnalyzer*	m_pSpeechAnalyzer;
	
	
	LLMap_sf				m_advModifierMap;	// for advmod dependencies. custom dictionary
	

};

#endif