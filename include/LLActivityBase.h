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
* Filename: LLActivityBase.h
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#ifndef __LLACTIVITYBASE_H__
#define __LLACTIVITYBASE_H__

#ifdef _WINDOWS

#include <boost/python.hpp>
namespace python = boost::python;

class LLCharacter;
class LLActivityManager;

///////////////////////////////////////////////////////////////////////////////
// An abstract ActivityBase class...
///////////////////////////////////////////////////////////////////////////////
class LLActivityBase : public boost::noncopyable
{
public:

	LLActivityBase() {};
    virtual ~LLActivityBase() {};

	// initialization of internal state
	virtual void initialize() = 0;
	virtual void deinitialize() = 0;
	
	// 
	virtual void setActive(std::string str) = 0;

	virtual void reset() = 0;
	virtual void suspend() = 0;
	virtual void resume() = 0;

	virtual int update(float addedTime) = 0;
	
	// process recognized string
	virtual int processRecognition(int gid, int rid, char conf, std::string listened, std::string rulename) = 0;

	// C++ only
	void setOwner(LLCharacter* owner, LLActivityManager* mgr) 
	{ 
		m_pOwner = owner;
		m_pManager = mgr;
	}
	
	// python callable method
	void setName(char* name) { m_Name = std::string(name);}

	// register transition strings from root to this activity
	void registerTransition(char* reco);
	std::string getTransitionString() { return m_sTransitionStr;}
	unsigned int getID() { return m_uID; }
	void setID(unsigned int id) { m_uID = id;}

	// SR Grammar related
	int	addGrammar(char* gram);									// return grammar id
	int	addGrammarRule(int gramId, char* rule, char* phrase);	// return rule id (this decide next state)
	void setCurrentGrammar(int gramId);
	void setGrammarActive(int gramId, bool active);

	void speak(char* speech);		// default speech type is CSP_TTS:0
	void speaklisten(char* speech);
	void stopspeak();
	bool isListening();

	void muteMic();
	void unmuteMic();

	void addEvent(char* evtstr);

	void addDebug(char* dmsg);

	void setTipString(char* tip, float duration=0.0f);

	// communication related
	virtual void broadcastMsg(char* msg);	// broadcast msg to all activity
	virtual void requestSendMsg(char* msg);	// send msg to dedicated destination (only core activity responsible for this)
	virtual void sendMsg(std::string msg) = 0;
	virtual void msg_received(std::string msg) = 0;	// python side msg handling method

	std::string getOwnerName();
	std::string getScriptDir();

private:

	LLCharacter*		m_pOwner;
	LLActivityManager*	m_pManager;
	std::string			m_Name;
	std::string			m_sTransitionStr;
	unsigned int		m_uID;

};

///////////////////////////////////////////////////////////////////////////////
// A C++ Derived class...
///////////////////////////////////////////////////////////////////////////////
class LLActivity : public LLActivityBase
{
public:

	LLActivity() {}
    virtual ~LLActivity() {}

	void initialize() 
	{
		// build up states and functions...
		registerTransition("register\n");
		addGrammar("grammar\n");
		addGrammarRule(0, "rule", "grammar rule\n");
		setCurrentGrammar(1);
		speak("speaking\n");
		speaklisten("speaking and listening\n");
		stopspeak();
		setTipString("tip\n", 0.0f);
		addEvent("addEvent\n");
		muteMic();
		unmuteMic();
	}

	void deinitialize() {}

	void setActive(std::string str)
	{

	}

	int processRecognition(int gid, int rid, char conf, std::string listened, std::string rulename)
	{
		// C++ derived processRecognition method
		return 0;
	}

	void reset() {}
	void suspend() {}
	void resume() {}
	int update(float addedTime) {}
	void msg_received(std::string msg) {}
	void sendMsg(std::string msg) {}

};

///////////////////////////////////////////////////////////////////////////////
// Boost.Python wrapper class for Base
///////////////////////////////////////////////////////////////////////////////
struct LLActivityBaseWrap : public LLActivityBase
{
    LLActivityBaseWrap(PyObject* self_) : self(self_) 
	{
		Py_INCREF(self);
	}
	virtual ~LLActivityBaseWrap()
	{
		deinitialize();
		Py_DECREF(self);
	}

	void initialize()
	{
		boost::python::call_method<void>(self, "initialize");
	}

	void deinitialize()
	{
		boost::python::call_method<void>(self, "deinitialize");
	}

	void setActive(std::string str)
	{
		boost::python::call_method<void>(self, "setActive", str);
	}

	int processRecognition(int gid, int rid, char conf, std::string listened, std::string rulename)
	{
		return boost::python::call_method<int>(self, "processRecognition", gid, rid, conf, listened, rulename);
	}

	void reset()
	{
		boost::python::call_method<void>(self, "reset");
	}

	void suspend()
	{
		boost::python::call_method<void>(self, "suspend");
	}

	void resume()
	{
		boost::python::call_method<void>(self, "resume");
	}

	int update(float addedTime)
	{
		return boost::python::call_method<int>(self, "update", addedTime);
	}

	void msg_received(std::string msg)
	{
		boost::python::call_method<void>(self, "msg_received", msg);
	}

	void sendMsg(std::string msg)
	{
		boost::python::call_method<void>(self, "sendMsg", msg);
	}

    PyObject* self;
};

#endif

#endif
