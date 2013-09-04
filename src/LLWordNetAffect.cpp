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
* Filename: LLWordNetAffect.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#include "LLWordNetAffect.h"
#include "wn.h"
#include "tinyxml2.h"

#ifdef _WINDOWS
#include <windows.h>
#include <windowsx.h>
#endif

#include <string>

using namespace std;
using namespace tinyxml2;

#define ASYNSETFILE		  	"%s\\wn-affect-1.1\\a-synsets.xml"
#define AHIERARCHYFILE  	"%s\\wn-affect-1.1\\a-hierarchy.xml"

//-----------------------------------------------------------------------------
LLWordNetAffect::LLWordNetAffect()
{

	// basic emotional word map
	m_emoWordMap["anger"] = 0;
	m_emoWordMap["contempt"] = 1;
	m_emoWordMap["disgust"] = 2;
	m_emoWordMap["negative-fear"] = 3;
	m_emoWordMap["joy"] = 4;
	m_emoWordMap["sadness"] = 5;
	m_emoWordMap["surprise"] = 6;

	// initialize wordnet
	if (!wninit() && !loadWordNetDB())
		m_bWNInitialized = true;
	else
		m_bWNInitialized = false;

}


//-----------------------------------------------------------------------------
LLWordNetAffect::~LLWordNetAffect()
{

}

//-----------------------------------------------------------------------------
int LLWordNetAffect::loadWordNetDB()
{
	// affect related database located in WNHome\wn-affect-1.1
    int i, openerr;
    char searchdir[256], tmpbuf[256];

#ifdef _WINDOWS
    HKEY hkey;
    DWORD dwType, dwSize;
#else
    char *env;
#endif

	openerr = 0;


#ifdef _WINDOWS
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\WordNet\\3.0"),
		     0, KEY_READ, &hkey) == ERROR_SUCCESS) {
		dwSize = sizeof(searchdir);
		RegQueryValueEx(hkey, TEXT("WNHome"),
				NULL, &dwType, (LPBYTE)searchdir, &dwSize);
		RegCloseKey(hkey);
    } else if (RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\WordNet\\3.0"),
		     0, KEY_READ, &hkey) == ERROR_SUCCESS) {
		dwSize = sizeof(searchdir);
		RegQueryValueEx(hkey, TEXT("WNHome"),
				NULL, &dwType, (LPBYTE)searchdir, &dwSize);
		RegCloseKey(hkey);
    } else
		sprintf(searchdir, DEFAULTPATH);
#else
    if ((env = getenv("WNSEARCHDIR")) != NULL)
		strcpy(searchdir, env);
    else if ((env = getenv("WNHOME")) != NULL)
		sprintf(searchdir, "%s%s", env, DICTDIR);
    else
		strcpy(searchdir, DEFAULTPATH);
