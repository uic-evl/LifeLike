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
* Filename: LLSceneManager.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#include <OgreExternalTextureSourceManager.h>

#include "LLSceneManager.h"
#include "LLCharacter.h"
#include "LLSoundManager.h"
#include "LLAniManager.h"
#include "LLMicMonitor.h"
#include "LLAudioMonitor.h"
#include "LifeLike.h"
#include "LLScreenLog.h"

// light animation
SceneNode* mainSpotLightNode = NULL;
float gLightRot = 240.0f;
bool bSpotLightAnimation = false;

//-----------------------------------------------------------------------------
LLSceneManager::LLSceneManager(SceneManager* mgr)
{
	m_pOGRESceneMgr = mgr;
	m_pHeadNode = m_pOGRESceneMgr->getRootSceneNode()->createChildSceneNode();

	m_pSoundManager = NULL;
	m_fAnimationSpeed = 1.0f;

	m_pCharacters = NULL;

	m_rTimeUntilNextToggle = 0;

	m_pAudioMonitor = NULL;
	m_pMicMonitor = NULL;

	m_CurrentPickedObject = NULL;
	m_RayScnQuery = m_pOGRESceneMgr->createRayQuery(Ogre::Ray());
	m_RayScnQuery->setSortByDistance(true);
	m_RayScnQuery->setQueryMask(ET_ITEM|ET_ENTRY);

	m_fLigntIniDir = 240.0f;
}

//-----------------------------------------------------------------------------
LLSceneManager::~LLSceneManager(void)
{
	delete m_pSoundManager;
	if (m_pMicMonitor)
		delete m_pMicMonitor;
	if (m_pAudioMonitor)
		delete m_pAudioMonitor;

	for (int i=0; i<m_iNumCharacter; i++)
		if (m_pCharacters[i])
			delete m_pCharacters[i];
	delete m_pCharacters;
}

