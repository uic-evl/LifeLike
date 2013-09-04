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
* Filename: LLStanfordParser.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#include "LLStanfordParser.h"
#include "tinyxml2.h"

#ifdef LIFELIKEMAINAPP
	#include "LLScreenLog.h"
#else
	#include <string>
	#include <algorithm>
#endif

#include <stack>
#include <assert.h>

#define CHECK_RC(rc) assert((rc) == CJ_ERR_SUCCESS)

// java arguments
#define JCLASSPATH			"-Djava.class.path=../java;../java/stanford-parser.jar;../java/stanford-parser-2012-07-09-models.jar"
#define JCLASSNAME			"cj/example/CSFParser"

using namespace std;
using namespace tinyxml2;

#ifndef LIFELIKEMAINAPP
	char *LLDictionaryFile = "..\\data\\ll-dictionary.xml";
#endif

//-----------------------------------------------------------------------------
LLStanfordParser::LLStanfordParser()
{
	m_bInitialized = initializeJVM();

	loadDictionary();
}

//-----------------------------------------------------------------------------
LLStanfordParser::~LLStanfordParser()
{
	// clean up
	int rc;
	rc = cjFreeObject((m_proxy.clazz)->jvm, m_proxy.object);
	CHECK_RC(rc);

	rc = cjClassDestroy(&m_proxyClass);
	CHECK_RC(rc);

	rc = cjJVMDisconnect(&m_jvm);
	CHECK_RC(rc);

}

//-----------------------------------------------------------------------------
bool LLStanfordParser::initializeJVM()
{
	int rc;
	jobject dummy;
	char *args[] = {JCLASSPATH, "-Xms128m", "-Xmx256m"};

	memset(&m_jvm, 0, sizeof(cjJVM_t));
	memset(&m_proxyClass, 0, sizeof(cjClass_t));
	memset(&m_proxy, 0, sizeof(cjObject_t));

	m_jvm.argc = 3;
	m_jvm.argv = args;
	rc = cjJVMConnect(&m_jvm);
	CHECK_RC(rc);

	rc = cjProxyClassCreate(&m_proxyClass, JCLASSNAME, &m_jvm);
	CHECK_RC(rc);

	m_proxy.clazz = &m_proxyClass;
	rc = cjProxyCreate(&m_proxy);
	CHECK_RC(rc);

	return true;
}

//-----------------------------------------------------------------------------
void LLStanfordParser::loadDictionary()
{
	// lifelike custom dictionary

	// xml stuff
	tinyxml2::XMLDocument	xmlDoc;
	XMLElement *element, *element1;
	const char* str;
	int iVal;
	float fVal;

	// load custom dictionary
	xmlDoc.LoadFile(LLDictionaryFile);
	element = xmlDoc.RootElement();
	if (!element)	// error
		return;

	// read penntree pos to wn pos mapping dictionary
	element1 = element->FirstChildElement("pennpos-to-wn-list")->FirstChildElement("penn2wn");
	while (element1)
	{
		str = element1->Attribute("pos");
		iVal = element1->IntAttribute("wn");
		m_penn2wnposMap[str] = iVal;
		element1 = element1->NextSiblingElement("penn2wn");
	}
	
	// affirmative phrase dictionary
	element1 = element->FirstChildElement("affirmative-phrase-list")->FirstChildElement("aff-phrase");
	while (element1)
	{
		str = element1->Attribute("phrase");
		iVal = element1->IntAttribute("type");
		m_affirmativePhraseMap[str] = iVal;
		element1 = element1->NextSiblingElement("aff-phrase");
	}

	// main keyword dictionary: word(str), type(int), value(float)
	// thie custom keyword includes affirmative, negative, person-pronoun, adv-mod type words
	element1 = element->FirstChildElement("keyword-list")->FirstChildElement("keyword");
	while (element1)
	{
		str = element1->Attribute("word");
		iVal = element1->IntAttribute("type");
		fVal = element1->FloatAttribute("value");
		
		pair<int, float> pvalue(iVal, fVal);
		m_keywordMap[str] = pvalue;
		element1 = element1->NextSiblingElement("keyword");
	}

	// simple POS tag name map
	//m_simplePOSMap["DT"] = "DT";
	m_simplePOSMap["JJ"] = "JJ";
	m_simplePOSMap["JJR"] = "JJ";
	m_simplePOSMap["JJS"] = "JJ";
	m_simplePOSMap["NN"] = "NN";
	m_simplePOSMap["NNS"] = "NN";
	m_simplePOSMap["NNP"] = "NNP";
	m_simplePOSMap["NNPS"] = "NNP";
	m_simplePOSMap["PRP"] = "PRP";
	m_simplePOSMap["PRP$"] = "PRP";
	m_simplePOSMap["RB"] = "RB";
	m_simplePOSMap["RBR"] = "RB";
	m_simplePOSMap["RBS"] = "RB";
	m_simplePOSMap["UH"] = "UH";
	m_simplePOSMap["VB"] = "VB";
	m_simplePOSMap["VBD"] = "VB";
	m_simplePOSMap["VBG"] = "VB";
	m_simplePOSMap["VBN"] = "VB";
	m_simplePOSMap["VBP"] = "VB";
	m_simplePOSMap["VBZ"] = "VB";
	//m_simplePOSMap[","] = ",";
	//m_simplePOSMap["TO"] = "TO";
	m_simplePOSMap["IN"] = "IN";

}

