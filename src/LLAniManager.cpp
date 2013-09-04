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
* Filename: LLAniManager.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#include "LLAniManager.h"
#include "LLCharacter.h"
#include "LLFSM.h"
#include "LLAnimation.h"
#include "LLAnimationBlender.h"
#include "tinyxml2.h"
#include "LLdefine.h"
#include "LLSoundManager.h"
#include "LLScreenLog.h"

using namespace tinyxml2;

//-----------------------------------------------------------------------------
LLAniManager::LLAniManager(Entity* ent, LLCharacter* character)
{
	m_pEntity = ent;
	m_pOwner = character;
	m_pFSM = NULL;
	m_iCurrAction = -1;
	
	m_pOldAnim = NULL;
	m_pCurrAnim = NULL;

	m_fBlendTime = 0;
	m_pAniBlender = new LLAnimationBlender(ent);
	m_fBlendMask = 1.0f;
	m_fBlendMaskDelta = 0.0f;
}

//-----------------------------------------------------------------------------
LLAniManager::~LLAniManager(void)
{
	if(m_pFSM)
		delete m_pFSM;

	int iani;
	iani = (int)m_vAnimations.size();
	for (int i=0; i<iani; i++)
	{
		LLAnimation* ani = m_vAnimations[i];
		delete ani;
		m_vAnimations[i] = NULL;
	}
	m_vAnimations.clear();
	
	delete m_pAniBlender;

}

//-----------------------------------------------------------------------------
void LLAniManager::loadFSM(const char* filename)
{
	if(m_pFSM)
		delete m_pFSM;

	m_pFSM = new LLFSM(filename);

}

//-----------------------------------------------------------------------------
void LLAniManager::loadAnimations(const char* filename)
{
	const char *fsm;
	const char *headmanual, *headbone;
	char headbonename[256];
	bool bManualHead = false;

	// init xml parser
	tinyxml2::XMLDocument	xmlDoc;
	
	// load xml file
	xmlDoc.LoadFile(filename);
	XMLElement* element = xmlDoc.RootElement();
	
	// load fsm spec file
	fsm = element->Attribute("FSM");
#ifndef _WINDOWS
		correctSlash(fsm);
#endif
	loadFSM(fsm);

	// head manual control flag
	headmanual = element->Attribute("HeadControl");
	if (strlen(headmanual) > 0 && strcmp(headmanual, "Manual") == 0)
		bManualHead = true;

	// head bone name
	headbone = element->Attribute("HeadBone");
	if (headbone)
		strcpy(headbonename, headbone);
	else
		strcpy(headbonename, "Head_1");

	// need to load skeleton if exists
	XMLElement* element1 = element->FirstChildElement("Skeleton");
	const char* skeletonfile, *masterfile;
	while (element1)
	{
		// now do the job here
		skeletonfile = element1->Attribute("file");
		masterfile = element1->Attribute("master");
		addSkeletons(skeletonfile, masterfile);

		element1 = element1->NextSiblingElement("Skeleton");
	}

	// load each animation spec
	element1 = element->FirstChildElement("Anim");
	const char* aniname; float blend, evtime, speed; 
	const char* evtype, *param;
	int loop;
	LLAnimation* anim;
	while (element1)
	{
		// animation
		aniname = element1->Attribute("name");
		blend	= element1->FloatAttribute("blend");
		loop = element1->IntAttribute("loop");
		speed = element1->FloatAttribute("movespeed");

		anim = new LLAnimation(this, m_pEntity, aniname);
		anim->setBlendTime(blend);
		anim->setMovingSpeed(speed);

		// assume that the first one is looping idle animation
		if (loop==1)		
			anim->setLoop(true);
		
		// event
		XMLElement* element2 = element1->FirstChildElement("AEvent");
		while (element2)
		{
			evtime = element2->FloatAttribute("time");
			evtype = element2->Attribute("type");
			param = element2->Attribute("param");
			
			// event type
			tEVENT etype = EVT_NONE;
			if (strcmp(evtype, "EVT_SOUND")==0)
				etype = EVT_SOUND;
			else if (strcmp(evtype, "EVT_SPEAK")==0)
				etype = EVT_SPEAK;
			else if (strcmp(evtype, "EVT_FFTSPEAK")==0)
				etype = EVT_FFTSPEAK;
			else if (strcmp(evtype, "EVT_ACTION")==0)
				etype = EVT_ACTION;
			else if (strcmp(evtype, "EVT_SHAPE")==0)
				etype = EVT_SHAPE;
			else if (strcmp(evtype, "EVT_VISIBILITY")==0)
			{
				etype = EVT_VISIBILITY;
				//LLScreenLog::getSingleton().addText("Animation visibility event adding: " + Ogre::String(param));
			}

			anim->addEvent(evtime, etype, param);		// need to modify: use hashmap to find event id

			element2 = element2->NextSiblingElement("AEvent");
		}
		
		// character event handler
		anim->i_CharacterEventCallback.setCallback(m_pOwner, &LLCharacter::eventHandler);

		addAnimation(anim);

		element1 = element1->NextSiblingElement("Anim");
	}

	if (bManualHead)
		destroyNodeTrack(headbonename);

	//
	m_uHeadBoneHandle = m_pEntity->getSkeleton()->getBone(headbonename)->getHandle();
}

