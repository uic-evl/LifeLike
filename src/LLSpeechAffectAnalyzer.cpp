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
* Filename: LLSpeechAffectAnalyzer.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/


#include "LLSpeechAffectAnalyzer.h"
#include "LLWordNetAffect.h"
#include "LLStanfordParser.h"

#include "tinyxml2.h"
#include "LLScreenLog.h"

using namespace std;
using namespace tinyxml2;

LLSpeechAffectAnalyzer::LLSpeechAffectAnalyzer()
{
	// load adv-mod word dictionary
	loadAdvModDictionary();

	// WordNet-Affect DB
	m_pWNAffect = new LLWordNetAffect();

	// stanford parser test
	m_pParser = new LLStanfordParser();

	// emotion name to id mapping utility
	m_mEmotionMap["NONE"] = -1;
	m_mEmotionMap["ANGRY"] = 0;
	m_mEmotionMap["CONTEMPT"] = 1;
	m_mEmotionMap["DISGUST"] = 2;
	m_mEmotionMap["FEAR"] = 3;
	m_mEmotionMap["HAPPY"] = 4;
	m_mEmotionMap["SADNESS"] = 5;
	m_mEmotionMap["SAD"] = 5;
	m_mEmotionMap["SURPRISE"] = 6;
	m_mEmotionMap["PAIN"] = 8;
}

LLSpeechAffectAnalyzer::~LLSpeechAffectAnalyzer()
{
	if (m_pWNAffect)
		delete m_pWNAffect;
	if (m_pParser)
		delete m_pParser;

}

//-----------------------------------------------------------------------------
void LLSpeechAffectAnalyzer::loadAdvModDictionary()
{
	// affect related database located in WNHome\wn-affect-1.1
	const char* str;
	float eVal;

	// xml stuff
	tinyxml2::XMLDocument	xmlDoc;
	XMLElement *element, *element1, *element2;

	// load adjective modifier dictionary
	xmlDoc.LoadFile(LLDictionaryFile);
	element = xmlDoc.RootElement();
	if (!element)	// error
		return;

	element1 = element->FirstChildElement("adv-mod-list")->FirstChildElement("adv-mod");

	// iterate from second entry: the first one is root without "isa" attribute
	while (element1)
	{
		str = element1->Attribute("word");
		eVal = element1->FloatAttribute("mod");
		m_advModifierMap[str] = eVal;
		element1 = element1->NextSiblingElement("adv-mod");
	}
}