std::string removePeriod(string& str)
{
	if (str.length() < 2)
		return str;

	// in fact, this is strange error: i.e. "hell no." (hell) (no.) not (hell) (no) (.)
	// so just delete all period
	string result = str;
	int begin = result.find(".");
	while (begin != -1)
	{
		result.erase(begin, 1);
		begin = result.find(".");
	}
	return result;
}

//-----------------------------------------------------------------------------
// this function takes relatively long time...
// so, it's noticeable in real-time rendering
void LLStanfordParser::parseSentence(string sentence, vector<POSToken>& vt, tNode& tree, multimap<string, pair<string, string>>& depmap)
{
	if (!m_bInitialized)
		return;

	int rc;
	char sout[8192];
	memset(&sout, 0, 8192);

	// call stanford parser: parse string
	rc = cjProxyExecString(&m_proxy, (char*)sentence.c_str(), sout);
	CHECK_RC(rc);
	
	//LLScreenLog::getSingleton().addText("Sentence parsed:  " + string(sout) );
	
	// what to do with returned string?

	// result string for "This is an easy sentence."
	// line0 [word/tag, ...]
	// line1~ penn tree
	// in fact, penntree includes all POS info
	// so, probably only need to parse tree not both
	/*
	(ROOT
		(S
			(NP (DT This))
			(VP (VBZ is)
				(NP (DT an) (JJ easy) (NN sentence)))
			(. .)))
	*/

	// safeguard
	if (strlen(sout) == 0)
		return;

	// seperate parts (POS, Tree, Dependencies)
	int begin(0), end(0);
	string tStr(sout);
	string::iterator it, ite;

	// POS: [This/DT, is/VBZ, an/DT, easy/JJ, sentence/NN, ./.]
	begin = tStr.find('[', end) + 1;
	end = tStr.find(']', begin);
	string posStr(tStr.substr(begin, end-begin));
	it = posStr.begin(); ite = posStr.end();
	parsePOS(it, ite, vt);
	//LLScreenLog::getSingleton().addText("Sentence POS:  " + posStr );

	// Tree
	begin = tStr.find('[', end) + 1;
	end = tStr.find(']', begin);
	string treeStr(tStr.substr(begin, end-begin));
	std::replace(treeStr.begin(), treeStr.end(), 13, 32);
	std::replace(treeStr.begin(), treeStr.end(), 10, 32);
	it = treeStr.begin(); ite = treeStr.end();
	m_wordLoc = 0;
	//parseTree(it, ite, tree);
	parseTree2(it, ite, vt);
	//LLScreenLog::getSingleton().addText("Sentence Tree:  " + treeStr );
	

	// Dependencies
	begin = tStr.find('[', end) + 1;
	end = tStr.find(']', begin);
	string depStr(tStr.substr(begin, end-begin));
	it = depStr.begin(); ite = depStr.end();
	parseDeps(it, ite, depmap);
	//LLScreenLog::getSingleton().addText("Sentence Deps:  " + depStr );

	// WN-POS mapping
	mapPennPOStoWN(vt);

	// search basic word function: negation, intensify, person-pronoun, affirmative
	searchWordFunction(vt);

	// need to analysis dependency to dig down negation, intensification etc
	evaluateDependency(vt, depmap);
}