//-----------------------------------------------------------------------------
void LLSceneManager::loadScene()
{
	// load scene configuration file
	ConfigFile cf;
	cf.load("LifeLike.cfg");

	String pcs, objs, blog, audiodevice, nsound, bgm, bgmaterial, useRecording, checkstr;
	int iObjs = 0;
	pcs = cf.getSetting("Character", "Scene", "1");
	objs = cf.getSetting("StaticObj", "Scene", "0");
	audiodevice = cf.getSetting("AudioMonitorDevice", "Scene", "Realtek HD Audio Input");
	bgm = cf.getSetting("BGM", "Scene", "");
	bgmaterial = cf.getSetting("BGMaterial", "Scene", "");
	useRecording = cf.getSetting("AudioRecording", "Scene", "0");
	m_iNumCharacter = atoi(pcs.c_str());
	iObjs = atoi(objs.c_str());


	char characterXML[][128] = { "..\\..\\media\\Lifelike\\mike.xml", "..\\..\\media\\Lifelike\\mary.xml"};

	char sections[128];
	for (int i=0; i<m_iNumCharacter; i++)
	{
		sprintf(sections, "Character%i", i);
		
		// simple check to see if it exists.
		checkstr = cf.getSetting("XMLFile", sections, "none");
		if (checkstr.compare("none")==0)
			continue;

		sprintf(characterXML[i], "%s", cf.getSetting("XMLFile", sections, "..\\..\\media\\Lifelike\\mike.xml").c_str());
#ifndef _WINDOWS
		correctSlash(characterXML[i]);		
#endif
	}

	char posx[16], posy[16], posz[16], scalex[16], scaley[16], scalez[16], pitch[16], yaw[16], roll[16];
	char objNames[10][128];
	char objFiles[10][128];
	float objPositions[10][3];
	float objScale[10][3];
	float objRotation[10][3];
	char objMaterial[10][128];
	
	for (int i=0; i<iObjs; i++)
	{
		sprintf(sections, "Object%i", i);
		
		// simple check to see if it exists.
		checkstr = cf.getSetting("Name", sections, "none");
		if (checkstr.compare("none")==0)
		{
			sprintf(objNames[i], "%s", "");
			continue;
		}

		sprintf(objNames[i],  "%s", cf.getSetting("Name", sections, "desk").c_str());
		sprintf(objFiles[i],  "%s", cf.getSetting("File", sections, "desk.mesh").c_str());
		sprintf(posx,  "%s", cf.getSetting("PosX", sections, "0").c_str());
		sprintf(posy,  "%s", cf.getSetting("PosY", sections, "0").c_str());
		sprintf(posz,  "%s", cf.getSetting("PosZ", sections, "0").c_str());
		sprintf(scalex,  "%s", cf.getSetting("ScaleX", sections, "1").c_str());
		sprintf(scaley,  "%s", cf.getSetting("ScaleY", sections, "1").c_str());
		sprintf(scalez,  "%s", cf.getSetting("ScaleZ", sections, "1").c_str());
		sprintf(pitch,  "%s", cf.getSetting("RotX", sections, "0").c_str());
		sprintf(yaw,  "%s", cf.getSetting("RotY", sections, "0").c_str());
		sprintf(yaw,  "%s", cf.getSetting("Rot", sections, yaw).c_str());		// for old style (only used single rotation anlog y-axis)
		sprintf(roll,  "%s", cf.getSetting("RotZ", sections, "0").c_str());
		sprintf(objMaterial[i],  "%s", cf.getSetting("Material", sections, "").c_str());
		
		objPositions[i][0] = atof(posx);
		objPositions[i][1] = atof(posy);
		objPositions[i][2] = atof(posz);
		objScale[i][0] = atof(scalex);
		objScale[i][1] = atof(scaley);
		objScale[i][2] = atof(scalez);
		objRotation[i][0] = atof(yaw);		// y-axis
		objRotation[i][1] = atof(pitch);	// x-axis
		objRotation[i][2] = atof(roll);		// z-axis
	}
	
	nsound = cf.getSetting("NumSound", "Sound", "0");
	int isound = atoi(nsound.c_str());

	// vsm shadow technique
	::Ogre::MaterialManager::getSingleton().setDefaultTextureFiltering(::Ogre::TFO_ANISOTROPIC);
	m_pOGRESceneMgr->setShadowTextureSelfShadow(true);
	m_pOGRESceneMgr->setShadowTextureCasterMaterial("shadow_caster");
	m_pOGRESceneMgr->setShadowTextureCount(1);
	m_pOGRESceneMgr->setShadowTextureSize(4096);
	m_pOGRESceneMgr->setShadowTexturePixelFormat(PF_FLOAT16_RGB);
	m_pOGRESceneMgr->setShadowCasterRenderBackFaces(false);
	const unsigned numShadowRTTs = m_pOGRESceneMgr->getShadowTextureCount();
	for (unsigned i = 0; i < numShadowRTTs; ++i) {
		Ogre::TexturePtr tex = m_pOGRESceneMgr->getShadowTexture(i);
		Ogre::Viewport *vp = tex->getBuffer()->getRenderTarget()->getViewport(0);
		vp->setBackgroundColour(Ogre::ColourValue(1, 1, 1, 1));
		vp->setClearEveryFrame(true);
	}
	m_pOGRESceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_ADDITIVE_INTEGRATED);

	// load static environment mesh
	LLEntity* tempEnt; 
	Quaternion qt;
	Matrix3 mat;
	for (int i=0; i< iObjs; i++)
	{
		if (strlen(objNames[i])==0)
			continue;
		
		tempEnt = new LLEntity(m_pHeadNode);
		tempEnt->createEntity(m_pOGRESceneMgr, objNames[i], objFiles[i], objMaterial[i]);
		
		mat.FromEulerAnglesXYZ(Degree(objRotation[i][1]), Degree(objRotation[i][0]), Degree(objRotation[i][2]));
		qt.FromRotationMatrix(mat);
		tempEnt->setOrientation(qt);
		Vector3 vv(objPositions[i][0], objPositions[i][1], objPositions[i][2]);
		tempEnt->setPosition(vv);		
		vv = Vector3(objScale[i][0],objScale[i][1],objScale[i][2]);		
		tempEnt->setScale(vv);
		tempEnt->getEntity()->setCastShadows(false);
	}

	// Sound Manager
	m_pSoundManager = new LLSoundManager();
	char soundindex[64], sfilename[128];
	String soundfile;	
	for (int i=0; i<isound; i++)
	{
		sprintf(soundindex, "Sound%i", i);
		soundfile = cf.getSetting("soundindex", "Sound", "..\\..\\media\\Lifelike\\sound\\sound.wav");
		sprintf(sfilename, "%s", soundfile.c_str());
#ifndef _WINDOWS
		correctSlash(characterXML[i]);		
#endif
		
		m_pSoundManager->loadSoundSource(sfilename);
	}
	if (strlen(bgm.c_str()) !=0)
	{
		m_pSoundManager->loadSoundSource((char*)bgm.c_str(), true);
		m_pSoundManager->playBGSound(0, true);
	}

