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
-----------------------------------------------------------------------------
Filename:    LifeLike.h
-----------------------------------------------------------------------------
This source file is generated by the Ogre AppWizard. Based on the Example 
Framework for OGRE (Object-oriented Graphics Rendering Engine)

Copyright (c) 2000-2007 The OGRE Team
For the latest info, see http://www.ogre3d.org/

Original source code is modified by Sangyoon Lee, Electronic Visualization 
Laboratory, University of Illinois at Chicago
-----------------------------------------------------------------------------
*/

#ifndef __LifeLike_h_
#define __LifeLike_h_

#include "BaseApplication.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif

#include "LLdefine.h"

class LLCharacter;
class LLSceneManager;
class LLScreenLog;
class LLNavigator;

class LifeLikeApp : public BaseApplication
{
public:
	LifeLikeApp(void);
	virtual ~LifeLikeApp(void);
	virtual void go(void);
	
	SceneManager*	getSceneManager() { return mSceneMgr;};
	LLSceneManager* getLLManager() { return m_pLLManager;};


	void			setTipEntry(char* msg, float duration=0.0f);
	void			showTipEntry(float duration=0.0f);
	void			setCaptionEntry(char* msg, float duration=0.0f);
	void			setMicVisible(bool visible = true);
	void			setMicMeter(float value);

	void			setGUIVisible(char* guiName, bool visible);
	void			setGUITexture(char* guiName, char* materialName);
	void			setMaterialMsg(char* matName, char* msg, char* param = NULL);

protected:
	virtual void	createScene(void);
	virtual bool	processUnbufferedKeyInput(const FrameEvent& evt);
	virtual bool	frameRenderingQueued(const FrameEvent& evt);
	virtual bool	frameStarted(const FrameEvent& evt);
	virtual bool	frameEnded(const FrameEvent& evt);
	virtual void	setupEventHandlers(void);

	bool			mouseMoved( const OIS::MouseEvent &arg );
	bool			mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
	bool			mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
	bool			keyPressed( const OIS::KeyEvent &arg );
	bool			keyReleased( const OIS::KeyEvent &arg );
	bool			menuButtonPressed(const CEGUI::EventArgs& e);


	bool			handleDictation(const CEGUI::EventArgs& e);
	bool			toggleDictationGUI(const CEGUI::EventArgs& e);

	// keyboard input related modifier keys
	bool			mAltDown;
	bool			mShiftDown;
	bool			mShouldRotate; 
    bool			mShouldTranslate;
	bool			mShouldZoom;
	bool			mShouldRotateZ;
    bool			mManaged;
	
	// pending exit
	bool			mGoodBye;

	// debug
	LLScreenLog*	mScreenDebugMessage;

	// LifeLike Scene Manager
	LLSceneManager*	m_pLLManager;

	// Navigation and Camera
	float			m_InitCamPos[3];
	float			m_InitCamLookAt[3];
	LLNavigator*	m_pNavigator;
	Vector3			mHeadTranslateVector;
	Vector3			mHeadInitialVector;

	// GUI related
	CEGUI::OgreRenderer* mGUIRenderer;

	// tip string
	CEGUI::Window*	m_tipContentsGUI;
	CEGUI::Window*	m_tipGUI;
	float			m_fTipTimer;

	// caption string
	CEGUI::Window*	m_captionContentsGUI;
	CEGUI::Window*	m_captionContentsGUIBG;
	CEGUI::Window*	m_captionGUI;
	float			m_fCaptionTimer;
	bool			m_bCaptionOn;

	// mic meter
	CEGUI::Window*	m_deactiveMicIcon;		// Mic indicator
	CEGUI::Window*	m_activeMicIcon;		// Mic indicator
	CEGUI::Window*	m_micWaveform;			// Mic waveform visualization
	CEGUI::ProgressBar*	m_micMeter;			// Simple integrated

	// dictation for various testing
	CEGUI::Window*				m_dictationGui;
	CEGUI::Window*				m_dictationInputEditBox;

};


extern LifeLikeApp	* g_pLLApp;


#endif // #ifndef __LifeLike_h_