//-----------------------------------------------------------------------------
void LLStanfordParser::parsePOS(LLItr_s &it, LLItr_s &ite, std::vector<POSToken>& vt, bool simpler)
{
	// I/PRP, am/VBP, so/RB, happy/JJ, ./.

	int sel = 0;
	string token[2];
	bool comma = false;
	std::map<std::string, std::string>::iterator mapit;

	for ( ; it != ite; it++)
	{
		if (*it == '/')				// word/pos
			sel = (sel + 1) % 2;
		else if (*it == ',')
		{
			if (sel == 1)
			{
				if (comma)
				{
					// comma itself is token string
					comma = false;
					token[sel].push_back(*it);
				}
				else
				{
					// simple form?
					// basically reduce variation for Noun, Adjective, Verb, Adverb
					if (simpler)
					{
						/*if (token[1].find("NN") == 0)
							token[1] = "NN";
						else if (token[1].find("RB") == 0)
							token[1] = "RB";
						else if (token[1].find("VB") == 0)
							token[1] = "VB";
						else if (token[1].find("JJ") == 0)
							token[1] = "JJ";
						*/
						
						// there is strange error in stanford NLP parser
						// "No" tagged as NN always
						// try simple hack
						if (token[0].compare("No")==0 || token[0].compare("no")==0)
						{
							if (token[1].compare("NN")==0 || token[1].compare("NNP")==0)
								token[1] = "UH";
						}

						if ( (mapit = m_simplePOSMap.find(token[1])) != m_simplePOSMap.end())
						{
							token[1] = mapit->second;
						}
						else
							token[1] = "ETC";
					}
					
					// one token done. add to vector
					token[0] = removePeriod(token[0]);
					vt.push_back(POSToken(token[0], token[1]));

					// clear storage and continue
					token[0].clear(); token[1].clear();
					sel = 0;
					it++;		// there is one whitespace between tokens
				}
			}
			else
			{
				token[sel].push_back(*it);
				comma = true;
			}
		}
		else
			token[sel].push_back(*it);
	}
	token[0] = removePeriod(token[0]);
/*	if (simpler)
	{
		if (token[1].find("NN") == 0)
			token[1] = "NN";
		else if (token[1].find("RB") == 0)
			token[1] = "RB";
		else if (token[1].find("VB") == 0)
			token[1] = "VB";
		else if (token[1].find("JJ") == 0)
			token[1] = "JJ";
	}
*/	
	// there is strange error in stanford NLP parser
	// "No" tagged as NN always
	// try simple hack
	if (token[0].compare("No")==0 || token[0].compare("no")==0)
	{
		if (token[1].compare("NN")==0 || token[1].compare("NNP")==0)
			token[1] = "UH";
	}

	if ( (mapit = m_simplePOSMap.find(token[1])) != m_simplePOSMap.end())
	{
		token[1] = mapit->second;
	}
	else
		token[1] = "ETC";

	vt.push_back(POSToken(token[0], token[1]));

	// post hack to fix "Let's" phrase (stanford parser generate this as NNP POS)
	int size = vt.size();
	for (int i=0; i< size; i++)
	{
		if (vt[i].str.compare("Let")==0 || vt[i].str.compare("let")==0)
			if (i+1 < size && vt[i+1].str.compare("'s")==0)
			{
				vt[i].POS = "VB";
				vt[i+1].POS = "PRP";
			}

	}
}