#ifdef _WINDOWS
	// sound monitor: stereo mixer
	m_pAudioMonitor = new LLAudioMonitor((char*)audiodevice.c_str());

	// mic monitor: default recording device (microphone)
	m_pMicMonitor = new LLMicMonitor;
	if (atoi(useRecording.c_str()) == 1)
		m_pMicMonitor->enableRecording();
#endif

	// Player Character
	m_pCharacters = new LLCharacter*[m_iNumCharacter];
	for (int i=0; i<m_iNumCharacter; i++)
	{
		// send out xml character file
		createCharacter(characterXML[i], i);
	}

	// Create a light
	// ToDo: use conf to add lights (good for per app light setup)
	createLights();
	
	// temp backgroud image
	if (strlen(bgmaterial.c_str()) != 0)
	{
		Rectangle2D* mRect = new Rectangle2D(true); 
		mRect->setCorners(-1.0, 1.0, 1.0, -1.0); 
		AxisAlignedBox aabInf;
		aabInf.setInfinite();
		mRect->setBoundingBox(aabInf);
		mRect->setMaterial(bgmaterial); 
		mRect->setRenderQueueGroup(RENDER_QUEUE_BACKGROUND);
		SceneNode* node = m_pOGRESceneMgr->getRootSceneNode()->createChildSceneNode("BackgroundImage"); 
		node->attachObject(mRect);
		mRect->setCastShadows(false);
	}

#ifndef _DEBUG
	// hair
	try{
		Ogre::MaterialPtr mptr = (Ogre::MaterialPtr)Ogre::MaterialManager::getSingleton().getByName("hair_ani/hair_mat");
		m_ptrHairParams = mptr->getTechnique("simTechnique")->getPass("simPass")->getVertexProgramParameters();
	}
	catch (Ogre::Exception e)
	{
		// do nothing for now
		if (!m_ptrHairParams.isNull())
			m_ptrHairParams.setNull();
	}
#endif

}

//-----------------------------------------------------------------------------
void LLSceneManager::update(Real addedTime)
{
	// update input toggle value
	if (m_rTimeUntilNextToggle >= 0)
		m_rTimeUntilNextToggle -= addedTime;

	// update character
	for (int i=0; i<m_iNumCharacter; i++)
	{
		m_pCharacters[i]->update(addedTime);
	}

	m_pSoundManager->update(addedTime);


#ifdef _WINDOWS
	// microphone monitor
	if (m_pMicMonitor)
	{
		m_pMicMonitor->update(addedTime);
		float levelvalue = m_pMicMonitor->getMicLevel(0);
			
		g_pLLApp->setMicMeter(levelvalue*2.0f);
	}
	if (m_pAudioMonitor)
		m_pAudioMonitor->update(addedTime);

#endif

	// move light
	if (mainSpotLightNode && bSpotLightAnimation)
	{
		gLightRot += addedTime * 20.0f;
		Quaternion qt;
		qt.FromAngleAxis(Degree(gLightRot), Vector3::UNIT_Y);
		mainSpotLightNode->setOrientation(qt);
	}

	// custom animation
	if (!m_EnabledAnimationStates.empty())
	{
		// tick all enabled animation state
	    EnabledAnimationStateList::iterator it, itend;
		itend = m_EnabledAnimationStates.end();
		AnimationState* astate;
		for (it = m_EnabledAnimationStates.begin(); it != itend;)
		{
			astate = *it;
			
			// end of animation?
			if (astate->hasEnded())
			{
				astate->setEnabled(false);
				astate->setTimePosition(0);
				it = m_EnabledAnimationStates.erase(it++);
			}
			else
			{
				astate->addTime(addedTime);
				it++;
			}
		}	
	}
}

//-----------------------------------------------------------------------------
void LLSceneManager::rotate(const Quaternion& q)
{
	// rotate top level of node: accumlate
	m_pHeadNode->rotate(q);
}

//-----------------------------------------------------------------------------
void LLSceneManager::speak(int id, const String& speech, tCHARACTER_SPEECH_TYPE type)
{
#ifdef _WINDOWS

	if (id > m_iNumCharacter-1)
		return;

	if (m_pCharacters[id] == NULL)
		return;

	if (m_pCharacters[id]->isSpeaking())
		return;

	char str[4096];
	strcpy(str, speech.c_str());
	m_pCharacters[id]->speak(speech, type);

#endif
}

//-----------------------------------------------------------------------------
void LLSceneManager::stopSpeak(int id)
{
#ifdef _WINDOWS
	// multiple characters
	if (id >= m_iNumCharacter)
		return;
	if (m_pCharacters[id] == NULL)
		return;
	m_pCharacters[id]->stopSpeak();

#endif
}

