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
/*
-----------------------------------------------------------------------------
Filename:    BaseApplication.cpp
-----------------------------------------------------------------------------
This source file is generated by the Ogre AppWizard. Based on the Example 
Framework for OGRE (Object-oriented Graphics Rendering Engine)

Copyright (c) 2000-2007 The OGRE Team
For the latest info, see http://www.ogre3d.org/
-----------------------------------------------------------------------------
*/

#include "BaseApplication.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include "../res/resource.h"
#endif

#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR > 7 // If you're using a version newer than 1.7.
	template<> BaseApplication* Ogre::Singleton<BaseApplication>::msSingleton = 0;
#else
	template<> BaseApplication* Ogre::Singleton<BaseApplication>::ms_Singleton = 0;
#endif

//template<> BaseApplication* Ogre::Singleton<BaseApplication>::ms_Singleton = 0;

//-----------------------------------------------------------------------------
CEGUI::MouseButton convertOISMouseButtonToCegui(int buttonID)
{
    switch (buttonID)
    {
	case 0: return CEGUI::LeftButton;
	case 1: return CEGUI::RightButton;
	case 2:	return CEGUI::MiddleButton;
	case 3: return CEGUI::X1Button;
	default: return CEGUI::LeftButton;
    }
}

//-----------------------------------------------------------------------------
BaseApplication::BaseApplication(void)
	: mRoot(0),
	mPrimaryCamera(0),
	mSecondaryCamera(0),
	mSceneMgr(0),
	mWindow(0),
	mWindow2(0),
	mShutdownRequested(false),
	mSceneDetailIndex(0),
	mMoveSpeed(100),
	mRotateSpeed(36),
	mDebugOverlay(0),
	mInputManager(0),
	mTrayMgr(0),
	mMouse(0),
	mKeyboard(0),
	mTranslateVector(Vector3::ZERO),
	mStatsOn(true),
	mUseBufferedInputKeys(false),
	mUseBufferedInputMouse(true),
	mInputTypeSwitchingOn(false),
	mNumScreenShots(0),
	mMoveScale(0.0f),
	mRotScale(0.0f),
	mTimeUntilNextToggle(0),
	mRotX(0),
	mRotY(0)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
		mResourcePath = macBundlePath() + "/Contents/Resources/";
#else
		mResourcePath = "";
#endif
}

//-----------------------------------------------------------------------------
BaseApplication::~BaseApplication(void)
{
    if (mTrayMgr) delete mTrayMgr;

	//Remove ourself as a Window listener
	WindowEventUtilities::removeWindowEventListener(mWindow, this);
	windowClosed(mWindow);
	
	OGRE_DELETE mRoot;
}