//-----------------------------------------------------------------------------
std::string LLStanfordParser::parseTree(LLItr_s &it, LLItr_s &ite, tNode & node)
{
	// (ROOT    (S      (NP (PRP I))      (VP (VBP am)        (ADJP (RB so) (JJ happy)))      (. .)))

	// some example phrase level tag
	// NP, VP, ADJP, ADVP, PP, CONJP, INTJ
	// may tag word with closest phrase tage (parent tag)

	// node is myself
	// string must start with (LABEL ...
	string token[2];//, label, tag, word;
	string phrase, tstr;
	it++;
	int step = 0;
	bool whitespace = false;
	while (it != ite)
	{
		if (*it == '(')
		{
			// start of new nested node
			// add child node and recursive call
			tNode child;
			tstr = parseTree(it, ite, child);
			phrase.append(" " + tstr);
			node.child.push_back(child);
			continue;
		}
		else if (*it == ')')
		{
			// done with myself!
			if (step == 1)
			{
				token[1] = removePeriod(token[1]);
				node.word.str = token[1];
				node.word.POS = token[0];
				node.word.loc = m_wordLoc;
				m_wordLoc++;

				tstr = token[1];
				phrase = tstr.append(phrase);
			}
			else
				node.label = token[0];

			it++;

			// need to clean up some misc letters from phrase?
			// double whitespace, punctuation, comma, etc
			// possiblly make phrase level analysis easier
			

			node.phrase = phrase;
			return phrase;
		}
		else if (*it == 32)  // whitespace
		{
			// next one should be word
			whitespace = true;
		}
		else if (*it == 13)	// CR (carriage return)
		{
			int a=0;
		}
		else if (*it == '\n')
		{
			int b=0;
		}
		else
		{
			if (whitespace)
			{
				step++;
				whitespace = false;
			}

			token[step].push_back(*it);
		}
		// move to the next character
		it++;
	}

}

//-----------------------------------------------------------------------------
void LLStanfordParser::parseTree2(LLItr_s &it, LLItr_s &ite, std::vector<POSToken>& vt, bool simpler)
{
	// (ROOT    (S      (NP (PRP I))      (VP (VBP am)        (ADJP (RB so) (JJ happy)))      (. .)))

	// some example phrase level tag
	// NP, VP, ADJP, ADVP, PP, CONJP, INTJ
	// tag word with closest phrase tage (parent tag)

	// string must start with (LABEL ...
	string phrase, tstr;
	stack<string>labels;
	bool whitespace = false;
	int wordPos = 0;
	bool lb = false;

	while (it != ite)
	{
		if (*it == '(')
		{
			lb = true;
		}
		else if (*it == ')')
		{
			if (lb)
			{
				// end of word
				// pop one label (this is POS for this word)
				labels.pop();
				// check the top element in stack (this is phrase label)
				vt[wordPos].phrase = labels.top();
				
				lb = false;
				wordPos++;
				tstr = "";
			}
			else
				// pop one label
				labels.pop();

		}
		else if (*it == 32)  // whitespace
		{
			if (!whitespace)
			{
				if (lb)
				{
					// label found (phrase label or pos): push to list
					if (simpler)
					{
						// only use major 5 categories + ETC
						/*if (tstr.compare("NP")!=0)
							if (tstr.compare("VP")!=0)
								if (tstr.compare("INTJ")!=0)
									if (tstr.compare("ADVP")!=0)
										if (tstr.compare("ADJP")!=0)
											tstr = "ETC";
						

						// PP = NP, VP	=> in fact, there is PP (prepositional phrase). so use something else
						// AP = INTJ, ADVP, ADJP
						if (tstr.compare("NP")==0 || tstr.compare("VP")==0)
							tstr = "1P";
						if (tstr.compare("INTJ")==0 || tstr.compare("ADVP")==0 || tstr.compare("ADJP")==0)
							tstr = "2P";
						*/
					}

					labels.push(tstr);
					tstr = "";
				}

				whitespace = true;
			}
		}
		else if (*it == 13)	// CR (carriage return)
		{
			int a=0;
		}
		else if (*it == '\n')
		{
			int b=0;
		}
		else
		{
			// letter found: part of [pharse label, pos, word]
			if (whitespace)
				// here must be word letter
				whitespace = false;

			tstr.push_back(*it);
		}
		
		// move to the next character
		it++;
	}

}