//-----------------------------------------------------------------------------
void LLSceneManager::createCharacter(char* filename, int id)
{
	// create character
	m_pCharacters[id] = new LLCharacter(m_pHeadNode);
	m_pCharacters[id]->setID(id);

#ifdef _WINDOWS
	// set mic monitor handle
	m_pCharacters[id]->setMicMonitor(m_pMicMonitor);
	m_pCharacters[id]->setSoundMonitor(m_pAudioMonitor);
#endif

	// load up spec file
	m_pCharacters[id]->loadFromSpecFile(m_pOGRESceneMgr, filename);

	m_pCharacters[id]->setAction(CAT_IDLE);

	// set sound event handler
	m_pCharacters[id]->i_SoundEventCallback.setCallback(m_pSoundManager, &LLSoundManager::eventHandler);

}

//-----------------------------------------------------------------------------
void triple2float(const char* str, float* val)
{
	char sVal[256];
	strcpy(sVal, str);
	char *p_token;
	char seps[] = ",";
	p_token = strtok(sVal, seps);
	val[0] = atof(p_token);
	p_token = strtok(NULL, seps);
	val[1] = atof(p_token);
	p_token = strtok(NULL, seps);
	val[2] = atof(p_token);
}

//-----------------------------------------------------------------------------
void LLSceneManager::createLights()
{
	// lights depends on scene (application setup), so should make this more flexible
	// add all light parameters in configuratin file
	// number of light, light type, position, direction, color, misc
	// light attribute depends on light type
	ConfigFile cf;
	cf.load("LifeLike.cfg");

	String tripleStr, tStr;
	float fTriple[3];
	int iLights = 0;
	iLights = atoi(cf.getSetting("numLight", "Scene", "0").c_str());

	// 0. ambient light color
	tripleStr = cf.getSetting("ambientLight", "Scene", "0.2,0.2,0.2");
	triple2float(tripleStr.c_str(), fTriple);
	m_pOGRESceneMgr->setAmbientLight(ColourValue(fTriple[0], fTriple[1], fTriple[2]));

	// 1. direction for light node (spot light)
	m_fLigntIniDir = atof(cf.getSetting("iniLightAngle", "Scene", "240").c_str());

	// 2. all other type of light
	// loop
	char items[128], tchar[128];
	Light* plight;
	for (int i=0; i<iLights; i++)
	{
		// session name
		sprintf(items, "Light%i", i);
		
		// simple check to see if it exists.
		tStr = cf.getSetting("type", items, "none");
		if (tStr.compare("none")==0)
			continue;

		// light type: spot (SPOT), directional (DIR), point (POINT)
		tStr = cf.getSetting("type", items, "SPOT");
		if (strcmp(tStr.c_str(), "DIR") == 0)
		{
			sprintf(tchar, "dirLight%d", i);
			plight = m_pOGRESceneMgr->createLight(tchar);
			plight->setType(Light::LT_DIRECTIONAL);
			
			// direction
			tStr = cf.getSetting("direction", items, "1.0,1.0,1.0");
			triple2float(tStr.c_str(), fTriple);
			plight->setDirection(fTriple[0], fTriple[1], fTriple[2]);
			
			// color
			tStr = cf.getSetting("color", items, "1.0,1.0,1.0");
			triple2float(tStr.c_str(), fTriple);
			plight->setDiffuseColour(fTriple[0], fTriple[1], fTriple[2]);

			plight->setCastShadows(false);
			
			sprintf(tchar, "dirLightNode%d", i);
			SceneNode* lnode = m_pHeadNode->createChildSceneNode(tchar);
			lnode->attachObject(plight);
		}
		else if (strcmp(tStr.c_str(), "SPOT") == 0)
		{
			sprintf(tchar, "spotLight%d", i);

			plight = m_pOGRESceneMgr->createLight(tchar);
			
			// color
			tStr = cf.getSetting("color", items, "1.0,1.0,1.0");
			triple2float(tStr.c_str(), fTriple);
			plight->setDiffuseColour(fTriple[0], fTriple[1], fTriple[2]);
			plight->setType(Ogre::Light::LT_SPOTLIGHT);
			
			// inner angle
			plight->setSpotlightInnerAngle(Ogre::Degree(atof(cf.getSetting("innerAngle", items, "25").c_str())));
			
			// outer angle
			plight->setSpotlightOuterAngle(Ogre::Degree(atof(cf.getSetting("outerAngle", items, "28").c_str())));
			
			// attenuation
			plight->setAttenuation(atof(cf.getSetting("range", items, "400").c_str()), 1, 1, 1);
			
			// posiiton
			tStr = cf.getSetting("position", items, "1.0,1.0,1.0");
			triple2float(tStr.c_str(), fTriple);
			plight->setPosition(fTriple[0], fTriple[1], fTriple[2]);

			// direction
			tStr = cf.getSetting("direction", items, "1.0,1.0,1.0");
			triple2float(tStr.c_str(), fTriple);
			plight->setDirection(fTriple[0], fTriple[1], fTriple[2]);

			if (mainSpotLightNode == NULL)
				mainSpotLightNode = m_pHeadNode->createChildSceneNode("spotLightNode1");
			
			mainSpotLightNode->attachObject(plight);

		}
		else if (strcmp(tStr.c_str(), "POINT") == 0)
		{
			sprintf(tchar, "pointLight%d", i);
			plight = m_pOGRESceneMgr->createLight(tchar);
			plight->setType(Light::LT_POINT);

			// position
			tStr = cf.getSetting("position", items, "1.0,1.0,1.0");
			triple2float(tStr.c_str(), fTriple);
			plight->setPosition(fTriple[0], fTriple[1], fTriple[2]);

			// color
			tStr = cf.getSetting("color", items, "1.0,1.0,1.0");
			triple2float(tStr.c_str(), fTriple);
			plight->setDiffuseColour(fTriple[0], fTriple[1], fTriple[2]);
			plight->setCastShadows(false);
			
			sprintf(tchar, "pointLightNode%d", i);
			SceneNode* lnode = m_pHeadNode->createChildSceneNode(tchar);
			lnode->attachObject(plight);
		}
	}

	// tweak to change initial direction
	Quaternion qt;
	qt.FromAngleAxis(Degree(m_fLigntIniDir), Vector3::UNIT_Y);
	if (mainSpotLightNode)
		mainSpotLightNode->setOrientation(qt);

	gLightRot = m_fLigntIniDir;
}

