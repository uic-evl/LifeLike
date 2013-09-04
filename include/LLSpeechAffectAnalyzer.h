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
* Filename: LLSpeechAffectAnalyzer.h
* -----------------------------------------------------------------------------
* Notes: 
* -----------------------------------------------------------------------------
*/

#ifndef __LLSPEECHAFFECTANALYZER_H_
#define __LLSPEECHAFFECTANALYZER_H_

#include "LLdefine.h"


class LLStanfordParser;
class LLWordNetAffect;


class LLSpeechAffectAnalyzer
{
public:
	LLSpeechAffectAnalyzer();
	~LLSpeechAffectAnalyzer();

	// evaluate speech string : affect extraction from text
	// return: emotionid, intensity, keyword positions & intensities (nth word)
	// what if there are more than one words associated with evaluated emotion?
	// likely we want to show multiple facial expression here and there
	sSpeechAffect	processSpeechInput(std::string & speech, int dialogAct=DACT_NON);

protected:

	int				getEmotinoID(char* emo);

	// realize associated affection event for the give word (identify with its position)
	// used when TTS encounter new word boundary. then request processing
	void			processWord(int wordpos);

	void			loadAdvModDictionary();
	void			evaluateDependency(std::vector<POSToken>& vt, std::multimap<std::string, std::pair<std::string, std::string>>& depmap);
	void			computeAffect(tNode & node);

	LLStanfordParser*		m_pParser;
	LLWordNetAffect*		m_pWNAffect;
	LLMap_sf				m_advModifierMap;	// for advmod dependencies. custom dictionary

	// emotion name to id map
	LLMap_si	m_mEmotionMap;
};

#endif