//-----------------------------------------------------------------------------
void LLStanfordParser::parseDeps(LLItr_s &it, LLItr_s &ite, 
								multimap<string, pair<string, string>>& depmap)
{
	// nsubj(happy-4, I-1), cop(happy-4, am-2), advmod(happy-4, so-3), root(ROOT-0, happy-4)
	// tag(word-position_id, word-position_id)

	// later would need to search tag
	// there could be multiple same tags in dependecies (multimap?)

	string token[3];	// dep_tag, word1, word2
	int step = 0;
	bool whitespace = false;
	while (it != ite)
	{
		if (*it == '(')
		{
			// start of new pair
			step++;
		}
		else if (*it == ')')
		{
			// end of one dependencies: store data
			token[1] = removePeriod(token[1]);
			token[2] = removePeriod(token[2]);
			pair<string, string> dep(token[1], token[2]);
			depmap.insert(pair<string, pair<string, string>>(token[0], dep));
			step = 0;
			token[0].clear(); token[1].clear(); token[2].clear();
		}
		else if (*it == ',')
		{
			if (step!=0)
				step++;
		}
		else if (*it == 32)  // whitespace
		{
			// do nothing
		}
		else
		{
			token[step].push_back(*it);
		}
		
		// move to the next character
		it++;
	}
}


//-----------------------------------------------------------------------------
// convert all token's penn tree POS to WN-Affect tag (noun, verb, adj, adv)
void LLStanfordParser::mapPennPOStoWN(std::vector<POSToken>& vt)
{
	int size = vt.size();
	std::map<string, int>::iterator it;
	std::string str;

	for (int i=0; i<size; i++)
	{
		// what's this word's POS?
		if ( (it = m_penn2wnposMap.find(vt[i].POS)) != m_penn2wnposMap.end())
			vt[i].wnPOS = it->second;

		str = vt[i].str;
		std::transform(str.begin(), str.end(),str.begin(), ::tolower);
	}
}

//-----------------------------------------------------------------------------
void LLStanfordParser::searchWordFunction(std::vector<POSToken>& vt)
{
	// very simple word level function detection
	int size = vt.size();
	std::map<std::string, std::pair<int, float>>::iterator it;
	std::string str, sentence;

	// for each word
	for (int i=0; i<size; i++)
	{
		str = vt[i].str;
		std::transform(str.begin(), str.end(),str.begin(), ::tolower);

		if (str.compare(".")!=0 && str.compare(",")!=0)
			sentence.append(str + " ");

		if ( (it = m_keywordMap.find(str)) != m_keywordMap.end())
		{
			// what kind of function we got?
			switch (it->second.first)
			{
			case WORDFN_PERSON:
				// person pronoun
				vt[i].person = (int)it->second.second;
				break;
			case WORDFN_NEGATION:
				vt[i].func_type = WORDFN_NEGATION;
				vt[i].intensity = it->second.second;
				break;
			case WORDFN_AFFIRMATIVE:
				// affirmative word
				vt[i].func_type = WORDFN_AFFIRMATIVE;
				vt[i].intensity = it->second.second / 3.0f;		// value range [1~3] / 3.0f = 0.333f ~ 1.0f
				
				break;
			case WORDFN_INTENSE:
				// adv-mod words
				// do nothing for now
				// this will be done by speech analyzer with dependency graph
				break;
			case WORDFN_INCLUSIVE:
				// inclusivve words
				vt[i].func_type = WORDFN_INCLUSIVE;
				vt[i].intensity = it->second.second;	// in fact, intensity is all same in inclusive word dictionary (1.0f)

				break;
			default:
				break;

			}
		}
	}

	// simple phrase level detection for affirmative phrase
	LLMap_si::iterator itSI;
	for (itSI=m_affirmativePhraseMap.begin(); itSI!=m_affirmativePhraseMap.end(); itSI++)
	{
		if (sentence.find(itSI->first) == 0)
		{
			vt[0].func_type = WORDFN_AFFIRMATIVE;
			vt[0].intensity = (float)itSI->second / 3.0f;
			break;
		}
	}
}

