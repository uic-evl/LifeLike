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
* Filename: LLSpeechRecognizer.h
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#ifndef __LLSPEECHRECOGNIZER_H__
#define __LLSPEECHRECOGNIZER_H__

#ifdef _WINDOWS

#include <sphelper.h>
#include <string>
#include <map>

class LLCharacter;

struct Grammar
{
  ULONGLONG mID;
  std::string mFileName;
  int mIntState;
  std::string mStringState;
  CComPtr<ISpRecoGrammar> mcpGrammar;
  bool mIsLoaded;
  int mRuleCounter;
};

class LLSpeechRecognizer
{

public:

	LLSpeechRecognizer(LLCharacter* owner);
	~LLSpeechRecognizer();

	// SR grammer related
	int addGrammar(char* filename, bool root=false);
	int addEmptyGrammar(char* grammarname);
	int addGrammarRule(int gramId, char* rule);
	int addGrammarRule(int gramId, int ruleId, char* rule);
	int addGrammarRule(int gramId, char* rule, char* phrases);
	int addGrammarRule(int gramId, int ruleId, char* rule, char* phrases);
	void addGrammarRulePhrase(int gramId, char* rule, char* phrase);
	void addGrammarRulePhrase(int gramId, int ruleId, char* rule, char* phrase);

	// Listening Status
	void startListening();
	void stopListening();
	bool isListening();

	// Grammar Interface
	void setGrammarActive(unsigned int id, bool active = true);
	void setCurrentGrammar(unsigned int id);
	Grammar* getGrammar(unsigned int id);
	Grammar* getGrammar(char* filename);
	Grammar* getCurrentGrammar() { return m_pCurrentGram;}

	// SAPI SR callback
	static void __stdcall	sapiSRCallBack(WPARAM wParam, LPARAM lParam);
	void					processRecognizerEvent();

private:

	// SAPI obj pointer
	CComPtr<ISpRecognizer>			m_cpEngine;		// SR Engine
	CComPtr<ISpRecoContext>			m_cpRecoCtx;	// SR Context
	CComPtr<ISpGrammarBuilder>		m_cpGrammarBuilder;
	
	Grammar*						m_pRootGram;	// root level Grammar
	SPPHRASE*						m_pParts;
	ISpPhrase*						m_pPhrase;

	bool							m_bListening;
	
	// Character : owner of this SR
	LLCharacter*					m_pOwner;

	// SR grammar
	ULONGLONG						m_uGrammarIDGen;
	std::map<std::string, Grammar*> m_GrammarNameMap;
	std::map<ULONGLONG, Grammar*>	m_GrammarIDMap;
	Grammar*						m_pCurrentGram;

};

#endif

#endif