//-----------------------------------------------------------------------------
void LLSceneManager::setUserPosition(float x, float y, float z)
{
	for (int i=0; i<m_iNumCharacter; i++)
		m_pCharacters[i]->setUserPosition(x, y, z);
}

//-----------------------------------------------------------------------------
void LLSceneManager::setAttractionPosition(float x, float y, float z)
{
	for (int i=0; i<m_iNumCharacter; i++)
		m_pCharacters[i]->setAttractionPosition(x, y, z);
}

//-----------------------------------------------------------------------------
bool LLSceneManager::isSpeaking(int id)
{
#ifdef _WINDOWS

	if (id >= m_iNumCharacter)
		return false;
	else
		return m_pCharacters[id]->isSpeaking();
#endif
}

//-----------------------------------------------------------------------------
void LLSceneManager::setAnimationSpeed(float speed, bool relative)
{
	for (int i=0; i < m_iNumCharacter; i++)
		m_pCharacters[i]->setAnimationSpeed(speed, relative);
}

//-----------------------------------------------------------------------------
void LLSceneManager::processKeyInput(OIS::Keyboard* keyboard)
{
	if(m_pCharacters == NULL || m_pCharacters[0] == NULL)
		return;

	// key event pass to characters
	// in fact, characters should not directly listen for key inputs in general
	// restored for python keyboard event handling addition
	for (int i=0; i < m_iNumCharacter; i++)
		m_pCharacters[i]->processKeyInput(keyboard);

	// Spot Light Animation: toggle
	if (keyboard->isKeyDown(OIS::KC_NUMPAD0) && m_rTimeUntilNextToggle <= 0)
	{
		if (bSpotLightAnimation)
			bSpotLightAnimation = false;
		else
			bSpotLightAnimation = true;

		m_rTimeUntilNextToggle = 0.5;
	}
	// Spot Light Animation: reset
	if (keyboard->isKeyDown(OIS::KC_NUMPAD1) && m_rTimeUntilNextToggle <= 0)
	{
		gLightRot = m_fLigntIniDir;
		Quaternion qt;
		qt.FromAngleAxis(Degree(gLightRot), Vector3::UNIT_Y);
		mainSpotLightNode->setOrientation(qt);

		m_rTimeUntilNextToggle = 0.5;
	}

	// character position reset
	if (keyboard->isKeyDown(OIS::KC_END) && m_rTimeUntilNextToggle <= 0)
	{
		m_pCharacters[0]->moveTo(Vector3::ZERO);
		m_rTimeUntilNextToggle = 0.5;
	}

#ifdef _WINDOWS

	if (keyboard->isKeyDown(OIS::KC_GRAVE) && m_rTimeUntilNextToggle <= 0)
	{
		m_pCharacters[0]->speak("angry.");
		m_rTimeUntilNextToggle = 0.5;
	}
	if (keyboard->isKeyDown(OIS::KC_1) && m_rTimeUntilNextToggle <= 0)
	{
		m_pCharacters[0]->speak("contempt.");
		m_rTimeUntilNextToggle = 0.5;
	}
	if (keyboard->isKeyDown(OIS::KC_2) && m_rTimeUntilNextToggle <= 0)
	{
		m_pCharacters[0]->speak("disgust.");
		m_rTimeUntilNextToggle = 0.5;
	}
	if (keyboard->isKeyDown(OIS::KC_3) && m_rTimeUntilNextToggle <= 0)
	{
		m_pCharacters[0]->speak("fear.");
		m_rTimeUntilNextToggle = 0.5;
	}
	if (keyboard->isKeyDown(OIS::KC_4) && m_rTimeUntilNextToggle <= 0)
	{
		m_pCharacters[0]->speak("happy.");
		m_rTimeUntilNextToggle = 0.5;
	}
	if (keyboard->isKeyDown(OIS::KC_5) && m_rTimeUntilNextToggle <= 0)
	{
		m_pCharacters[0]->speak("sad.");
		m_rTimeUntilNextToggle = 0.5;
	}
	if (keyboard->isKeyDown(OIS::KC_6) && m_rTimeUntilNextToggle <= 0)
	{
		m_pCharacters[0]->speak("surprise.");
		m_rTimeUntilNextToggle = 0.5;
	}
	if (keyboard->isKeyDown(OIS::KC_7) && m_rTimeUntilNextToggle <= 0)
	{
		m_pCharacters[0]->speak("<emo id=\"SAD\" value=\"0.5\"/>I </>am <emo id=\"HAPPY\" value=\"0.5\"/>sad.");
		m_rTimeUntilNextToggle = 0.5;
	}
	if (keyboard->isKeyDown(OIS::KC_8) && m_rTimeUntilNextToggle <= 0)
	{
		m_pCharacters[0]->speak("I am very glad to hear that you made it.");
		m_rTimeUntilNextToggle = 0.5;
	}
	if (keyboard->isKeyDown(OIS::KC_9) && m_rTimeUntilNextToggle <= 0)
	{
		m_pCharacters[0]->addFacialExpression(E_ANGER, 2.0f, 1.0f, 1.0f, 0.2f);
		m_rTimeUntilNextToggle = 0.5;
	}
	if (keyboard->isKeyDown(OIS::KC_0) && m_rTimeUntilNextToggle <= 0)
	{
		m_pCharacters[0]->addFacialExpression(E_SMILE, 2.0f, 1.0f, 1.0f, 0.2f);
		m_rTimeUntilNextToggle = 0.5;
	}
	if (keyboard->isKeyDown(OIS::KC_EQUALS) && m_rTimeUntilNextToggle <= 0)
	{
		static bool sUseAffect = true;
		sUseAffect = !sUseAffect;
		m_pCharacters[0]->setUseAffectBehavior(sUseAffect);
		m_rTimeUntilNextToggle = 0.5;

		if (sUseAffect)
			LLScreenLog::getSingleton().addText("** Character Affect Model is Active");
		else
			LLScreenLog::getSingleton().addText("** Character Affect Model is Inactive");

	}
	if (keyboard->isKeyDown(OIS::KC_MINUS) && m_rTimeUntilNextToggle <= 0)
	{
		m_pCharacters[0]->resetMood();
		m_rTimeUntilNextToggle = 0.5;
	}

#endif
}

