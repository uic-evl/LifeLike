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
* Filename: LLSpeechRecognizer.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#ifdef _WINDOWS

#include <windows.h>
#include "LLSpeechRecognizer.h"
#include "LLUnicodeUtil.h"
#include "LLCharacter.h"
#include "LLScreenLog.h"

//-----------------------------------------------------------------------------
LLSpeechRecognizer::LLSpeechRecognizer(LLCharacter* owner)
{
	m_pOwner = owner;
	m_bListening = false;
	m_uGrammarIDGen = 0;
	m_pRootGram = NULL;
	m_pCurrentGram = NULL;

	HRESULT hr;

	// initializer com
	hr = CoInitialize(NULL);

	// initializer recognition engine
	//hr = m_cpEngine.CoCreateInstance(CLSID_SpSharedRecognizer);
	//hr = m_cpEngine->CreateRecoContext(&m_cpRecoCtx);
	
	// for private sr engine (for win 7 sr)
	hr = m_cpEngine.CoCreateInstance(CLSID_SpInprocRecognizer);
	hr = m_cpEngine->CreateRecoContext(&m_cpRecoCtx);

	//LONG vl = 0;
	//m_cpEngine->GetPropertyNum(L"HighConfidenceThreshold", &vl);
	//m_cpEngine->SetPropertyNum(L"HighConfidenceThreshold", 50);
	//m_cpEngine->GetPropertyNum(L"HighConfidenceThreshold", &vl);
	
	// set event handler configuration
	ULONGLONG ullEvents = SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_FALSE_RECOGNITION);
	hr = m_cpRecoCtx->SetInterest(ullEvents, ullEvents);
	m_cpRecoCtx->SetNotifyCallbackFunction(&this->sapiSRCallBack, 0, LPARAM(this));

	// we do not start to listen untill get grammar file
	m_cpEngine->SetRecoState(SPRST_INACTIVE_WITH_PURGE);

	// extra stuff for private sr engine
	CComPtr<ISpObjectToken> pAudioToken;
    CComPtr<ISpAudio> pAudio;
	hr = SpGetDefaultTokenFromCategoryId(SPCAT_AUDIOIN, &pAudioToken);
	hr = m_cpEngine->SetInput(pAudioToken, TRUE);
	hr = SpCreateDefaultObjectFromCategoryId(SPCAT_AUDIOIN, &pAudio);
	hr = m_cpEngine->SetInput(pAudio, TRUE);

}

//-----------------------------------------------------------------------------
LLSpeechRecognizer::~LLSpeechRecognizer()
{
	// release com ptr
	stopListening();

	// root Grammar
	if (m_pRootGram)
		m_pRootGram->mcpGrammar.Release();
	
	// other Grammars
	std::map<ULONGLONG, Grammar*>::iterator GramIter;
    for(GramIter = m_GrammarIDMap.begin(); GramIter != m_GrammarIDMap.end(); ++GramIter)
	{
		if( (*GramIter).second->mIsLoaded )
			(*GramIter).second->mcpGrammar.Release();
	}

	// Context
	if (m_cpRecoCtx)
		m_cpRecoCtx.Release();
	
	// Engine
	if (m_cpEngine)
		m_cpEngine.Release();

	// uninitialize com object
	CoUninitialize(); 
}

//-----------------------------------------------------------------------------
// add new Grammar to RecoContext
int LLSpeechRecognizer::addGrammar(char* filename, bool root)
{
	// check current gramma dictionary (map)
	Grammar* pGram = NULL;

	if (!root)
	{
		pGram = getGrammar(filename);
		if( pGram )
		{
			return pGram->mID;
		}
	}
	else
	{
		// this is root grammar: we do not replace root grammar
		// there just return existing root grammar id
		if (m_pRootGram)
			return m_pRootGram->mID;

	}

	// create grammar object
	m_uGrammarIDGen = m_uGrammarIDGen + 1;
	pGram = new Grammar;
	pGram->mFileName = std::string(filename);
	pGram->mID = m_uGrammarIDGen;
	pGram->mIntState = 0;
	pGram->mStringState = "";
	pGram->mIsLoaded = false;

	// add to dictionary (map)
	if (!root)
	{
		m_GrammarNameMap[pGram->mFileName] = pGram;
		m_GrammarIDMap[pGram->mID] = pGram;
	}
	else
		m_pRootGram = pGram;

	// create sapi grammar
	HRESULT hr = S_OK;
	hr = m_cpRecoCtx->CreateGrammar(pGram->mID, &pGram->mcpGrammar);
	if (FAILED(hr))	return -1;

	// load grammar file
	WCHAR *tempWC;
	tempWC = new WCHAR[_mbstrlen(pGram->mFileName.c_str())+1];
	mbstowcs( tempWC, pGram->mFileName.c_str(), _mbstrlen(pGram->mFileName.c_str()) );
	tempWC[_mbstrlen(pGram->mFileName.c_str())] = L'\0';
	hr = pGram->mcpGrammar->LoadCmdFromFile(tempWC, SPLO_STATIC);
	delete [] tempWC;
	if ( FAILED( hr ) )	return -1;

	// Set rules to active, we are now listening for commands
	// only root gramma is active. other will be inactive
	// to enable gramma need to call setGrammarActive function
	if (root)
	{
		hr = pGram->mcpGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE );
		if ( FAILED( hr ) )	return -1;
	}
	else
	{
		hr = pGram->mcpGrammar->SetRuleState(NULL, NULL, SPRS_INACTIVE );
		if ( FAILED( hr ) )	return -1;
	}

	pGram->mIsLoaded = true;

	return pGram->mID;
}

