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
* Filename: LLSceneManager.h
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#ifndef __LLSCENEMANAGER_H_
#define __LLSCENEMANAGER_H_

#include <Ogre.h>

using namespace Ogre;

class LLEntity;
class LLCharacter;
class LLSoundManager;
class LLMicMonitor;
class LLAudioMonitor;

#include "LLdefine.h"
#include "LLSound.h"

#define OIS_DYNAMIC_LIB
#include <OIS/OIS.h>

//-----------------------------------------------------------------------------
//  Entity (basic object)
//-----------------------------------------------------------------------------
class LLSceneManager
{
public:
	// -------------------------------------------------------------------------
	// Constructors and Destructor
	// -------------------------------------------------------------------------
	LLSceneManager(SceneManager* mgr);
	~LLSceneManager(void);

	void			loadScene();
	void			createLights();
	void			createCharacter(char* filename, int id);	// character xml file

	void			update(Real addedTime);
	void			processKeyInput(OIS::Keyboard* keyboard);
	void			processMouseInput( const OIS::MouseEvent &arg, int btID = -1, int pressed = 0);
	void			mouseMoved(int relX, int relY);

	void			resetOrientation() {m_pHeadNode->resetOrientation();}
	void			rotate(const Quaternion& q);
	void			setUserPosition(float x, float y, float z);
	void			setAttractionPosition(float x, float y, float z);
	bool			isSpeaking(int id = 0);
	void			setAnimationSpeed(float speed, bool relative=false);
	void			setSoundAnimation(int id = 0, float start=0, float end=1, float duration=1);

	// voice
	void			speak(int id, const String& speech, tCHARACTER_SPEECH_TYPE type = CSP_TTS);
	void			stopSpeak(int id);
	float			getMicLevel();

	// sound related
	LLSoundManager* getSoundManager() { return m_pSoundManager;}
	LLMicMonitor*	getMicMonitor() { return m_pMicMonitor;}

	int				getNumCharacters() { return m_iNumCharacter;}
	LLCharacter*	getCharacter(int id=0);

	// manual animation xml file loader
	void			createAnimationFromFile(const String& nodeName,const String& trackName,const String& filename);
	void			enableNodeAnimation(const String& trackName);

protected:
	
	// OGRE scene
	SceneManager*	m_pOGRESceneMgr;
	SceneNode*		m_pHeadNode;
	float			m_fAnimationSpeed;

	// Sound
	LLSoundManager*	m_pSoundManager;
	LLMicMonitor*	m_pMicMonitor;
	LLAudioMonitor*	m_pAudioMonitor;

	// Input
	Real			m_rTimeUntilNextToggle;

	// scene information
	int				m_iNumCharacter;
	LLCharacter**	m_pCharacters;
	float			m_fLigntIniDir;

	// log file...
	FILE*			m_logFile;

	// mouse picking test
	Ogre::RaySceneQuery* m_RayScnQuery;
	Ogre::SceneNode *m_CurrentPickedObject;

	// user defined animation sets (animation state)
	AnimationStateMap m_AnimationStates;
	EnabledAnimationStateList m_EnabledAnimationStates;

	// hair animaiton test
	Ogre::GpuProgramParametersSharedPtr m_ptrHairParams;
};

#endif