//-----------------------------------------------------------------------------
void LLSceneManager::processMouseInput( const OIS::MouseEvent &arg, int btID, int pressed )
{
	if (!pressed)
		return;

#ifndef _WINDOWS
	return;
#endif

	// this function called by LifeLike app when mouse click event occurs
	// Set up the ray scene query, use CEGUI's mouse position
	CEGUI::Point mousePos = CEGUI::MouseCursor::getSingleton().getPosition();
	Ray mouseRay = m_pOGRESceneMgr->getCamera("PlayerPrimaryCam")->getCameraToViewportRay(mousePos.d_x/float(arg.state.width), mousePos.d_y/float(arg.state.height));
	m_RayScnQuery->setRay(mouseRay);
 
	// Execute query
	RaySceneQueryResult &result = m_RayScnQuery->execute();
	RaySceneQueryResult::iterator itr = result.begin();

	Ogre::SceneNode *pickedObject = NULL;
	for ( itr; itr != result.end(); itr++ )
	{
		// only item type entity will be selected!!!
		if (itr->movable)
		{
			pickedObject = itr->movable->getParentSceneNode();
			break;
		}
	}


	if (pickedObject)
	{
		// show boundary box
		if (m_CurrentPickedObject)
		{
			if (pickedObject != m_CurrentPickedObject)
				m_CurrentPickedObject->showBoundingBox(false);
		}

		m_CurrentPickedObject = pickedObject;

		try
		{
			// video material texture control
			Entity* ent = m_pOGRESceneMgr->getEntity(m_CurrentPickedObject->getName());
			if (ent)
			{
				Ogre::String mname = ent->getSubEntity(0)->getMaterialName();
				if (mname.find_first_of("Video") != -1)
				{
					ExternalTextureSource* extex = ExternalTextureSourceManager::getSingleton().getExternalTextureSource("ogg_video");
					if (extex)
					{
						if (btID == OIS::MB_Left)
							extex->setParameter("play_mode", "toggle\t"+mname);
						if (btID == OIS::MB_Right)
							extex->setParameter("play_mode", "restart\t"+mname);
					}
				}

				// character move test
				if (ent->getQueryFlags() == ET_ITEM)
				{
					Plane floor(m_pHeadNode->getOrientation().yAxis(),0);
					std::pair<bool, Real> pickresult = mouseRay.intersects(floor);
					if (pickresult.first)
					{
						Vector3 pickPoint = mouseRay.getPoint(pickresult.second);
						pickPoint = m_pHeadNode->getOrientation().Inverse() * pickPoint;
						if (btID == OIS::MB_Left && m_pCharacters[0])
						{
							m_pCharacters[0]->moveTo(pickPoint, 0);
						}
						else if (btID == OIS::MB_Right && m_iNumCharacter>1 && m_pCharacters[1])
						{
							m_pCharacters[1]->moveTo(pickPoint, 0);
						}
					}
				}
			}
		}
		catch (const Ogre::Exception* e)
		{
			std::cout << e->getFullDescription() << std::endl;
		}
	}
	else if (m_CurrentPickedObject)
	{
		m_CurrentPickedObject->showBoundingBox(false);
		m_CurrentPickedObject = NULL;
	}

}