//-----------------------------------------------------------------------------
void LLAniManager::addSkeletons(const char* filename, const char* masterfile)
{
	// merge skeleton animaton from separate file
	SkeletonPtr pSkeletonMaster = SkeletonManager::getSingleton().getByName( masterfile ); 
	SkeletonPtr pSkeletonNew = SkeletonManager::getSingleton().load( filename, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME ); 

	Skeleton::BoneHandleMap boneHandleMap; 
	pSkeletonNew->_buildMapBoneByHandle( pSkeletonNew.getPointer(), boneHandleMap ); 
	pSkeletonMaster->_mergeSkeletonAnimations( pSkeletonNew.getPointer(), boneHandleMap ); 

	SkeletonManager::getSingleton().remove( filename ); 

	m_pEntity->getSkeleton()->_refreshAnimationState( m_pEntity->getAllAnimationStates() ); 
}

//-----------------------------------------------------------------------------
void LLAniManager::destroyNodeTrack(const char* bonename)
{
	int anims = m_vAnimations.size();
	for(int i=0; i<anims; i++)
		m_vAnimations[i]->destroyNodeTrack(bonename);
}

//-----------------------------------------------------------------------------
void LLAniManager::addAnimation(LLAnimation* anim)
{
	m_vAnimations.push_back(anim);
}

//-----------------------------------------------------------------------------
void LLAniManager::update(Real addedTime)
{
	if (m_pOldAnim)
	{
		m_pOldAnim->setBlendMask(m_uHeadBoneHandle, m_fBlendMask);
		m_pOldAnim->update(addedTime);
	}
	if (m_pCurrAnim)
	{
		m_pCurrAnim->setBlendMask(m_uHeadBoneHandle, m_fBlendMask);
		m_pCurrAnim->update(addedTime);
	}

}

//-----------------------------------------------------------------------------
void LLAniManager::animationEnded(LLAnimation* anim)
{
	//if (m_pOldAnim)
	if (m_pOldAnim == anim)
	{
		m_pOldAnim->setBlendMask(m_uHeadBoneHandle, 1.0f);
		m_pOldAnim = NULL;
		//LLScreenLog::getSingleton().addText("AniManager null oldAni");
	}
	else
	{
		m_pCurrAnim->setBlendMask(m_uHeadBoneHandle, 1.0f);
		m_pCurrAnim = NULL;
		setAction(m_iCurrAction);
		//LLScreenLog::getSingleton().addText("AniManager null currAni");
	}

}

//-----------------------------------------------------------------------------
void LLAniManager::setAction(unsigned int actionid, bool force)
{
	//return;
	
	//LLScreenLog::getSingleton().addText("AniManager setAction id: "+Ogre::StringConverter::toString(actionid));

	// range test first
	//if (actionid >= m_vAnimations.size())
	//	return;

	// if idle, then no manual bone control...
	if (actionid == CAT_IDLE && m_fBlendMask != 1.0f)
		m_fBlendMaskDelta = 1.0f;

	m_iCurrAction = actionid;
	int oldstate = m_pFSM->getCurrentStateId();
	int newstate = m_pFSM->setEvent(actionid);

	//LLScreenLog::getSingleton().addText("AniManager getting state id returned: "+Ogre::StringConverter::toString(newstate));

	// make sure FSM returned proper id
	if (newstate == -1 || newstate >= m_vAnimations.size())
		return;
	
	//LLScreenLog::getSingleton().addText("AniManager changing animation...");

	if (force)
	{
		if (m_pCurrAnim != NULL)
		{
			m_pCurrAnim->endAnimation();
		}
		if (m_pOldAnim != NULL)
		{
			m_pOldAnim->endAnimation();
		}
		
		//m_pOldAnim = m_pCurrAnim;
		m_pOldAnim = NULL;
		m_pCurrAnim = m_vAnimations[newstate];
		m_pCurrAnim->startAnimation(actionid);
		
		return;
	}

	if (oldstate == newstate)
	{
/*		if (m_pCurrAnim == NULL)
			m_pCurrAnim = m_vAnimations[newstate];
		
		m_pCurrAnim->startAnimation(actionid);
*/
		if (m_pCurrAnim == NULL)
		{
			m_pCurrAnim = m_vAnimations[newstate];
			m_pCurrAnim->startAnimation(actionid);
		}

	}
	else
	{
		if (m_pCurrAnim == NULL)
		{		
			m_pCurrAnim = m_vAnimations[newstate];
			m_pCurrAnim->startAnimation(actionid);
		}
		else
		{
			if (m_pOldAnim)
			{
				m_pOldAnim->endAnimation();
			}

			m_pOldAnim = m_pCurrAnim;
			m_pCurrAnim = m_vAnimations[newstate];
			m_pCurrAnim->startAnimation(actionid);

			//m_pCurrAnim->setBlend(true);
			m_pOldAnim->setBlend(false, m_pCurrAnim->getBlendTime());
			m_pCurrAnim->setBlend(true, m_pOldAnim->getBlendOutTime());
		}
	}

}

//-----------------------------------------------------------------------------
void LLAniManager::setBlendMask(float value, float duration)
{
	//printf("setBlendMask(%f, %f)\n", value, duration);
//	if (duration < 0.0f)
	{
		m_fBlendMask = value;
		m_fBlendMaskDelta = 0.0f;
	}
	/*else
	{
		// let's do smooth transition...
		m_fBlendMaskDelta = (value - m_fBlendMask) / duration;
		printf("setBlendMask transition...\n");
	}*/
}

//-----------------------------------------------------------------------------
float LLAniManager::getMovingSpeed()
{
	// retrieve moving speed of current animation
	if (!m_pCurrAnim)
		return 0.0f;

	// 
	return m_pCurrAnim->getMovingSpeed();

}