///////////////////////////////////////////////////
// grabbed from speechanalyzer in LifeLike
// find negation, intensifier
//-----------------------------------------------------------------------------
void LLStanfordParser::evaluateDependency(std::vector<POSToken>& vt, std::multimap<string, pair<string, string>>& depmap)
{
	// this will modify raw affect value in token vector
	// a few useful dependencies: most simple and useful dependencies
	// neg(a, b): negation		(use opposite of a: happy -> sad, others zero out)
	// amod						(adjectival modifier: modify noun)
	// advmod					(adverbial modifier: modify adj,verb)
	// prepc_without(a, b)		(negate meaning of b)
	// nsubj
	// conj_and conj_or			a and b, a or b
	std::multimap<string, pair<string, string>>::iterator it;
	std::multimap<string, pair<string, string>>::iterator it_sub;
	pair<std::multimap<string, pair<string, string>>::iterator,std::multimap<string, pair<string, string>>::iterator> ret;
	pair<std::multimap<string, pair<string, string>>::iterator,std::multimap<string, pair<string, string>>::iterator> ret_sub;
	string aword, bword;
	int rankA, rankB;
	size_t begin;

	std::map<std::string, std::pair<int, float>>::iterator kit;

	// AMOD: i.e. amod(guitar-5, favorite-4)
	// should increase/decrease dependent word affect
	// "favorite guitar" case. adjective does not really chagne guitar as it does not have affect
	// so, if aword has affect, bword may modify it base on bword's function/meaning
	// or, if aword has no affect, bword may still keep its own affect

	// Negation: i.e. neg(happy-4, not-2)
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
		if ( (kit = m_keywordMap.find(bword)) != m_keywordMap.end())
		{
			if (kit->second.first == WORDFN_INTENSE)
			{
				// does aword has affect?
				aword = (*it).second.first;
				begin = aword.find("-");
				rankA = atoi(aword.substr(begin+1, aword.length()-begin-1).c_str()) - 1;

				// put function type intense on bword if its value > 1.0f
				// constraint only if bword is before aword
				//if (rankB > -1 && rankB < vt.size())
				if (rankB > -1 && rankB < rankA)
				{
					vt[rankB].func_type = WORDFN_INTENSE;
					vt[rankB].intensity = kit->second.second;		// 1.xxx can be used to adjust nod intensity
				}
			}
		}
	}

	// listing?
	// conj
	ret = depmap.equal_range(string("conj_and"));
	for (it=ret.first; it!=ret.second; ++it)
	{
		aword = (*it).second.first;
		begin = aword.find("-");
		rankA = atoi(aword.substr(begin+1, aword.length()-begin-1).c_str()) - 1;

		bword = (*it).second.second;
		begin = bword.find("-");
		rankB = atoi(bword.substr(begin+1, bword.length()-begin-1).c_str()) - 1;

		// too far away each other?
		//if ((rankB - rankA) > 2)
			//continue;

		if (rankA > -1 && rankA < vt.size())
		{
			if (!vt[rankA].func_type)
				vt[rankA].func_type = WORDFN_LISTING;
		}
		if (rankB > -1 && rankB < vt.size())
		{
			if (!vt[rankB].func_type)
				vt[rankB].func_type = WORDFN_LISTING;
		}

	}

}