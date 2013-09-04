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
* Filename: LLActivityManager.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#include <iostream>
#include <fstream>
#include <string>

#ifdef _WINDOWS

#include "boost/filesystem.hpp"
#include <boost/python/errors.hpp>
#include <boost/python/handle.hpp>

#include "LLActivityManager.h"
#include "LLActivityBase.h"
#include "LLCharacter.h"
#include "LLSpeechRecognizer.h"
#include "LLScreenLog.h"

// some definition for activity return status
#define ACTIVITY_SUSPEND -2		// current activity get into suspend mode
#define ACTIVITY_EXIT -1		// exiting current activity
#define ACTIVITY_STAY 0			// keep staying in current one

// Use Boost.Python to pack information concerning the Base class and its 
// wrapper class into a module.
BOOST_PYTHON_MODULE( LLActivityPlugin )
{
    python::class_<LLActivityBase, auto_ptr<LLActivityBaseWrap>, boost::noncopyable>( "LLActivityBase" )
		.def("setName", &LLActivityBase::setName)
		.def("getOwnerName", &LLActivityBase::getOwnerName)
		.def("getScriptDir", &LLActivityBase::getScriptDir)
		.def("registerTransition", &LLActivityBase::registerTransition)
		.def("addGrammar", &LLActivityBase::addGrammar)
		.def("addGrammarRule", &LLActivityBase::addGrammarRule)
		.def("setCurrentGrammar", &LLActivityBase::setCurrentGrammar)
		.def("setGrammarActive", &LLActivityBase::setGrammarActive)
		.def("speak", &LLActivityBase::speak)
		.def("speaklisten", &LLActivityBase::speaklisten)
		.def("stopspeak", &LLActivityBase::stopspeak)
		.def("muteMic", &LLActivityBase::muteMic)
		.def("unmuteMic", &LLActivityBase::unmuteMic)
		.def("isListening", &LLActivityBase::isListening)
		.def("broadcastMsg", &LLActivityBase::broadcastMsg)
		.def("requestSendMsg", &LLActivityBase::requestSendMsg)
		.def("addDebug", &LLActivityBase::addDebug)
		.def("setTipString", &LLActivityBase::setTipString)
		.def("addEvent", &LLActivityBase::addEvent);
	python::implicitly_convertible<auto_ptr<LLActivityBaseWrap>, auto_ptr<LLActivityBase> >();
}

//-----------------------------------------------------------------------------
// Name: readPythonScript()
// Desc: 
//-----------------------------------------------------------------------------
string *readPythonScript( string fileName )
{
	ifstream pythonFile;

	pythonFile.open( fileName.c_str() );

	if ( !pythonFile.is_open() ) 
	{
		cout << "Cannot open Python script file, \"" << fileName << "\"!" << endl;
		return NULL;
	}
	else
	{
		// Get the length of the file
		pythonFile.seekg( 0, ios::end );
		int nLength = pythonFile.tellg();
		pythonFile.seekg( 0, ios::beg );

		// Allocate  a char buffer for the read.
		char *buffer = new char[nLength];
		memset( buffer, 0, nLength );

		// read data as a block:
		pythonFile.read( buffer, nLength );

		string *scriptString = new string;
		scriptString->assign( buffer );

		delete [] buffer;
		pythonFile.close();

		return scriptString;
	}
}


//-----------------------------------------------------------------------------
LLActivityManager::LLActivityManager(LLCharacter* owner, const char* scriptdir)
{
	m_pOwner = owner;
	m_bInitialized = false;
	m_pCurrentActivity = NULL;
	m_pSuspendActivity = NULL;
	m_pRootActivity = NULL;
	m_uActivityIDGen = 0;

	m_bMute = false;

	if (scriptdir == NULL)
		m_sScriptDir = std::string("..\\..\\pyscripts");
	else
		m_sScriptDir = std::string(scriptdir);
}

//-----------------------------------------------------------------------------
LLActivityManager::~LLActivityManager()
{
	// delete all python classes
	
	// clean up debug stuff
	//PyRun_SimpleString("sys.stdout = sys.__stdout__");

	// Cleanup after Python...
	Py_Finalize();
}