//-----------------------------------------------------------------------------
int LLSpeechRecognizer::addEmptyGrammar(char* grammarname)
{
	Grammar* pGram = NULL;
	pGram = getGrammar(grammarname);
	if (pGram)
		return pGram->mID;

	// create grammar object
	m_uGrammarIDGen = m_uGrammarIDGen + 1;
	pGram = new Grammar;
	pGram->mFileName = std::string(grammarname);
	pGram->mID = m_uGrammarIDGen;
	pGram->mIntState = 0;
	pGram->mStringState = "";
	pGram->mIsLoaded = false;
	pGram->mRuleCounter = 0;

	// add to dictionary (map)
	m_GrammarNameMap[pGram->mFileName] = pGram;
	m_GrammarIDMap[pGram->mID] = pGram;

	// create sapi grammar
	HRESULT hr = S_OK;
	hr = m_cpRecoCtx->CreateGrammar(pGram->mID, &pGram->mcpGrammar);
	if (FAILED(hr))	return -1;
	
	pGram->mIsLoaded = true;

	hr = pGram->mcpGrammar->Commit(0);

	if (!m_pRootGram && pGram->mID == 1)
	{
		m_pRootGram = pGram;
		m_pRootGram->mcpGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
	}
	return pGram->mID;

}

//-----------------------------------------------------------------------------
int LLSpeechRecognizer::addGrammarRule(int gramId, char* rule)
{
	// find grammar
	Grammar* pGram = NULL;
	pGram = getGrammar(gramId);
	if (!pGram)
		return -1;

	// increment rule counter
	pGram->mRuleCounter++;

	// add rule
	HRESULT hr = S_OK;
	SPSTATEHANDLE hRule;
	WCHAR *tempWC;
	tempWC = new WCHAR[_mbstrlen(rule)+1];
	mbstowcs( tempWC, rule, _mbstrlen(rule) );
	tempWC[_mbstrlen(rule)] = L'\0';
	hr = pGram->mcpGrammar->GetRule(tempWC, pGram->mRuleCounter, SPRAF_TopLevel | SPRAF_Active, TRUE, &hRule);
	delete [] tempWC;
	return pGram->mRuleCounter;

}

//-----------------------------------------------------------------------------
int LLSpeechRecognizer::addGrammarRule(int gramId, int ruleId, char* rule)
{
	// find grammar
	Grammar* pGram = NULL;
	pGram = getGrammar(gramId);
	if (!pGram)
		return -1;

	// increment rule counter
	pGram->mRuleCounter++;

	// add rule
	HRESULT hr = S_OK;
	SPSTATEHANDLE hRule;
	WCHAR *tempWC;
	tempWC = new WCHAR[_mbstrlen(rule)+1];
	mbstowcs( tempWC, rule, _mbstrlen(rule) );
	tempWC[_mbstrlen(rule)] = L'\0';
	hr = pGram->mcpGrammar->GetRule(tempWC, ruleId, SPRAF_TopLevel | SPRAF_Active, TRUE, &hRule);
	delete [] tempWC;
	return gramId;
}

//-----------------------------------------------------------------------------
int LLSpeechRecognizer::addGrammarRule(int gramId, char* rule, char* phrases)
{
	// add rule
	int rid = addGrammarRule(gramId, rule);
	
	// add multiple phrases (each phrases delim :)
	char* p_token;
	char seps[] = ":";
	char str[256];
	strcpy(str, phrases);
	p_token = strtok(str, seps);
	
	while (p_token != NULL)
	{
		// add rule phrase
		addGrammarRulePhrase(gramId, rid, rule, p_token);
		p_token = strtok(NULL, seps);
	}
	
	return rid;
}

