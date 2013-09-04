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
* Filename: LLActivityManager.h
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#ifndef __LLACTIVITYMANAGER_H__
#define __LLACTIVITYMANAGER_H__

#ifdef _WINDOWS

#include <map>

using namespace std;

//class LLActivity;
class LLCharacter;
class LLSpeechRecognizer;
class LLActivityBase;

// activity map
typedef std::map <int, LLActivityBase*> iActivityMap;
typedef std::map <int, LLActivityBase*>::iterator iActivityMapIter;
typedef std::map <int, LLActivityBase*>::value_type iActivityMapVt;

// activity manager: collection of Activities
class LLActivityManager
{
public:
	LLActivityManager(LLCharacter* owner, const char* scriptdir = NULL);
	~LLActivityManager();
	
	void update(float addedTime);
	bool init();
	void reset();
	void broadcastMsg(char* msg);
	void sendMsg(char* msg);

	void processRecognizedEvent(int gramId, int ruleId, char conf, char* listened, char* rulename);
	void addActivityTransition(unsigned int ActivityId, std::string str);

	LLActivityBase*	getRootActivity() { return m_pRootActivity;}
	const std::string & getScriptDir() { return m_sScriptDir;}

private:

	// three level of transition
	// a. global system command level (command id 0 to 19 reserved for this)
	// b. transition to other activity (only when we are at the root level)
	// c. transition to other activitystate (within activity. when we already in an activity)
	iActivityMap	m_ActivityMap;

	LLCharacter*	m_pOwner;
	LLActivityBase*	m_pRootActivity;
	LLActivityBase*	m_pCurrentActivity;
	LLActivityBase*	m_pSuspendActivity;
	
	unsigned int	m_uActivityIDGen;
	bool			m_bInitialized;
	std::string		m_sScriptDir;
	bool			m_bMute;

};

/*

procedure

find all py script files
process each script

add activity	: int addActivity()
	register transition strings to reach this activity	: registerActivityTransitionRule(int actId, char* string)
	(we use this strings as rule in root grammar to switch to this activity)
	i.e. "what's in the news"

add activity state
	create state	: int addActivityState()
		create a grammar
		add rules (each rule has multiple strings to be recognized)	: int addRule(int gramid, char* rulename)
		add transition (evnt "1" to state "2")	: addRulePhrase(int gramid, int ruleid, char* phrase)
		register process function (when reached this state, what to do including speaking?)	: registerProcessFunc()


*/

#endif

#endif