//-----------------------------------------------------------------------------
bool LLActivityManager::init()
{
	// already initialized?
	if (m_bInitialized)
		return true;


	// scan script dir and get file names: using boost filesystem library
	namespace fs = boost::filesystem;
	fs::path script_path(fs::initial_path<fs::path>() );
	script_path = fs::system_complete( fs::path( m_sScriptDir.c_str(), fs::native ) );
	
	unsigned long file_count = 0;
	unsigned long script_count = 0;

	if (!fs::exists(script_path))
		return false;	

	std::string* scripts;
	std::string corescript;
	std::string tempstr;

	if ( fs::is_directory( script_path ) )
	{
		// pre-count the number of files in dedicated script directory
		fs::directory_iterator end_iter;
		for ( fs::directory_iterator dir_itr( script_path ); dir_itr != end_iter; ++dir_itr )
		{
			try
			{
				if (!boost::filesystem::is_directory(*dir_itr) && 
					boost::filesystem::extension(*dir_itr) == ".py")
				{
					// hack to filter mac's ._ files
					tempstr = dir_itr->leaf();
					if (tempstr.c_str()[0] != '.')
						++script_count;
				}
			}
			catch ( const std::exception & ex )
			{
				// error
				LLScreenLog::getSingleton().addText("python script init exception");
			}
		}
		
		// build up script file name array
		if (script_count !=0)
		{
			// then build up filename array
			scripts = new std::string[script_count];
			int idx = 0;
			for ( fs::directory_iterator dir_itr( script_path ); dir_itr != end_iter; ++dir_itr )
			{
				try
				{
					if (!boost::filesystem::is_directory(*dir_itr) && 
						boost::filesystem::extension(*dir_itr) == ".py")
					{
						// is this core script file?
						tempstr = dir_itr->leaf();
						if (tempstr.c_str()[0] != '.')
						{
							if ( strcmp (tempstr.c_str(), "CoreScript.py") == 0)
							{
								corescript = script_path.native_directory_string() + "\\" + tempstr;
							}
							else
							{
								scripts[idx] = script_path.native_directory_string() + "\\" + dir_itr->leaf();
								++idx;
							}
						}
					}
				}
				catch ( const std::exception & ex )
				{
					// error
					LLScreenLog::getSingleton().addText("python script init exception in finding script path");
				}
			}
		}
		else
			return false;
	}
	else // must be a file
	{
		// error: we are expecting script directory
		return false;
	}
	
	// make sure we found core script file
	if (corescript.length() == 0)
		return false;

	// lets decrement file count by one to exclude core script file
	script_count--;

	// Register the module with the interpreter. It's important to do this 
	// before the call to Py_Initialize(). Note that the function called 
	// "initLLActivityPlugin" is automatically declared by the BOOST_PYTHON_MODULE
	// macro, which was declared earlier in this file.
	if( PyImport_AppendInittab( "LLActivityPlugin", initLLActivityPlugin ) == -1 )
		throw runtime_error( "Failed to add \"LLActivityPlugin\" to the "
		                     "interpreter's built-in modules" );

	// Setup Python to be embedded...
	Py_Initialize();

	// some extra for debuggin...
	//PyRun_SimpleString("sys.stdout = open(\"pythonout.txt\", 'a')"); 
	PyRun_SimpleString("import cStringIO");
	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.stderr=cStringIO.StringIO()");

	// Access the "__main__" module and its name space dictionary.
	python::handle<> hMainModule( python::borrowed( PyImport_AddModule( "__main__" ) ) );
	python::handle<> hMainNamespace( python::borrowed( PyModule_GetDict( hMainModule.get() ) ) );
	

	// create root activity first (CoreScript.py)
	LLActivityBase *pActivity;
	try
	{
		// Load and run the Python script as a string.
		string *pythonScript = readPythonScript( corescript );
		PyObject *result;
		if( pythonScript != NULL )
		{
			result = PyRun_String( pythonScript->c_str(), Py_file_input,
							hMainNamespace.get(), hMainNamespace.get() );
			if (result == NULL)
			{
				PyErr_Print();

				fprintf(stderr, "************** python parsing error occured...\n");
				boost::python::object sys(boost::python::handle<>(PyImport_ImportModule("sys")));
				boost::python::object err = sys.attr("stderr");
				std::string err_text = boost::python::extract<std::string>(err.attr("getvalue")());
				fprintf(stderr, err_text.c_str());

				LLScreenLog::getSingleton().addText("python script parsing error: " + err_text);
			}
		}

		delete pythonScript;

		// Extract the raw Python object representing the derived class defined in python script
		python::handle<> hClassPtr( PyRun_String( "PythonActivity", Py_eval_input,
												  hMainNamespace.get(), 
												  hMainNamespace.get()) );

		// Wrap the raw Python object in a Boost.Python object
		python::object PythonDerived( hClassPtr );

		// Creating an instance of our Python derived class...
		python::object pyBase = PythonDerived();
		pActivity = python::extract< LLActivityBase* >( pyBase );
		
		pActivity->setID(m_uActivityIDGen);
		m_ActivityMap.insert( iActivityMapVt(m_uActivityIDGen, pActivity) );
		m_uActivityIDGen++;

		pActivity->setOwner(m_pOwner, this);
		pActivity->initialize();

		m_pRootActivity = pActivity;
	}
	catch (boost::python::error_already_set const & e)
	{
		// we got exception here
		PyErr_Print();

		fprintf(stderr, "************** python parsing error occured...\n");
		boost::python::object sys(boost::python::handle<>(PyImport_ImportModule("sys")));
		boost::python::object err = sys.attr("stderr");
		std::string err_text = boost::python::extract<std::string>(err.attr("getvalue")());
		fprintf(stderr, err_text.c_str());

		LLScreenLog::getSingleton().addText("python script init exception: " + err_text);
	}


	// create & register all python defined activities
	for (int i=0; i < script_count; i++)
	{
		try
		{
			// Load and run the Python script as a string.
			string *pythonScript = readPythonScript( scripts[i] );
			
			LLScreenLog::getSingleton().addText("python loading script: " + Ogre::String(scripts[i]));

			if( pythonScript != NULL )
			{
				PyRun_String( pythonScript->c_str(), Py_file_input,
								hMainNamespace.get(), hMainNamespace.get() );
			}

			delete pythonScript;

			// Extract the raw Python object representing the derived class defined in python script
			python::handle<> hClassPtr( PyRun_String( "PythonActivity", Py_eval_input,
													  hMainNamespace.get(), 
													  hMainNamespace.get()) );

			// Wrap the raw Python object in a Boost.Python object
			python::object PythonDerived( hClassPtr );

			// Creating an instance of our Python derived class...
			python::object pyBase = PythonDerived();
			pActivity = python::extract< LLActivityBase* >( pyBase );
			
			pActivity->setID(m_uActivityIDGen);
			m_ActivityMap.insert( iActivityMapVt(m_uActivityIDGen, pActivity) );
			m_uActivityIDGen++;

			pActivity->setOwner(m_pOwner, this);
			pActivity->initialize();
		}
		catch (python::error_already_set const & e)
		{
			// we got exception here
			PyErr_Print();

			fprintf(stderr, "************** python parsing error occured...\n");
			boost::python::object sys(boost::python::handle<>(PyImport_ImportModule("sys")));
			boost::python::object err = sys.attr("stderr");
			std::string err_text = boost::python::extract<std::string>(err.attr("getvalue")());
			fprintf(stderr, err_text.c_str());

			LLScreenLog::getSingleton().addText("python script init exception" + err_text);
		}
	}
		
	delete[] scripts;

	if (m_pRootActivity)
	{
		LLScreenLog::getSingleton().addText("LLActivityManager activate rootactivity");
		m_pRootActivity->setActive("");
		m_bInitialized = true;
		LLScreenLog::getSingleton().addText("LLActivityManager initialized");
		return true;
	}
	else
		return false;

}