//-----------------------------------------------------------------------------
bool BaseApplication::configure(void)
{
	// Show the configuration dialog and initialise the system
	// You can skip this and use root.restoreConfig() to load configuration
	// settings if you were sure there are valid ones saved in ogre.cfg
	if(mRoot->restoreConfig() || mRoot->showConfigDialog())
	{
		// If returned true, user clicked OK so initialise
		// Here we choose to let the system create a default rendering window by passing 'true'
		//mWindow = mRoot->initialise(true, "LifeLike");

		// custom window creation: i.e. borderless window for tiled display
		// note that stereomanager or other type of plugins that use viewport listener binding
		// there is no way for them to find primary render window
		// since Ogre::Root only provide interface to retrieve automatic window
		// as quick hack, changed stereomanager to find primary rendertarget from current active rendersystem
		ConfigFile cf;
		cf.load("LifeLike.cfg");
		String cscreen = cf.getSetting("Use", "CustomScreen", "false");
		if (StringConverter::parseBool(cscreen))
		{
			// initialise root
			mRoot->initialise(false, "LifeLike");

			bool fullscreen = false;
			int cw, ch;
			String top, left, border;
			top = cf.getSetting("ScreenTop", "CustomScreen", "0");
			left = cf.getSetting("ScreenLeft", "CustomScreen", "0");
			border = cf.getSetting("Border", "CustomScreen", "none");

			// create custom render window
			// use can set border type, top, left 
			// width, height and other options should be obtained from Ogre configuration
			RenderSystem* re = mRoot->getRenderSystem();
			ConfigOptionMap cmap = re->getConfigOptions();
			ConfigOptionMap::iterator opt;
			ConfigOptionMap::iterator end = cmap.end();

			Ogre::NameValuePairList misc;
			misc["border"] = border;							// none, ...
			misc["left"] = left;								// int value
			misc["top"] = top;									// int value

			if((opt = cmap.find("Full Screen")) != end)
				fullscreen = (opt->second.currentValue == "Yes");

			if((opt = cmap.find("Display Frequency")) != end)
				misc["displayFrequency"] = opt->second.currentValue;

			if((opt = cmap.find("Video Mode")) != end)
			{
				String val = opt->second.currentValue;
				String::size_type pos = val.find('x');

				if (pos != String::npos)
				{
					cw = StringConverter::parseUnsignedInt(val.substr(0, pos));
					ch = StringConverter::parseUnsignedInt(val.substr(pos + 1));
				}
			}

			if((opt = cmap.find("FSAA")) != end)
				misc["FSAA"] = opt->second.currentValue;

			if((opt = cmap.find("VSync")) != end)
				misc["vsync"] = opt->second.currentValue;

			if((opt = cmap.find("sRGB Gamma Conversion")) != end)
				misc["gamma"] = opt->second.currentValue;
			

			// create custom render window			
			mWindow = mRoot->createRenderWindow("LifeLike", cw, ch, fullscreen, &misc);
			//misc["left"] = "0";
			//misc["monitorIndex"] = "0";
			//mWindow2 = mRoot->createRenderWindow("LifeLike2", cw, ch, fullscreen, &misc);
			//mWindow2->setDeactivateOnFocusChange(false);
		}
		else
		{
			// use Ogre's automatic initialise with auto render window creation			
			mWindow = mRoot->initialise(true, "LifeLike");
		}


		// Let's add a nice window icon
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		HWND hwnd;
		mWindow->getCustomAttribute("WINDOW", (void*)&hwnd);
		LONG iconID   = (LONG)LoadIcon( GetModuleHandle(0), MAKEINTRESOURCE(IDI_APPICON) );
		SetClassLong( hwnd, GCL_HICON, iconID );
		windonHandle = hwnd;
#endif

		return true;

	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
void BaseApplication::chooseSceneManager(void)
{
	// Get the SceneManager, in this case a generic one
	mSceneMgr = mRoot->createSceneManager(ST_GENERIC, "LifeLikeSceneManager");
}

//-----------------------------------------------------------------------------
void BaseApplication::createCamera(void)
{
	// Create the camera
	mPrimaryCamera = mSceneMgr->createCamera("PlayerPrimaryCam");

	// Position it at 500 in Z direction
	//mPrimaryCamera->setPosition(Vector3(0,0,80));
	mPrimaryCamera->setPosition(Vector3(0,0,0));
	// Look back along -Z
	mPrimaryCamera->lookAt(Vector3(0,0,-300));
	//mPrimaryCamera->setNearClipDistance(0.5);
	mPrimaryCamera->setNearClipDistance(50);

	// shadow test
	mPrimaryCamera->setFarClipDistance(2000);

	// create camera node
	m_pPrimaryCameraNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("PlayerPrimaryCamNode");
	m_pPrimaryCameraNode->attachObject(mPrimaryCamera);

}

//-----------------------------------------------------------------------------
void BaseApplication::setupInput()
{
	OIS::ParamList pl;
	size_t winHandle = 0;
	std::ostringstream winHandleStr;

	mWindow->getCustomAttribute("WINDOW", &winHandle);
	winHandleStr << winHandle;

	pl.insert(std::make_pair("WINDOW", winHandleStr.str()));

	mInputManager = OIS::InputManager::createInputSystem(pl);

	createInputDevices();      // create the specific input devices

	windowResized(mWindow);    // do an initial adjustment of mouse area

}

//-----------------------------------------------------------------------------
void BaseApplication::createInputDevices()
{
#if OGRE_PLATFORM == OGRE_PLATFORM_IPHONE
	mMouse = static_cast<OIS::MultiTouch*>(mInputMgr->createInputObject(OIS::OISMultiTouch, true));
	mAccelerometer = static_cast<OIS::JoyStick*>(mInputMgr->createInputObject(OIS::OISJoyStick, true));
#else
	mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject(OIS::OISKeyboard, mUseBufferedInputKeys));
	mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject(OIS::OISMouse, mUseBufferedInputMouse));

	mKeyboard->setEventCallback(this);
#endif
	mMouse->setEventCallback(this);
}