//-----------------------------------------------------------------------------
int LLSpeechAffectAnalyzer::getEmotinoID(char* emo)
{
	std::string str = emo;
	std::transform(str.begin(), str.end(),str.begin(), ::toupper);

	std::map<std::string,int>::iterator it;
	it = m_mEmotionMap.find(str);
	if (it != m_mEmotionMap.end())
	{
		return it->second;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// two cases
//		with manually annotated emotional tag
//		without manually annotated emotional tag
sSpeechAffect LLSpeechAffectAnalyzer::processSpeechInput(string& speech, int dialogAct)
{
	sSpeechAffect result;

	// copy string
	string tempStr = speech;
	
	// check if there is manually annotated emotion tag exists
	// tag must be removed to be processed later on
	// somehow need to store its position information: some sort of loopup table?
	// <emo a/>I am <emo b/>happy.  => pos(a)=0, pos(b)=5
	int s(0),e(0),annoCounter(0);
	bool bManualAnnotation = false;
	std::vector<POSToken> annotationVT;
	s = tempStr.find("<emo");
	if (s != -1)
	{
		bManualAnnotation = true;
		tinyxml2::XMLDocument	xmlDoc;
		char tchars[128];
		
		while (s != -1)
		{
			// parse this emotion tag and process it then... return;
			e = tempStr.find("/>", s);

			// any error check?
			if (e == -1)
			{
				if (annoCounter==0)
					bManualAnnotation = false;
				break;
			}
			annoCounter++;

			// temp storage for position restore later on
			POSToken anno;

			// new xml parser
			std::string emoStr = tempStr.substr(s, e-s+2);	// <emo id="HAPPY" value="0.5"/>
			xmlDoc.Parse(emoStr.c_str());
			XMLElement* element;
			element = xmlDoc.FirstChildElement( "emo" );
			char* str = (char*)element->Attribute("id");
			anno.intensity = element->FloatAttribute("value");
			anno.emotion = getEmotinoID(str);
			anno.loc = s;
			annotationVT.push_back(anno);

			tempStr.erase(s, e-s+2);
			s = tempStr.find("<emo");
		}
		// set speech string with tag removed one
		speech = tempStr;
	}
	
	// dialog act
	if (dialogAct == DACT_NON)
	{
		// see if there is embedded dialog act
		// it is beginning of sentence with ()
		// (y) (n) (a) (r)

		s = tempStr.find("(");
		if (s != -1)
		{
			const char dcode = tempStr.at(s+1);
			if (dcode == 'y')
				dialogAct = DACT_YES;
			else if (dcode == 'a')
				dialogAct = DACT_AGR;
			else if (dcode == 'n')
				dialogAct = DACT_NO;
			else if (dcode == 'r')
				dialogAct = DACT_NAGR;

			tempStr.erase(s, s+3);
		}
	}

	// get rid of all sapi related tag if present: find <..> and erase it
	int offset = 0;
	s = tempStr.find("<");
	while (s!=-1)
	{
		// find >
		e = tempStr.find(">");
		
		// exception: cannot find >
		if (e==-1)
			break;
		
		// remove tag
		tempStr.erase(s, e-s+1);
		
		// adjust manual annotation tag position if any presents
		if (bManualAnnotation)
		{
			for (int i=0; i<annoCounter; i++)
				if (annotationVT[i].loc > s)
					annotationVT[i].loc -= (e-s+1);
		}

		// find next <
		s = tempStr.find("<");

	}
	result.sentence = tempStr;

	// parse sentence: stanford nlp parser
	tNode tree;
	std::multimap<string, pair<string, string>> depmap;
	m_pParser->parseSentence(tempStr, result.token, tree, depmap);
	
	// evaluate word affect and store them in vector
	int nWords = result.token.size();
	for (int i=0; i<nWords; i++)
		m_pWNAffect->findWordAffect(result.token[i]);

	// evaluate dependency map
	evaluateDependency(result.token, depmap);

	// if there was a manually annotated emotional tag with origial speech
	// reset all emotional affect value, and use manual ones
	if (bManualAnnotation)
	{
		s=0,e=0;
		for (int i=0; i<nWords; i++)
		{
			// position of this token in sentence
			s = tempStr.find(result.token[i].str, s);
			result.token[i].emotion = -1;

			// check if there is manual annotation that has the same position as this one
			for (;e<annoCounter;e++)
			{
				if (s == annotationVT[e].loc)
				{
					result.token[i].emotion = annotationVT[e].emotion;
					result.token[i].intensity = annotationVT[e].intensity;
					result.token[i].func_type = WORDFN_AFFECT;
					e++;
					break;
				}
				else if (s < annotationVT[e].loc)
					break;
			}

		}
	}

	// compute sentence affect from words
	// some rule need to be applied here: borrow some ideas from AAM framework
	// try very simple rule here. find max one and use it as affect value
	float maxVal = 0.0f;
	int mainword = -1;
	for (int i=0; i<nWords; i++)
	{
		if (result.token[i].emotion != -1 && result.token[i].intensity > maxVal)
		{
			maxVal = result.token[i].intensity;
			result.emotion = result.token[i].emotion;
			result.intensity = maxVal;
			result.lifetime = AFFECTLIFETIME;
			mainword = i;
		}

		if (!bManualAnnotation && result.token[i].func_type == WORDFN_AFFECT)
			result.token[i].func_type = WORDFN_NONE;


		// set dialog act
		result.token[i].act = dialogAct;

	}

	// recover affect main word function type
	if (mainword != -1)
		result.token[mainword].func_type = WORDFN_AFFECT;

	// debug for words affect values
//#ifdef _DEBUG
	// (int, float): word(int, float) word(int, float) ...
	char tstr[128];
	string dout;
	sprintf(tstr, "(%d, %.2f):  ", result.emotion, result.intensity);
	
	dout.append(tstr);
	for (int i=0; i<nWords; i++)
	{
		sprintf(tstr, "%s(%d, %.2f) ", result.token[i].str.c_str(), result.token[i].emotion, result.token[i].intensity);
		if (result.token[i].emotion == -1)
			sprintf(tstr, "%s ", result.token[i].str.c_str());
		else
			sprintf(tstr, "%s(%d) ", result.token[i].str.c_str(), result.token[i].emotion);

		dout.append(tstr);
	}
	LLScreenLog::getSingleton().addText("Sentence Affect:  " + dout );
//#endif

	return result;
}

//-----------------------------------------------------------------------------
// this is recursive function
void LLSpeechAffectAnalyzer::computeAffect(tNode & node)
{
	std::map<string, int>::iterator it;
	int nChild = node.child.size();
	
	// do it for myself if node has word => this is leaf node case
	if (nChild == 0)
		return;

	// non-leaf node: recursive call for child node
	for (int i=0; i<nChild; i++)
		computeAffect(node.child[i]);

	// compute phrase level affect
	// how?
}

//-----------------------------------------------------------------------------
void LLSpeechAffectAnalyzer::evaluateDependency(std::vector<POSToken>& vt, std::multimap<string, pair<string, string>>& depmap)
{
	// this will modify raw affect value in token vector
	// a few useful dependencies: most simple and useful dependencies
	// neg(a, b): negation		(use opposite of a: happy -> sad, others zero out)
	// amod						(adjectival modifier: modify noun)
	// advmod					(adverbial modifier: modify adj,verb)
	// prepc_without(a, b)		(negate meaning of b)
	// nsubj
	// conj_and conj_or			a and b, a or b
	// dep(a, b)				possiblly a negate b (i.e. "not happy")

	std::multimap<string, pair<string, string>>::iterator it;
	std::multimap<string, pair<string, string>>::iterator it_sub;
	pair<std::multimap<string, pair<string, string>>::iterator,std::multimap<string, pair<string, string>>::iterator> ret;
	pair<std::multimap<string, pair<string, string>>::iterator,std::multimap<string, pair<string, string>>::iterator> ret_sub;
	string aword, bword;
	int rankA, rankB;
	size_t begin;

	// AMOD: i.e. amod(guitar-5, favorite-4)
	// should increase/decrease dependent word affect
	// "favorite guitar" case. adjective does not really chagne guitar as it does not have affect
	// so, if aword has affect, bword may modify it base on bword's function/meaning
	// or, if aword has no affect, bword may still keep its own affect

	// Negation: i.e. neg(happy-4, not-2)
	// also this could be useful to show head shake motion in case
	ret = depmap.equal_range(string("neg"));
	for (it=ret.first; it!=ret.second; ++it)
	{
		// set negation bool for bword
		bword = (*it).second.second;
		begin = bword.find("-");
		rankB = atoi(bword.substr(begin+1, bword.length()-begin-1).c_str()) - 1;
		if (rankB > -1 && rankB < vt.size())
		{
			vt[rankB].func_type = WORDFN_NEGATION;
			if (vt[rankB].intensity == 0.0f)
				vt[rankB].intensity = 1.0f;
		}

		// opposite of the first word
		aword = (*it).second.first;
		// find the rank of this first word
		begin = aword.find("-");
		rankA = atoi(aword.substr(begin+1, aword.length()-begin-1).c_str()) - 1;
		if (rankA > -1 && rankA < vt.size())
		{
			if (vt[rankA].emotion == -1)
			{
				// any indirect or phrase level negation?
				// i.e. "I don't think I am happy."
				// nsubj(think-4, I-1), aux(think-4, do-2), neg(think-4, n't-3), root(ROOT-0, think-4), nsubj(happy-7, I-5), cop(happy-7, am-6), ccomp(think-4, happy-7)
				// this really needs in-depth analysis about sentence structure.

				// advcl	"I am not sure if she is happy"		(sure, happy)
				// ccomp	"I don't think that she is happy"	(think, happy)
				// xcomp	"I am not ready to be happy"		(ready, happy)
				ret_sub = depmap.equal_range(string("advcl"));
				for (it_sub=ret_sub.first; it_sub!=ret_sub.second; ++it_sub)
				{
					if (aword.compare((*it_sub).second.first) == 0)
					{
						bword = (*it_sub).second.second;
						begin = bword.find("-");
						rankB = atoi(bword.substr(begin+1, bword.length()-begin-1).c_str()) - 1;
						if (vt[rankB].emotion == -1)
							continue;

						if (vt[rankB].emotion == EMO_HAPPY)
							vt[rankB].emotion = EMO_SADNESS;
						else if (vt[rankB].emotion == EMO_SADNESS)
						{
							vt[rankB].emotion = EMO_HAPPY;
							vt[rankB].intensity *= 0.5;
						}
						else
							vt[rankB].emotion = -1;
					}
				}
				ret_sub = depmap.equal_range(string("ccomp"));
				for (it_sub=ret_sub.first; it_sub!=ret_sub.second; ++it_sub)
				{
					if (aword.compare((*it_sub).second.first) == 0)
					{
						bword = (*it_sub).second.second;
						begin = bword.find("-");
						rankB = atoi(bword.substr(begin+1, bword.length()-begin-1).c_str()) - 1;
						if (vt[rankB].emotion == -1)
							continue;

						if (vt[rankB].emotion == EMO_HAPPY)
							vt[rankB].emotion = EMO_SADNESS;
						else if (vt[rankB].emotion == EMO_SADNESS)
						{
							vt[rankB].emotion = EMO_HAPPY;
							vt[rankB].intensity *= 0.5;
						}
						else
							vt[rankB].emotion = -1;
					}
				}
				ret_sub = depmap.equal_range(string("xcomp"));
				for (it_sub=ret_sub.first; it_sub!=ret_sub.second; ++it_sub)
				{
					if (aword.compare((*it_sub).second.first) == 0)
					{
						bword = (*it_sub).second.second;
						begin = bword.find("-");
						rankB = atoi(bword.substr(begin+1, bword.length()-begin-1).c_str()) - 1;
						if (vt[rankB].emotion == -1)
							continue;

						if (vt[rankB].emotion == EMO_HAPPY)
							vt[rankB].emotion = EMO_SADNESS;
						else if (vt[rankB].emotion == EMO_SADNESS)
						{
							vt[rankB].emotion = EMO_HAPPY;
							vt[rankB].intensity *= 0.5;
						}
						else
							vt[rankB].emotion = -1;
					}
				}

				continue;
			}

			// change happy to sad. zero out for other category
			if (vt[rankA].emotion == EMO_HAPPY)
				vt[rankA].emotion = EMO_SADNESS;
			else if (vt[rankA].emotion == EMO_SADNESS)
			{
				vt[rankA].emotion = EMO_HAPPY;
				vt[rankA].intensity *= 0.5;
			}
			else
				vt[rankA].emotion = -1;
		}
	}
	// there are some possibility that dependency negation does not catch negative word
	// i.e. "no way" "absolutely not" "no" "nothing" ...
	// can do one more iteration for all word and find negation word.
	// assume no interaction with affect word. it's too complicated to take affect into account.
	ret = depmap.equal_range(string("dep"));
	for (it=ret.first; it!=ret.second; ++it)
	{
		// i.e dep(not, happy)
		aword = (*it).second.first;
		begin = aword.find("-");
		rankA = atoi(aword.substr(begin+1, aword.length()-begin-1).c_str()) - 1;
		if (rankA > -1 && rankA < vt.size())
		{
			if (vt[rankA].func_type == WORDFN_NEGATION)
			{
				bword = (*it).second.second;
				begin = bword.find("-");
				rankB = atoi(bword.substr(begin+1, bword.length()-begin-1).c_str()) - 1;
				if (rankB > -1 && rankB < vt.size())
				{
					if (vt[rankB].emotion == EMO_HAPPY)
						vt[rankB].emotion = EMO_SADNESS;
					else if (vt[rankB].emotion == EMO_SADNESS)
					{
						vt[rankB].emotion = EMO_HAPPY;
						vt[rankB].intensity *= 0.5;
					}
					else
						vt[rankB].emotion = -1;
				}
			}
		}
	}

	// ADVMOD : advmod(suitable, less)
	// need to know the meaning of bword to modify aword
	// may use simple dictionary for our own that includes very simple adj words
	ret = depmap.equal_range(string("advmod"));
	LLMap_sf::iterator isf;
	for (it=ret.first; it!=ret.second; ++it)
	{
		bword = (*it).second.second;
		begin = bword.find("-");
		rankB = atoi(bword.substr(begin+1, bword.length()-begin-1).c_str()) - 1;
		bword = bword.substr(0, begin);
		if ( (isf = m_advModifierMap.find(bword)) != m_advModifierMap.end())
		{
			// does aword has affect?
			aword = (*it).second.first;
			begin = aword.find("-");
			rankA = atoi(aword.substr(begin+1, aword.length()-begin-1).c_str()) - 1;
			if (rankA > -1 && rankA < vt.size())
			{
				if (vt[rankA].emotion != -1)
					// apply adverb modfication factor: 
					vt[rankA].intensity *= isf->second;
			}

			// put function type intense on bword if its value > 1.0f
			// constraint only if bword is before aword
			if (isf->second > 1.0f && rankA > rankB)
			{
				if (rankB > -1 && rankB < vt.size())
				{
					if (vt[rankA].func_type == WORDFN_NEGATION)
					{
						vt[rankA].intensity *= isf->second;
						continue;
					}
					
					vt[rankB].func_type = WORDFN_INTENSE;
					vt[rankB].intensity = isf->second;		// 1.xxx can be used to adjust nod intensity
					
				}
			}
		}
	}

	// PREPC_WITHOUT	: i.e. prep_without(smashed-2, regret-7)
	ret = depmap.equal_range(string("prep_without"));
	for (it=ret.first; it!=ret.second; ++it)
	{
		bword = (*it).second.second;
		begin = aword.find("-");
		rankB = atoi(bword.substr(begin+1, bword.length()-begin-1).c_str()) - 1;
		if (rankB > -1 && rankB < vt.size())
		{
			if (vt[rankB].emotion == -1)
				continue;
			else
				// coefficient zero applied
				vt[rankB].emotion = -1;
		}
	}

	// NSUBJ	: sentence level coefficient (refer to AAM)
	// this is acturally more complicated step (see AAM2010 page 111)
	// if affect word's subject is not "I", then reduce it's affect value by half
	// at lease this should be applied when a word is affect word
	ret = depmap.equal_range(string("nsubj"));
	for (it=ret.first; it!=ret.second; ++it)
	{
		bword = (*it).second.second;
		begin = bword.find("-");
		rankB = atoi(bword.substr(begin+1, bword.length()-begin-1).c_str()) - 1;

		aword = (*it).second.first;
		begin = aword.find("-");
		rankA = atoi(aword.substr(begin+1, aword.length()-begin-1).c_str()) - 1;
		
		if (vt[rankA].emotion != -1)
		{
			switch (vt[rankB].person)
			{
			case PERSON_SECOND:
				vt[rankA].intensity *= 0.8f;
				break;
			case PERSON_THIRD:
				vt[rankA].intensity *= 0.4f;
				break;
			default:
				break;


			}

		}
	}
}