//-----------------------------------------------------------------------------
void LLActivityManager::addActivityTransition(unsigned int ActivityId, std::string str)
{
	// add this SR stirng to root grammar
	// root activity never call this function since it is before generation of itself
	if (!m_pRootActivity)
		return;

	// in the first grammar (id 1, root activity grammar)
	// the first 100 IDs are reserved for root itself
	// therefore, activity transtion is shifted by 100
	char rulename[64];
	sprintf(rulename, "ACTIVITYTRANSITION_%i", ActivityId);
	int id = ActivityId + 100;

	LLSpeechRecognizer* SR = m_pOwner->getSpeechRecognizer();
	if (SR)
		SR->addGrammarRule(1, id, rulename, (char*)str.c_str());
}

//-----------------------------------------------------------------------------
void LLActivityManager::reset()
{
	if (!m_bInitialized)
		return;
}

//-----------------------------------------------------------------------------
void LLActivityManager::processRecognizedEvent(int gramId, int ruleId, char conf, char* listened, char* rulename)
{
	if (m_bMute)
		return;

	LLScreenLog::getSingleton().addText("LLActivityManager processing SR evt:");
	LLScreenLog::getSingleton().addText("\tgramId(" + StringConverter::toString(gramId) + 
										"), ruleId(" + StringConverter::toString(ruleId) + 
										"), rulename(" + Ogre::String(rulename) +
										"), conf(" + StringConverter::toString(conf) + ")");
	LLScreenLog::getSingleton().addText("\tlistened: " + Ogre::String(listened));

	// if gramID is 1, then it is root state recognition
	// otherwise, pass it to current activity class
	int result = -1;

	// we have two grammar running at the same time
	// one is root level activity and the other is current front end activity
	// root activiy has sigle grammar with id 1
	try
	{
		// processRecognition returns int
		// -1: suspend current activity
		//  0: stays in current activity
		//  others:
		if (gramId == 1)
		{
			// process recognition
			if (m_pRootActivity)
				result = m_pRootActivity->processRecognition(gramId, ruleId, conf, listened, rulename);
			else
				return;

			LLScreenLog::getSingleton().addText("Root Activity SR process result: " + StringConverter::toString(result));

			// if the result is not -1, it means we got new transition to ohter activity
			if (result > 0)
			{
				// reset current activity & assign new one
				if (m_pCurrentActivity)
				{
					m_pCurrentActivity->suspend();
					m_pSuspendActivity = m_pCurrentActivity;
					//m_pCurrentActivity->reset();
					LLScreenLog::getSingleton().addText("LLActivityManager suspend Activity: " +
						StringConverter::toString(m_pCurrentActivity->getID()));
				}
				
				// here we shift back ruleId by 100 so that we have corrected activity index in map
				int nextActId = result - 100;
				iActivityMapIter itr = m_ActivityMap.find(nextActId);
				LLActivityBase* tAct = (*itr).second;
				m_pCurrentActivity = tAct;
				if (tAct != NULL)
				{
					m_pCurrentActivity->setActive(listened);
					LLScreenLog::getSingleton().addText("LLActivityManager transition to Activity: " +
						StringConverter::toString(m_pCurrentActivity->getID()));
				}
			}
			
			return;
		}
		else
		{
			if (m_pCurrentActivity)
				result = m_pCurrentActivity->processRecognition(gramId, ruleId, conf, listened, rulename);
			else
			{
				// this is error in fact
				return;
			}

			// if the result is not -1, it means that current activity left its task
			// then, there is not active one
			if (result == ACTIVITY_EXIT)
			{
				m_pCurrentActivity = NULL;
				LLScreenLog::getSingleton().addText("LLActivityManager got exiting current activity in processing SR");

				if (m_pSuspendActivity)
				{
					m_pSuspendActivity->resume();
					m_pCurrentActivity = m_pSuspendActivity;
					m_pSuspendActivity = NULL;
				}
			}
			else if (result == ACTIVITY_SUSPEND)
			{
				// add current activity to suspended activity list
				// and set current pointer to NULL
				m_pSuspendActivity = m_pCurrentActivity;
				m_pCurrentActivity = NULL;
			}
			LLScreenLog::getSingleton().addText("LLActivity processing SR returned gramid: " + StringConverter::toString(result));
		}
	}
	catch (python::error_already_set const & e) 
	{
		// we got exception here
		fprintf(stderr, "************** python parsing error occured...\n");
		PyErr_Print();
	}

}