//-----------------------------------------------------------------------------
int LLSpeechRecognizer::addGrammarRule(int gramId, int ruleId, char* rule, char* phrases)
{
	// add rule
	int rid = addGrammarRule(gramId, ruleId, rule);
	
	// add multiple phrases (each phrases delim :)
	char* p_token;
	char seps[] = ":";
	char str[256];
	strcpy(str, phrases);
	p_token = strtok(str, seps);
	
	while (p_token != NULL)
	{
		// add rule phrase
		addGrammarRulePhrase(gramId, ruleId, rule, p_token);
		p_token = strtok(NULL, seps);
	}
	
	return rid;

}

//-----------------------------------------------------------------------------
void LLSpeechRecognizer::addGrammarRulePhrase(int gramId, char* rule, char* phrase)
{
	// find grammar
	Grammar* pGram = NULL;
	pGram = getGrammar(gramId);
	if (!pGram)
		return;

	// find rule
	HRESULT hr = S_OK;
	SPSTATEHANDLE hRule;
	WCHAR *tempWC;
	tempWC = new WCHAR[_mbstrlen(rule)+1];
	mbstowcs( tempWC, rule, _mbstrlen(rule) );
	tempWC[_mbstrlen(rule)] = L'\0';
	hr = pGram->mcpGrammar->GetRule(tempWC, 0, SPRAF_TopLevel | SPRAF_Active, FALSE, &hRule);
	delete tempWC;
	if (FAILED(hr))
		return;

	// add word transition
	tempWC = new WCHAR[_mbstrlen(phrase)+1];
	mbstowcs( tempWC, phrase, _mbstrlen(phrase) );
	tempWC[_mbstrlen(phrase)] = L'\0';
	hr = pGram->mcpGrammar->AddWordTransition(hRule, NULL, tempWC, L" ", SPWT_LEXICAL, 1, NULL);
	hr = pGram->mcpGrammar->Commit(0);

	delete tempWC;

}

//-----------------------------------------------------------------------------
void LLSpeechRecognizer::addGrammarRulePhrase(int gramId, int ruleId, char* rule, char* phrase)
{
	// find grammar
	Grammar* pGram = NULL;
	pGram = getGrammar(gramId);
	if (!pGram)
		return;

	// find rule
	HRESULT hr = S_OK;
	SPSTATEHANDLE hRule;
	WCHAR *tempWC;
	tempWC = new WCHAR[_mbstrlen(rule)+1];
	mbstowcs( tempWC, rule, _mbstrlen(rule) );
	tempWC[_mbstrlen(rule)] = L'\0';
	hr = pGram->mcpGrammar->GetRule(tempWC, ruleId, SPRAF_TopLevel | SPRAF_Active, FALSE, &hRule);
	delete tempWC;
	if (FAILED(hr))
		return;

	// add word transition
	tempWC = new WCHAR[_mbstrlen(phrase)+1];
	mbstowcs( tempWC, phrase, _mbstrlen(phrase) );
	tempWC[_mbstrlen(phrase)] = L'\0';
	hr = pGram->mcpGrammar->AddWordTransition(hRule, NULL, tempWC, L" ", SPWT_LEXICAL, 1, NULL);
	hr = pGram->mcpGrammar->Commit(0);

	delete tempWC;

}

//-----------------------------------------------------------------------------
void LLSpeechRecognizer::setGrammarActive(unsigned int id, bool active)
{
	// range check
	if (id < 1)
		return;
	
	std::map<ULONGLONG, Grammar*>::iterator GramIter;
	GramIter = m_GrammarIDMap.find(id);
	if (GramIter == m_GrammarIDMap.end())
		return;

	if (active)
	{
		// disable current grammar first
		if (m_pCurrentGram) {
			m_pCurrentGram->mcpGrammar->SetRuleState(NULL, NULL, SPRS_INACTIVE);
			m_pCurrentGram->mcpGrammar->SetGrammarState(SPGS_DISABLED);
		}

		// set new gramma active and it becomes current grammar
		(*GramIter).second->mcpGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
		(*GramIter).second->mcpGrammar->SetGrammarState(SPGS_ENABLED);
		m_pCurrentGram = (*GramIter).second;
	}
	else
	{
		(*GramIter).second->mcpGrammar->SetRuleState(NULL, NULL, SPRS_INACTIVE);
		(*GramIter).second->mcpGrammar->SetGrammarState(SPGS_DISABLED);
		m_pCurrentGram = NULL;
	}

}

