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
* Filename: LLStanfordParser.h
* -----------------------------------------------------------------------------
* Notes: 
* -----------------------------------------------------------------------------
*/

#ifndef __LLSTANFORDPARSER_H_
#define __LLSTANFORDPARSER_H_

#include "cj.h"
#include "LLdefine.h"

//(ROOT
//	(S
//		(NP (DT This))
//		(VP (VBZ is)
//			(NP (DT an) (JJ easy) (NN sentence)))
//		(. .)))

class LLStanfordParser
{
public:
	LLStanfordParser();
	~LLStanfordParser();

	void	parseSentence(std::string sentence, std::vector<POSToken>& vt, tNode& tree, std::multimap<std::string, std::pair<std::string, std::string>>& depmap);

protected:

	// JVM initialization
	bool	initializeJVM();

	void	loadDictionary();
	void	parsePOS(LLItr_s &it, LLItr_s &ite, std::vector<POSToken>& vt, bool simpler = true);
	std::string	parseTree(LLItr_s &it, LLItr_s &ite, tNode & node);
	void	parseTree2(LLItr_s &it, LLItr_s &ite, std::vector<POSToken>& vt, bool simpler = true);
	void	parseDeps(LLItr_s &it, LLItr_s &ite, std::multimap<std::string, std::pair<std::string, std::string>>& depmap);
	
	void	evaluateDependency(std::vector<POSToken>& vt, std::multimap<std::string, std::pair<std::string, std::string>>& depmap);
	
	void	mapPennPOStoWN(std::vector<POSToken>& vt);
	void	searchWordFunction(std::vector<POSToken>& vt);

	bool		m_bInitialized;

	// java related variables
	cjJVM_t		m_jvm;
	cjClass_t	m_proxyClass;
	cjObject_t	m_proxy;

	// temp counter to set word index during tree parsing
	int		m_wordLoc;

	// wordnet pos map
	LLMap_si	m_penn2wnposMap;	// Penn Treebank POS (36 tags) to WN-Affect POS (4 tags)

	LLMap_si	m_affirmativePhraseMap;

	// merged keyword map : string, type, value
	// this dictionary holds adv-mod, person-pronoun, affirmative-word, and negative-word
	std::map<std::string, std::pair<int, float>>	m_keywordMap;

	//
	LLMap_ss	m_simplePOSMap;		// simpler version more for prediction purpose
};

#endif