//-----------------------------------------------------------------------------
void BaseApplication::createFrameListener(void)
{
	mDebugOverlay = OverlayManager::getSingleton().getByName("Core/DebugOverlay");
	mUseBufferedInputKeys = false;
	mUseBufferedInputMouse = true;
	mInputTypeSwitchingOn = mUseBufferedInputKeys || mUseBufferedInputMouse;
	mRotateSpeed = 36;
	mMoveSpeed = 100;

	using namespace OIS;

	LogManager::getSingletonPtr()->logMessage("*** Initializing OIS ***");
	ParamList pl;
	size_t windowHnd = 0;
	std::ostringstream windowHndStr;

	mWindow->getCustomAttribute("WINDOW", &windowHnd);
	windowHndStr << windowHnd;
	pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

	mShutdownRequested = false;

	//Register as a Window listener
	WindowEventUtilities::addWindowEventListener(mWindow, this);

	mStatsOn = true;
	mNumScreenShots = 0;
	mTimeUntilNextToggle = 0;
	mSceneDetailIndex = 0;
	mMoveScale = 0.0f;
	mRotScale = 0.0f;
	mTranslateVector = Ogre::Vector3::ZERO;

	showDebugOverlay(true);
	mRoot->addFrameListener(this);

	//
	MouseState &ms = const_cast<MouseState &>(mMouse->getMouseState());
	ms.X.abs = mWindow->getWidth() / 2;
	ms.Y.abs = mWindow->getHeight() / 2;

	// Ogre Bite
    mTrayMgr->showFrameStats(OgreBites::TL_BOTTOMLEFT);
    mTrayMgr->hideCursor();
    // create a params panel for displaying sample details
    Ogre::StringVector items;
    items.push_back("cam.pX");
    items.push_back("cam.pY");
    items.push_back("cam.pZ");
    items.push_back("");
    items.push_back("cam.oW");
    items.push_back("cam.oX");
    items.push_back("cam.oY");
    items.push_back("cam.oZ");
    items.push_back("");
	items.push_back("cam.FOVy");
	items.push_back("");
    items.push_back("Filtering");
    items.push_back("Poly Mode");

    mDetailsPanel = mTrayMgr->createParamsPanel(OgreBites::TL_NONE, "DetailsPanel", 200, items);
	mTrayMgr->moveWidgetToTray(mDetailsPanel, OgreBites::TL_TOPRIGHT, 0);
    mDetailsPanel->setParamValue(11, "Bilinear");
    mDetailsPanel->setParamValue(12, "Solid");

	//mTrayMgr->hideAll();
}

//-----------------------------------------------------------------------------
void BaseApplication::destroyScene(void)
{
}

//-----------------------------------------------------------------------------
void BaseApplication::createViewports(void)
{
	// Create one viewport, entire window
	Viewport* vp = mWindow->addViewport(mPrimaryCamera);
	vp->setBackgroundColour(ColourValue(0,0,0,0));
	//vp->setBackgroundColour(ColourValue(0,0.5,0,0));	// for power project
	
	// Alter the camera aspect ratio to match the viewport
	mPrimaryCamera->setAspectRatio( Real(vp->getActualWidth()) / 
									Real(vp->getActualHeight()) );
	if (mWindow2)
	{
		Viewport* vp2 = mWindow2->addViewport(mPrimaryCamera);
		vp2->setOverlaysEnabled(false);
	}
}

//-----------------------------------------------------------------------------
void BaseApplication::setupResources(void)
{
	// Load resource paths from config file
	ConfigFile cf;
	cf.load("resources.cfg");

	// Go through all sections & settings in the file
	ConfigFile::SectionIterator seci = cf.getSectionIterator();

	String secName, typeName, archName;
	while (seci.hasMoreElements())
	{
		secName = seci.peekNextKey();
		ConfigFile::SettingsMultiMap *settings = seci.getNext();
		ConfigFile::SettingsMultiMap::iterator i;
		for (i = settings->begin(); i != settings->end(); ++i)
		{
			typeName = i->first;
			archName = i->second;
			ResourceGroupManager::getSingleton().addResourceLocation(
				archName, typeName, secName);
		}
	}
}

//-----------------------------------------------------------------------------
void BaseApplication::createResourceListener(void)
{

}

//-----------------------------------------------------------------------------
void BaseApplication::loadResources(void)
{
	mTrayMgr->showLoadingBar(2,2,0.7);
    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	mTrayMgr->hideLoadingBar();
}

//-----------------------------------------------------------------------------
void BaseApplication::go(void)
{
	// virtual func used the one in LifeLike.h
	if (!setup())
		return;

	showDebugOverlay(true);

	mRoot->startRendering();

	// clean up
	destroyScene();
}