#endif

	unsigned long id1, id2;
	const char* str;
	string category, isa;

	// xml stuff
	tinyxml2::XMLDocument	xmlDoc;
	XMLElement *element, *element1, *element2;

	// load synset xml file
	sprintf(tmpbuf, ASYNSETFILE, searchdir);
	xmlDoc.LoadFile(tmpbuf);
	element = xmlDoc.RootElement();		// <syn-list>

	if (!element)	// error
		return 1;

	// NOUN
	if ( (element1 = element->FirstChildElement("noun-syn-list")) == NULL)
		return 1;

	element2 = element1->FirstChildElement("noun-syn");
	while (element2)
	{
		// parse id and category
		str = element2->Attribute("id");
		id1 = atol(str+2);
		
		str =  element2->Attribute("categ");
		category = str;

		// add to map
		m_nounMap[id1] = category;

		element2 = element2->NextSiblingElement("noun-syn");
	}

	// VERB
	if ( (element1 = element->FirstChildElement("verb-syn-list")) == NULL)
		return 1;

	element2 = element1->FirstChildElement("verb-syn");
	while (element2)
	{
		// parse id and noun id
		str = element2->Attribute("id");
		id1 = atol(str+2);
		
		str = element2->Attribute("noun-id");
		id2 = atol(str+2);

		// add to map
		m_etcMap[0][id1] = id2;

		element2 = element2->NextSiblingElement("verb-syn");
	}

	// ADJ
	if ( (element1 = element->FirstChildElement("adj-syn-list")) == NULL)
		return 1;

	element2 = element1->FirstChildElement("adj-syn");
	while (element2)
	{
		// parse id and noun id
		str = element2->Attribute("id");
		id1 = atol(str+2);
		
		str = element2->Attribute("noun-id");
		id2 = atol(str+2);

		// add to map
		m_etcMap[1][id1] = id2;

		element2 = element2->NextSiblingElement("adj-syn");
	}

	// ADV
	if ( (element1 = element->FirstChildElement("adv-syn-list")) == NULL)
		return 1;

	element2 = element1->FirstChildElement("adv-syn");
	while (element2)
	{
		// parse id and noun id
		str = element2->Attribute("id");
		id1 = atol(str+2);
		
		str = element2->Attribute("noun-id");
		id2 = atol(str+2);

		// add to map
		m_etcMap[2][id1] = id2;

		element2 = element2->NextSiblingElement("adv-syn");
	}


	// load hierarchy file
	sprintf(tmpbuf, AHIERARCHYFILE, searchdir);
	xmlDoc.LoadFile(tmpbuf);
	element = xmlDoc.RootElement();
	if (!element)	// error
		return 1;

	element1 = element->FirstChildElement("categ");

	// iterate from second entry: the first one is root without "isa" attribute
	while (element1)
	{
		str = element1->Attribute("name");
		category = str;
		
		str = element1->Attribute("isa");
		if (str == NULL)		// root node does not have isa attribute
			isa = "";
		else
			isa = str;

		m_categoryMap[category] = isa;

		element1 = element1->NextSiblingElement("categ");
	}

	// build emoMap for each noun: iterate nounMap
	int count = 0;
	float eVal; int eCategory;
	int nodes = m_nounMap.size();
	std::map<unsigned long, string>::iterator it;
	std::map<unsigned long, string>::iterator ite;
	std::map<string, string>::iterator itSS;
	std::map<string, int>::iterator itSI;
	ite = m_nounMap.end();
	for (it =m_nounMap.begin(); it != ite; it++)
	{
		//
		count = 0;
		eCategory = -1;

		// get the name of category, then iterate it in hierarchy map
		// in fact, this can be done offline
		// what if this word itself is category name?
		if ( (itSI = m_emoWordMap.find(it->second)) != m_emoWordMap.end() )
			eCategory = itSI->second;

		itSS = m_categoryMap.find(it->second);
		while ( itSS != m_categoryMap.end() )
		{
			str = (char*)itSS->second.c_str();
			if (strcmp(str, "root") == 0)
				break;

			// determin category
			if (strcmp(str, "anger") == 0)
				eCategory = 0;
			else if (strcmp(str, "contempt") == 0)
				eCategory = 1;
			else if (strcmp(str, "disgust") == 0)
				eCategory = 2;
			else if (strcmp(str, "negative-fear") == 0)
				eCategory = 3;
			else if (strcmp(str, "joy") == 0)
				eCategory = 4;
			else if (strcmp(str, "sadness") == 0)
				eCategory = 5;
			else if (strcmp(str, "surprise") == 0)
				eCategory = 6;

			itSS = m_categoryMap.find(str);
			count++;
		}

		// intensity for this noun?
		eVal = CLAMP(0.01f + count * 0.007f, 0.0f, 0.1f);		// 0.0 to 0.1
		// emo category?	if eVal is negative, it means no matching emotion. use this conditionin evaluation process
		eVal += eCategory;

		// add to m_emoMap
		m_emoMap[it->second] = eVal;

		//LLScreenLog::getSingleton().addText("Word: " + it->second + " " +  Ogre::StringConverter::toString(count) + " " + Ogre::StringConverter::toString(eVal));
	}


	return 0;

}