//-----------------------------------------------------------------------------
void LLActivityManager::update(float addedTime)
{
	if (m_pRootActivity)
		m_pRootActivity->update(addedTime);

	if (m_pCurrentActivity)
	{
		if (m_pCurrentActivity->update(addedTime) == ACTIVITY_EXIT)
		{				
			// silently resume suspended activity
			m_pCurrentActivity = m_pSuspendActivity;
			m_pSuspendActivity = NULL;
			if (m_pCurrentActivity)
				m_pCurrentActivity->resume();

#ifdef DEBUG
			LLScreenLog::getSingleton().addText("LLActivityManager got exiting current activity in updating");
#endif
		}

	}
}

//-----------------------------------------------------------------------------
void LLActivityManager::broadcastMsg(char* msg)
{
#ifdef DEBUG
	LLScreenLog::getSingleton().addText("LLActivityManager broadcast msg: " + Ogre::String(msg));
#endif

	// iterate all registered activity and broadcast this msg
	for(iActivityMap::const_iterator it = m_ActivityMap.begin(); it != m_ActivityMap.end(); ++it)
    {
        LLActivityBase* tAct = (*it).second;
		tAct->msg_received(msg);
    }
	
}

//-----------------------------------------------------------------------------
void LLActivityManager::sendMsg(char* msg)
{
#ifdef DEBUG
	LLScreenLog::getSingleton().addText("LLActivityManager send msg to rootactivity: " + Ogre::String(msg));
#endif

	// assume that only core activity send msg
	if (m_pRootActivity)
		m_pRootActivity->sendMsg(msg);
}

#endif