//-----------------------------------------------------------------------------
bool BaseApplication::setup(void)
{
	//mRoot = OGRE_NEW Root();
	String pluginsPath;
	
	// only use plugins.cfg if not static
#ifndef OGRE_STATIC_LIB
	pluginsPath = mResourcePath + "plugins.cfg";
#endif
		
	mRoot = OGRE_NEW Root(pluginsPath, 
            mResourcePath + "ogre.cfg", mResourcePath + "Ogre.log");

	setupResources();

	bool carryOn = configure();
	if (!carryOn) return false;

	chooseSceneManager();
	createCamera();
	createViewports();

	// Set default mipmap level (NB some APIs ignore this)
	TextureManager::getSingleton().setDefaultNumMipmaps(5);

	// Create any resource listeners (for loading screens)
	createResourceListener();

	///////////////////
	setupInput();
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Essential");
	mTrayMgr = new OgreBites::SdkTrayManager("InterfaceName", mWindow, mMouse, this);

	// Load resources
	loadResources();

	// Create the scene
	createScene();

	// Create Frame listener
	createFrameListener();

	return true;
}

//-----------------------------------------------------------------------------
bool BaseApplication::processUnbufferedKeyInput(const FrameEvent& evt)
{
	// virtual func. used the one in LifeLike.h
	// Return true to continue rendering
	return true;
}

//-----------------------------------------------------------------------------
bool BaseApplication::processUnbufferedMouseInput(const FrameEvent& evt)
{
	// virtual func. used the one in LifeLike.h
	
	using namespace OIS;

	// Rotation factors, may not be used if the second mouse button is pressed
	// 2nd mouse button - slide, otherwise rotate
	const MouseState &ms = mMouse->getMouseState();
	if( ms.buttonDown( MB_Right ) )
	{
		mTranslateVector.x += ms.X.rel * 0.13;
		mTranslateVector.y -= ms.Y.rel * 0.13;
	}
	else
	{
		mRotX = Degree(-ms.X.rel * 0.13);
		mRotY = Degree(-ms.Y.rel * 0.13);
	}

	return true;
}

//-----------------------------------------------------------------------------
void BaseApplication::moveCamera()
{
}

//-----------------------------------------------------------------------------
void BaseApplication::showDebugOverlay(bool show)
{
	if (mDebugOverlay)
	{
		if (show)
		{
			mDebugOverlay->show();
		}
		else
		{
			mDebugOverlay->hide();
		}
	}
}

//-----------------------------------------------------------------------------
void BaseApplication::requestShutdown(void)
{
	mShutdownRequested = true;
}

//-----------------------------------------------------------------------------
bool BaseApplication::frameStarted(const FrameEvent& evt)
{
	if(mWindow->isClosed())
		return false;

	//Need to capture/update each device
	mKeyboard->capture();
	mMouse->capture();

	mUseBufferedInputMouse = mMouse->buffered();
	mUseBufferedInputKeys = mKeyboard->buffered();


      if(mUseBufferedInputMouse)
      {
         CEGUI::MouseCursor::getSingleton().show( );
      }
      else
      {
         CEGUI::MouseCursor::getSingleton().hide( );
      }

	if ( !mUseBufferedInputMouse || !mUseBufferedInputKeys)
	{
		// one of the input modes is immediate, so setup what is needed for immediate mouse/key movement
		if (mTimeUntilNextToggle >= 0)
			mTimeUntilNextToggle -= evt.timeSinceLastFrame;

		// If this is the first frame, pick a speed
		if (evt.timeSinceLastFrame == 0)
		{
			mMoveScale = 1;
			mRotScale = 0.1;
		}
		// Otherwise scale movement units by time passed since last frame
		else
		{
			// Move about 100 units per second,
			mMoveScale = mMoveSpeed * evt.timeSinceLastFrame;
			// Take about 10 seconds for full rotation
			mRotScale = mRotateSpeed * evt.timeSinceLastFrame;
		}
		mRotX = 0;
		mRotY = 0;
		mTranslateVector = Vector3::ZERO;
	}

	if (mUseBufferedInputKeys)
	{
		// no need to do any processing here, it is handled by event processor and
		// you get the results as KeyEvents
	}
	else
	{
		if (processUnbufferedKeyInput(evt) == false)
		{
			return false;
		}
	}
	if (mUseBufferedInputMouse)
	{
		// no need to do any processing here, it is handled by event processor and
		// you get the results as MouseEvents
	}
	else
	{
		if (processUnbufferedMouseInput(evt) == false)
		{
			return false;
		}
	}

	if ( !mUseBufferedInputMouse || !mUseBufferedInputKeys)
	{
		// one of the input modes is immediate, so update the movement vector

		moveCamera();

	}

	return true;
}

//-----------------------------------------------------------------------------
bool BaseApplication::frameEnded(const FrameEvent& evt)
{
	if (mShutdownRequested)
		return false;

	return true;
}