//-----------------------------------------------------------------------------
void LLWordNetAffect::findWordAffect(POSToken& token)
{
	//LLScreenLog::getSingleton().addText("findWordAffect:  " + string(searchword) + " " + Ogre::StringConverter::toString(pos));

	// only process if its WN POS is noun, verb, adj, adv (1,2,3,4)
	if (token.wnPOS == 0)
		return;

	// find affect category for a given word
	// also need to determine intensity of found affect (int, float)
	int i, found = 0;
    IndexPtr index;
	char* morphword;
	char word[64];
	strcpy(word, token.str.c_str());

	// just safe guard
	// empty string and a single letter (i.e. period, question, etc)
	if (strlen(word) < 2)
		return;

	// search word id in WN with given POS tag (noun, verb, adj, adv)
	if (token.wnPOS) 
	{
		if ( (index = getindex(word, token.wnPOS)) != NULL)
			found = token.wnPOS;
		else if ( (morphword = morphstr(word, token.wnPOS)) != NULL)
		{
			index = getindex(morphword, token.wnPOS);
			found = token.wnPOS;
		}
	}
	else
	{
		// search word id in WN
		// as WN-Affect is very limited, we have to use morphword instead of original string
		// note there are some words (noun) that does not have morph
		// therefore, do original word first and morph word for each position
		for (i = 1; i <= NUMPARTS; i++)
		{
			if ( (index = getindex(word, i)) != NULL)
			{
				found = i;
				break;
			}
			else if ( (morphword = morphstr(word, i)) != NULL)
			{
				if ( (index = getindex(morphword, i)) != NULL)
				{
					found = i;
					break;
				}
			}
		}
	}

	// word not found
	if (!found || !index)
		return;

	// use morph word to find it in WN-Affect
	// iterate all offset found and search it in WN-Affect
	// we need our own internal structure holding WN-Affect DB
	// limit synset search up to 5 relavant sense
	LLMap_ls::iterator itS;
	LLMap_ll::iterator itL;
	LLMap_sf::iterator itV;
	string affectStr;
	float affectValue = -1.0f;
	int maxsearch = CLAMP(index->off_cnt, 0, 5);
	for (i=0; i<maxsearch; i++)
	{
		index->offset[i];

		if (found == NOUN)
		{
			// find it in WN-Affect noun list
			if ( (itS = m_nounMap.find(index->offset[i])) != m_nounMap.end())
			{
				affectStr = itS->second;

				// find value
				if ( (itV = m_emoMap.find(affectStr)) != m_emoMap.end() )
					affectValue = itV->second;

				break;
			}
		}
		else
		{
			// etc index to noun index
			if ( (itL = m_etcMap[found-2].find(index->offset[i])) != m_etcMap[found-2].end() )
			{
				// find its noun (category) in noun map
				if ( (itS = m_nounMap.find((unsigned long)itL->second)) != m_nounMap.end())
				{
					affectStr = itS->second;

					// find value
					if ( (itV = m_emoMap.find(affectStr)) != m_emoMap.end() )
						affectValue = itV->second;

					break;
				}
			}
		}
	}

	if (affectStr.size() == 0)
		return;

	// now how to translate affectStr to Ekman's emotional category
	// need some kind of mapping from WN-Affect category to emotional category
	if (affectValue > 0.0f)
	{
		// coefficient for comparative form
		float comparative = 1.0f;
		
		if (token.wnPOS == ADJ)
			if (token.POS.compare("JJR") ==0 ) 
				comparative += 0.1f;		// 10% increase
			else if ( token.POS.compare("JJS") ==0 )
				comparative += 0.2f;		// 20% increase
		
		if (token.wnPOS == ADV)
			if (token.POS.compare("RBR") ==0 ) 
				comparative += 0.1f;		// 10% increase
			else if ( token.POS.compare("RBS") ==0 )
				comparative += 0.2f;		// 20% increase
		
		// decode affect value and apply comparative coeffcient
		token.wnPOS = found;		// position of sentence (noun, verb, adj, adv)
		token.emotion = floor(affectValue);
		//token.intensity = (affectValue - token.emotion) * 15.0f * comparative;
		token.intensity = 0.5f * comparative;
		token.func_type = WORDFN_AFFECT;
		//LLScreenLog::getSingleton().addText("crash:  " + Ogre::StringConverter::toString(result.emotion) + " " + Ogre::StringConverter::toString(result.intensity));
	}

	return;

}