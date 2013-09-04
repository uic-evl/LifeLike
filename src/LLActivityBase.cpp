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
* Filename: LLActivityBase.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#include "LLCharacter.h"
#include "LLSpeechRecognizer.h"
#include "LLActivityBase.h"
#include "LLActivityManager.h"
#include "LLScreenLog.h"

#ifdef _WINDOWS
//-----------------------------------------------------------------------------
void LLActivityBase::registerTransition(char *reco)
{
	// ActivityManager needs to know this
	LLScreenLog::getSingleton().addText("LLActivityBase register Transition: " + Ogre::String(reco));

	m_sTransitionStr = std::string(reco);
	
	m_pManager->addActivityTransition(m_uID, m_sTransitionStr);
}

//-----------------------------------------------------------------------------
int LLActivityBase::addGrammar(char* gram)
{
	if (!m_pOwner)
		return 0;

	LLSpeechRecognizer* SR = m_pOwner->getSpeechRecognizer();

	if (SR)
	{
		int gid = SR->addEmptyGrammar(gram);

		LLScreenLog::getSingleton().addText("LLActivityBase added Grammar: " + Ogre::String(gram));

		return gid;
	}
	else
		return 0;

}

//-----------------------------------------------------------------------------
int LLActivityBase::addGrammarRule(int gramId, char* rule, char* phrase)
{
	if (!m_pOwner)
		return 0;

	LLSpeechRecognizer* SR = m_pOwner->getSpeechRecognizer();

	if (SR)
	{
		int resultid = SR->addGrammarRule(gramId, rule, phrase);
		
		LLScreenLog::getSingleton().addText("LLActivityBase added GrammarRule: gramId(" + 
			StringConverter::toString(gramId) + "), rule(" + Ogre::String(rule) + 
			"), phrase(" + Ogre::String(phrase) + ")");
		LLScreenLog::getSingleton().addText("\t returned rule id: " + StringConverter::toString(resultid));

		return resultid;
	}
	else
		return 0;

}

//-----------------------------------------------------------------------------
void LLActivityBase::setCurrentGrammar(int gramId)
{
	if (!m_pOwner)
		return;

	LLSpeechRecognizer* SR = m_pOwner->getSpeechRecognizer();

	if (SR)
	{
		SR->setCurrentGrammar(gramId);

		LLScreenLog::getSingleton().addText("LLActivityBase set current Grammar: " + StringConverter::toString(gramId));

		// testing tip string...
		Grammar* grm = SR->getCurrentGrammar();
		
	}

}

//-----------------------------------------------------------------------------
void LLActivityBase::setGrammarActive(int gramId, bool active)
{
	if (!m_pOwner)
		return;

	LLSpeechRecognizer* SR = m_pOwner->getSpeechRecognizer();

	if (SR)
	{
		SR->setGrammarActive(gramId, active);
		
		LLScreenLog::getSingleton().addText("LLActivityBase set Grammar Active: id(" + StringConverter::toString(gramId)
			+ "), active(" + StringConverter::toString(active) + ")" );
	}

}

//-----------------------------------------------------------------------------
void LLActivityBase::speak(char* speech)
{
	if (!m_pOwner)
		return;

	//m_pOwner->speak(speech, CSP_WAVE);
	m_pOwner->speak(speech);

	//LLScreenLog::getSingleton().addText("LLActivityBase request speak: " + Ogre::String(speech));
}

//-----------------------------------------------------------------------------
// better to use stopListening() interface instead?
void LLActivityBase::muteMic()
{
	if (!m_pOwner)
		return;

	m_pOwner->stopListening();
}

//-----------------------------------------------------------------------------
// better to use startListening() interface instead?
void LLActivityBase::unmuteMic()
{
	if (!m_pOwner)
		return;

	m_pOwner->startListening();
}

//-----------------------------------------------------------------------------
std::string LLActivityBase::getOwnerName()
{

	if (!m_pOwner)
		return "";

	return m_pOwner->getName();
}

