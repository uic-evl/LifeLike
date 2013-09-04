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
* Filename: LLCRFTagger.h
* -----------------------------------------------------------------------------
* Notes: 
* -----------------------------------------------------------------------------
*/

#ifndef __LLCRFTAGGER_H_
#define __LLCRFTAGGER_H_

#include <crfsuite.h>
#include "LLdefine.h"

class LLCRFTagger
{
public:
	LLCRFTagger();
	~LLCRFTagger();

	// load model file
	void loadModel(const char* modelfile, int type);

	// tag surface sentence
	void tagSentence(std::vector<POSToken>& tokens);	// final version (pos, act, phrase, lex_keyword)

protected:

	bool m_bInitialized;

	crfsuite_model_t		*m_pModel[2];	// just two model for now (nod/shake)
	crfsuite_tagger_t		*m_pTagger[2];	// tagger
	crfsuite_dictionary_t	*m_pAttrs[2];	// attribute dictionary
	crfsuite_dictionary_t	*m_pLabels[2];	// label dictionary

	std::string				m_actString[5];				// dialog act id to feature string
};

#endif