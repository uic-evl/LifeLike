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
* Filename: LLAniManager.h
* -----------------------------------------------------------------------------
* Notes:    Animation Manager class of LifeLike. Handles all ani related work
* -----------------------------------------------------------------------------
*/

#ifndef __LLANIMANAGER_H_
#define __LLANIMANAGER_H_

#include <vector>
#include <Ogre.h>

class LLFSM;
class LLAnimation;
class LLCharacter;
class LLAnimationBlender;

using namespace Ogre;

typedef std::vector <LLAnimation*>	iAnimation_Vec;

class LLAniManager
{
public:
	LLAniManager(Entity* ent, LLCharacter* character);
	~LLAniManager(void);
	
	void loadFSM(const char* filename);
	void loadAnimations(const char* filename);
	void addAnimation(LLAnimation* anim);
	void addSkeletons(const char* filename, const char* masterfile);
	void destroyNodeTrack(const char* bonename);
	void update(Real addedTime);
	void animationEnded(LLAnimation* anim);
	
	void setAction(unsigned int actionid, bool force = false);
	void setAnimation(int id, const char* animation);
	void setBlendMask(float value, float duration = -1.0f);
	LLCharacter* getCharacter() { return m_pOwner;}

	float getMovingSpeed();

protected:
	
	Entity*				m_pEntity;
	LLCharacter*		m_pOwner;

	// state machine: transition control
	LLFSM*				m_pFSM;
	
	// vector of animations
	iAnimation_Vec		m_vAnimations;

	int					m_iCurrAction;
	LLAnimation*		m_pOldAnim;
	LLAnimation*		m_pCurrAnim;
	float				m_fBlendTime;
	unsigned short		m_uHeadBoneHandle;
	float				m_fBlendMask;
	float				m_fBlendMaskDelta;

	LLAnimationBlender*	m_pAniBlender;

};

#endif