//-----------------------------------------------------------------------------
static float rrx(0), rry(0);
void LLSceneManager::mouseMoved(int relX, int relY)
{
	// called when mouse left button is down and mouse is moving
	// in case there is picked object, let's move it!
	if (m_CurrentPickedObject)
	{
		try
		{
			Entity* ent = m_pOGRESceneMgr->getEntity(m_CurrentPickedObject->getName());

		}
		catch (const Ogre::Exception* e)
		{
			std::cout << e->getFullDescription() << std::endl;
		}

		// move this picked node by how much?
		// just try x, y move by relative value for now
		// damping
		float x = rrx + (relX*0.05 - rrx) * 0.5f;
		float y = rry + (relY*0.05 - rry) * 0.5f;
		m_CurrentPickedObject->translate(x*2.0, -y*2.0, 0);
		if (m_CurrentPickedObject->getName().compare("hair_test")==0)
		{
			// set shader parameter to test hair animation
			if (!m_ptrHairParams.isNull())
			{
				m_ptrHairParams->setNamedConstant("momentum", Vector3(x, -y, 0));
			}
		}
		rrx = x; rry = y;
	}
}

//-----------------------------------------------------------------------------
void LLSceneManager::setSoundAnimation(int id, float start, float end, float duration)
{
	m_pSoundManager->setSoundAnimation(id, start, end, duration);
}

//-----------------------------------------------------------------------------
float LLSceneManager::getMicLevel()
{
	return m_pMicMonitor->getMicLevel();

}

//-----------------------------------------------------------------------------
LLCharacter* LLSceneManager::getCharacter(int id)
{
	if (id >= m_iNumCharacter)
		return NULL;

	return m_pCharacters[id];
}