//-------------------------------------------------------------------------------------
bool BaseApplication::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
    if(mWindow->isClosed())
        return false;

    if(mShutdownRequested)
        return false;

    //Need to capture/update each device
    mKeyboard->capture();
    mMouse->capture();

    mTrayMgr->frameRenderingQueued(evt);

    if (!mTrayMgr->isDialogVisible())
    {
        //mCameraMan->frameRenderingQueued(evt);   // if dialog isn't up, then update the camera
        if (mDetailsPanel->isVisible())   // if details panel is visible, then update its contents
        {
            mDetailsPanel->setParamValue(0, Ogre::StringConverter::toString(mPrimaryCamera->getDerivedPosition().x));
            mDetailsPanel->setParamValue(1, Ogre::StringConverter::toString(mPrimaryCamera->getDerivedPosition().y));
            mDetailsPanel->setParamValue(2, Ogre::StringConverter::toString(mPrimaryCamera->getDerivedPosition().z));
            mDetailsPanel->setParamValue(4, Ogre::StringConverter::toString(mPrimaryCamera->getDerivedOrientation().w));
            mDetailsPanel->setParamValue(5, Ogre::StringConverter::toString(mPrimaryCamera->getDerivedOrientation().x));
            mDetailsPanel->setParamValue(6, Ogre::StringConverter::toString(mPrimaryCamera->getDerivedOrientation().y));
            mDetailsPanel->setParamValue(7, Ogre::StringConverter::toString(mPrimaryCamera->getDerivedOrientation().z));
			mDetailsPanel->setParamValue(9, Ogre::StringConverter::toString(Degree(mPrimaryCamera->getFOVy())));
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
void BaseApplication::switchMouseMode()
{
	mUseBufferedInputMouse = !mUseBufferedInputMouse;
	mMouse->setBuffered(mUseBufferedInputMouse);
}

//-----------------------------------------------------------------------------
void BaseApplication::switchKeyMode()
{
	mUseBufferedInputKeys = !mUseBufferedInputKeys;
	mKeyboard->setBuffered(mUseBufferedInputKeys);
}

//-----------------------------------------------------------------------------
bool BaseApplication::keyPressed( const OIS::KeyEvent &arg )
{
	if( arg.key == OIS::KC_ESCAPE )
		mShutdownRequested = true;

	CEGUI::System::getSingleton().injectKeyDown( arg.key );
	CEGUI::System::getSingleton().injectChar( arg.text );

	return true;
}

//-----------------------------------------------------------------------------
bool BaseApplication::keyReleased( const OIS::KeyEvent &arg )
{
	if( arg.key == OIS::KC_M )
		mMouse->setBuffered( !mMouse->buffered() );
	else if( arg.key == OIS::KC_K )
		mKeyboard->setBuffered( !mKeyboard->buffered() );
	
	CEGUI::System::getSingleton().injectKeyUp( arg.key );

	return true;
}

//-----------------------------------------------------------------------------
bool BaseApplication::mouseMoved( const OIS::MouseEvent &arg )
{
	CEGUI::System::getSingleton().injectMouseMove( arg.state.X.rel, arg.state.Y.rel );

	return true;
}

//-----------------------------------------------------------------------------
bool BaseApplication::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
	CEGUI::System::getSingleton().injectMouseButtonDown(convertOISMouseButtonToCegui(id));

	return true;
}

//-----------------------------------------------------------------------------
bool BaseApplication::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
	CEGUI::System::getSingleton().injectMouseButtonUp(convertOISMouseButtonToCegui(id));

	return true;
}

//-----------------------------------------------------------------------------
//Adjust mouse clipping area
void BaseApplication::windowResized(RenderWindow* rw)
{
	unsigned int width, height, depth;
	int left, top;
	rw->getMetrics(width, height, depth, left, top);

	const OIS::MouseState &ms = mMouse->getMouseState();
	ms.width = width;
	ms.height = height;
}

//-----------------------------------------------------------------------------
//Unattach OIS before window shutdown (very important under Linux)
void BaseApplication::windowClosed(RenderWindow* rw)
{
	//Only close for window that created OIS (the main window in these demos)
	if( rw == mWindow )
	{
		if( mInputManager )
		{
			mInputManager->destroyInputObject( mMouse );
			mInputManager->destroyInputObject( mKeyboard );

			OIS::InputManager::destroyInputSystem(mInputManager);
			mInputManager = 0;
		}
	}
}

//-----------------------------------------------------------------------------
void BaseApplication::setupEventHandlers(void)
{
}

//-----------------------------------------------------------------------------
bool BaseApplication::handleQuit(const CEGUI::EventArgs& e)
{
	requestShutdown();
	return true;
}