//-----------------------------------------------------------------------------
void LLSpeechRecognizer::setCurrentGrammar(unsigned int id)
{
	if (id == 1)
	{
		if (!m_pRootGram)	
			return;

		m_pRootGram->mcpGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
		m_pRootGram->mcpGrammar->SetGrammarState(SPGS_ENABLED);

		return;
	}

	std::map<ULONGLONG, Grammar*>::iterator GramIter;
	GramIter = m_GrammarIDMap.find(id);
	if (GramIter == m_GrammarIDMap.end())
		return;

	// disable current grammar first
	if (m_pCurrentGram) {
		m_pCurrentGram->mcpGrammar->SetRuleState(NULL, NULL, SPRS_INACTIVE);
		m_pCurrentGram->mcpGrammar->SetGrammarState(SPGS_DISABLED);
	}

	// set new gramma active
	(*GramIter).second->mcpGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
	m_pCurrentGram = (*GramIter).second;
	m_pCurrentGram->mcpGrammar->SetGrammarState(SPGS_ENABLED);

	m_pRootGram->mcpGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
	m_pRootGram->mcpGrammar->SetGrammarState(SPGS_ENABLED);

}

//-----------------------------------------------------------------------------
Grammar* LLSpeechRecognizer::getGrammar(unsigned int id)
{
	if (id < 1)
		return NULL;

	std::map<ULONGLONG, Grammar*>::iterator GramIter;
	GramIter = m_GrammarIDMap.find(id);
	if (GramIter == m_GrammarIDMap.end())
		return NULL;
	else
		return (*GramIter).second;

}

//-----------------------------------------------------------------------------
Grammar* LLSpeechRecognizer::getGrammar(char* filename)
{
	std::string name(filename);

	std::map<std::string, Grammar*>::iterator GramIter;
	GramIter = m_GrammarNameMap.find(name);
	if (GramIter == m_GrammarNameMap.end())
		return NULL;
	else
		return (*GramIter).second;

}

//-----------------------------------------------------------------------------
void LLSpeechRecognizer::startListening()
{
	if (m_bListening)
		return;

	m_cpEngine->SetRecoState(SPRST_ACTIVE);
	m_bListening = true;
}

//-----------------------------------------------------------------------------
void LLSpeechRecognizer::stopListening()
{
	if (!m_bListening)
		return;

	m_cpEngine->SetRecoState(SPRST_INACTIVE_WITH_PURGE);
	m_bListening = false;

}

//-----------------------------------------------------------------------------
bool LLSpeechRecognizer::isListening()
{
	return m_bListening;
	/*
	SPRECOSTATE state;
	m_cpEngine->GetRecoState(&state);
	if (state == SPRST_ACTIVE)
		return true;
	else
		return false;
	*/
}

//-----------------------------------------------------------------------------
void __stdcall LLSpeechRecognizer::sapiSRCallBack(WPARAM wParam, LPARAM lParam)
{
	// SAPI SR event callback function
	LLSpeechRecognizer* pThis = (LLSpeechRecognizer*)lParam;
	pThis->processRecognizerEvent();
}

//-----------------------------------------------------------------------------
void LLSpeechRecognizer::processRecognizerEvent()
{
	CSpEvent event;
	HRESULT hr;
	LPWSTR pwszText;
	LPWSTR pwszRule;

	while ( event.GetFrom(m_cpRecoCtx) == S_OK )
	{
		if (event.eEventId == SPEI_RECOGNITION)
		{
			m_pPhrase = event.RecoResult();
			hr = m_pPhrase->GetPhrase(&m_pParts);
			hr = m_pPhrase->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, FALSE, &pwszText, 0);

			// gramma id
			ULONG gId = m_pParts->ullGrammarID;

			// rule id
			ULONG rId = m_pParts->Rule.ulId;

			// rule name
			char rulename[256];
			wcstombs(rulename, m_pParts->Rule.pszName, wcslen(m_pParts->Rule.pszName)+1);
			
			// confidence
			signed char conf = m_pParts->Rule.Confidence;

			// result
			char result[256];
			wcstombs(result, pwszText, wcslen(pwszText)+1);
			
			LLScreenLog::getSingleton().addText("SR result string: " + Ogre::String(result) +
				" confidence(" + StringConverter::toString(conf) + ")");

			if (conf == 1 || conf == 0)
			{
				// now pass recognized result to owner (character)
				m_pOwner->processListeningEvent(gId, rId, conf, result, rulename);
			}

			// release memory
			CoTaskMemFree(m_pParts);
			CoTaskMemFree(pwszText);
		}
		else if ( event.eEventId == SPEI_FALSE_RECOGNITION )
		{
			// No recognition: do nothing for now
			//m_pOwner->showTipString(10.0f);
		}

	}

}

#endif