//-----------------------------------------------------------------------------
void	LLSceneManager::createAnimationFromFile(const String& nodeName,const String& trackName,const String& filename)
{
	// need to implement .anim import module here
	// assume there is only one node & animation curve is baked in maya
	// translateX,Y,Z, rotateX,Y,Z, scaleX,Y,Z
	// read line by line and parse it => build serial keyframe data

	// open file
	FILE *fp;
	if (!(fp=fopen(filename.c_str(),"r"))) 
	{
		printf("Can't open animation file file: %s\n", filename);
		return;
	}

	// pre-scan : determine number of keys
	int numKey = 0;
	char line[256];
	char* p_token, *pch;
	char seps[] =" \t";

	fgets(line, sizeof(line), fp);

	while (strncmp(line, "{", 1)!=0)
		fgets(line, sizeof(line), fp);

	fgets(line, sizeof(line), fp);	// section value header
	fgets(line, sizeof(line), fp);

	while (strncmp(line, "}", 1)!=0)
	{
		numKey++;
		fgets(line, sizeof(line), fp);
	}
	
	// rewind
	rewind(fp);

	if (numKey < 1)
		return;

	// initialize temp storage for key, trans, rot, scale
	float *fvalues = new float[numKey*10];

	// read values
	// determine which value is this first, then loop numKey times
	int offset = 0;
	char* pos;
	while( fgets(line, sizeof(line), fp) )
	{
		// which section is this?
		pos = strrchr(line, '_');
		if (pos != NULL)
		{
			// find offset to determine which section we are in
			// once detect, read two more lines ({ and header)
			if (strncmp(pos+1, "translateX", 10)==0)
				offset = 1, fgets(line, sizeof(line), fp), fgets(line, sizeof(line), fp);
			else if (strncmp(pos+1, "translateY", 10)==0)
				offset = 2, fgets(line, sizeof(line), fp), fgets(line, sizeof(line), fp);
			else if (strncmp(pos+1, "translateZ", 10)==0)
				offset = 3, fgets(line, sizeof(line), fp), fgets(line, sizeof(line), fp);
			else if (strncmp(pos+1, "rotateX", 7)==0)
				offset = 4, fgets(line, sizeof(line), fp), fgets(line, sizeof(line), fp);
			else if (strncmp(pos+1, "rotateY", 7)==0)
				offset = 5, fgets(line, sizeof(line), fp), fgets(line, sizeof(line), fp);
			else if (strncmp(pos+1, "rotateZ", 7)==0)
				offset = 6, fgets(line, sizeof(line), fp), fgets(line, sizeof(line), fp);
			else
				offset = 0;
		}

		// well in fact, we can read all numKey entries in this section
		// this assume all key values were baked in maya (every key frame has data)
		if (offset)
		{
			// this is real value
			for (int i=0; i<numKey; i++)
			{
				// read values
				fscanf(fp, "%f", fvalues+(i*10));	// key time
				fscanf(fp, "%f", fvalues+(i*10+offset));
			}
			offset = 0;
		}
	}
	
	// now build animation track and state
	float anitime = fvalues[(numKey-1)*10];
	Node *pNode = m_pOGRESceneMgr->getRootSceneNode()->getChild(nodeName);
	AnimationState* aniState = NULL;
	if (pNode)
	{
		if (m_pOGRESceneMgr->hasAnimation(trackName))
		{
			m_pOGRESceneMgr->destroyAnimationState(trackName);
			m_pOGRESceneMgr->destroyAnimation(trackName);
		}

		// create animation and track
		Animation* anim = m_pOGRESceneMgr->createAnimation(trackName, anitime);
		NodeAnimationTrack* track = anim->createNodeTrack(0, pNode);

		// create keyframes for our track
		TransformKeyFrame* key;
		Quaternion qt;
		Matrix3 mat;
		int idx = 0;

		for (int i=0; i<numKey; i++)
		{
			// base index for key value
			idx = i*10;

			// key
			key = track->createNodeKeyFrame(fvalues[idx]);
			
			// translate
			key->setTranslate(Vector3(fvalues[idx+1],fvalues[idx+2],fvalues[idx+3]));
			
			// orientation: computer euler angles to quaternion
			mat.FromEulerAnglesXYZ(Radian(fvalues[idx+4]), Radian(fvalues[idx+5]), Radian(fvalues[idx+6]));
			qt.FromRotationMatrix(mat);
			key->setRotation(qt);
		}

		// create animation state and add it to list
		// need some mechanism to keep all these user defined animations with scene manager
		aniState = m_pOGRESceneMgr->createAnimationState(trackName);
		aniState->setLoop(false);
		m_AnimationStates[trackName] = aniState;
	}

	// clean up temp storage
	delete fvalues;
	fclose(fp);

	return;
}

//-----------------------------------------------------------------------------
void LLSceneManager::enableNodeAnimation(const String& trackName)
{
	// find track
	AnimationStateMap::iterator i = m_AnimationStates.find(trackName);
	if (i != m_AnimationStates.end())
	{
		if (!i->second->getEnabled())
		{
			m_EnabledAnimationStates.push_back(i->second);
			i->second->setEnabled(true);
		}
	}
}