//-----------------------------------------------------------------------------
void LLActivityBase::addEvent(char* evtstr)
{
	if (!m_pOwner)
		return;

	// this is integration of behavior() & feedback()
	// anything activity wants to chagne other than speak and listen
	// generate new event and enqueue it to character's event queue
	// then character will handle the event

	// event string consists of ...
	// [EVT][starttime][parameters]
	// EVT: type of event (MOV, ATT, VOC, ACT, GUI, MAT, ...)
	// starttime: delayed event start time (0.0 means prompt start)
	// parameters: string stores various paramters for each event (':' delim)

	char *p_token;
	char seps[] = "[]";
	char seps2[] = ":";
	char str[256], str2[256], val[128];
	strcpy(str, evtstr);
	p_token = strtok(str, seps);

	if (p_token == NULL)
		return;

	// new event
	LLEvent evt;

	// move character: [MOV][starttime][x y z ori_degree]	note that x y z ori_degree should use ':' delim (notify ucf folks)
	if (strcmp(p_token, "MOV") == 0)
	{
		// general version for character general event list
		// do not decode parameters here. character will do it
		evt.type = EVT_MOVE;
		evt.eTime = atof(strtok(NULL, seps));
		
		// decode params
		strcpy(str2, strtok(NULL, seps));
		evt.args[0] = atof(strtok(str2, seps2));	// x
		evt.args[1] = atof(strtok(NULL, seps2));	// y
		evt.args[2] = atof(strtok(NULL, seps2));	// z
		evt.args[3] = atof(strtok(NULL, seps2));	// ori_degree
	}
	// set attraction: [ATT][starttime][x y z expiretime]
	else if (strcmp(p_token, "ATT") ==0)
	{
		// set attraction point for avatar
		// useful to make avatar look the other avatar while listening
		evt.type = EVT_ATTRACTION;
		evt.eTime = atof(strtok(NULL, seps));
		
		// decode params
		strcpy(str2, strtok(NULL, seps));
		evt.args[0] = atof(strtok(str2, seps2));	// x
		evt.args[1] = atof(strtok(NULL, seps2));	// y
		evt.args[2] = atof(strtok(NULL, seps2));	// z
		evt.args[3] = atof(strtok(NULL, seps2));	// expiretime
	}
	// set action: [ACT][starttime][actionid]
	else if (strcmp(p_token, "ACT") == 0)
	{
		evt.type = EVT_ACTION;
		evt.eTime = atof(strtok(NULL, seps));
		evt.ids[0] = atof(strtok(NULL, seps));
	}
	// change voice: [VOC][starttime][voicename]
	else if (strcmp(p_token, "VOC") == 0)
	{
		// chance voice on the fly
		// direct call to change voice from python side does not work well
		// for sapi object (releasing current voice)
		// so, let's do event queue instead direct call
		evt.type = EVT_VOICE;
		evt.eTime = atof(strtok(NULL, seps));
		strcpy(evt.param, strtok(NULL, seps));
	}
	// gui related: [GUI][starttime][subcmd:params]
	if (strncmp(p_token, "GUI", 3) == 0)
	{
		evt.eTime = atof(strtok(NULL, seps));
		
		p_token = strtok(NULL, seps);
		strcpy(str2, p_token);

		p_token = strtok(str2, seps2);

		// subcmd: [VIS:ON/OFF:guiname]
		if (strncmp(p_token, "VIS", 3) == 0)
		{
			evt.type = EVT_GUI_VISIBILITY;
			
			// On or Off
			p_token = strtok(NULL, seps2);
			evt.args[0] = strncmp(p_token, "ON", 2) == 0 ? 1:0;
			
			// gui window name
			p_token = strtok(NULL, seps2);
			strcpy(evt.param, p_token);
		}
		// subcmd: [MAT:targetGUI:matName]
		else if (strncmp(p_token, "MAT", 3) == 0)
		{
			evt.type = EVT_GUI_MATERIAL;

			// store two string type param with ':' delim
			strcpy(evt.param, strtok(NULL, seps2));
			strcat(evt.param, ":");
			strcat(evt.param, strtok(NULL, seps2));
		}

	}
	// message to material: [MAT][starttime][subcmd:parameters]
	else if (strncmp(p_token, "MAT", 3) == 0)
	{
		evt.eTime = atof(strtok(NULL, seps));

		// third params
		p_token = strtok(NULL, seps);
		strcpy(str2, p_token);

		// subcmd
		p_token = strtok(str2, seps2);

		// subcmd: [MSG:matName:play_mode:restart]
		if (strncmp(p_token, "MSG", 3) == 0)
		{
			evt.type = EVT_MAT_MESSAGE;
			strcpy(evt.param, strtok(NULL, seps2));
			p_token = strtok(NULL, seps2);
			while (p_token)
			{
				strcat(evt.param, ":");
				strcat(evt.param, p_token);
				p_token = strtok(NULL, seps2);
			}
		}
	}
	// fail to identify an event type, then simply return
	else
		return;

	m_pOwner->addEvent(evt);
}

//-----------------------------------------------------------------------------
void LLActivityBase::speaklisten(char* speech)
{
	if (!m_pOwner)
		return;

	m_pOwner->speak(speech, CSP_TTS_LISTEN);
}

void LLActivityBase::stopspeak()
{
	if (!m_pOwner)
		return;

	m_pOwner->stopSpeak();
}

//-----------------------------------------------------------------------------
bool LLActivityBase::isListening()
{
	if (!m_pOwner)
		return false;

	LLSpeechRecognizer* SR = m_pOwner->getSpeechRecognizer();

	if (SR)
		return SR->isListening();
	else
		return false;

}


//-----------------------------------------------------------------------------
void LLActivityBase::broadcastMsg(char* msg)
{
	m_pManager->broadcastMsg(msg);

	LLScreenLog::getSingleton().addText("LLActivityBase broadcast msg: " + Ogre::String(msg));
}

//-----------------------------------------------------------------------------
void LLActivityBase::requestSendMsg(char* msg)
{
	m_pManager->getRootActivity()->sendMsg(msg);

	LLScreenLog::getSingleton().addText("LLActivityBase request Send msg: " + Ogre::String(msg));
}

//-----------------------------------------------------------------------------
void LLActivityBase::sendMsg(std::string msg)
{

}

//-----------------------------------------------------------------------------
void LLActivityBase::msg_received(std::string msg)
{

}

//-----------------------------------------------------------------------------
void LLActivityBase::addDebug(char* dmsg)
{
	LLScreenLog::getSingleton().addText("py debug: " + Ogre::String(dmsg));
}

//-----------------------------------------------------------------------------
void LLActivityBase::setTipString(char* tip, float duration)
{
	if (!m_pOwner)
		return;

	m_pOwner->setTipString(tip, duration);

	LLScreenLog::getSingleton().addText("LLActivityBase set tips: " + Ogre::String(tip));
}

//-----------------------------------------------------------------------------
std::string LLActivityBase::getScriptDir()
{ 
	if (!m_pManager)
		return "";

	return m_pManager->getScriptDir();
}

#endif
