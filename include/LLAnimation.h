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
* Filename: LLAnimation.h
* -----------------------------------------------------------------------------
* Notes:    Animation class - work for character animation for now
*							  but need to change it to more generic one
* -----------------------------------------------------------------------------
*/

#ifndef __LLANIMATION_H_
#define __LLANIMATION_H_

#include <vector>
#include <Ogre.h>
using namespace Ogre;

#include "LLCallBack.h"
#include "LLCharacter.h"

enum tEVENT;

typedef std::vector <LLEvent*>	iAniEvent_Vec;

class LLAniManager;

class LLAnimation
{
public:
	LLAnimation(LLAniManager* mgr, Entity* ent, const char* name);
	~LLAnimation(void);

	void	update(Real addedTime);
	
	void	startAnimation(int actionid);
	void	endAnimation();
	
	void	destroyNodeTrack(const char * boneName);
	
	void	setBlend(bool fadein = true, float outtime = 0.5f);
	void	setBlendTime(float value);
	float	getBlendTime() { return m_fBlendTime;}
	float	getBlendOutTime() { return m_fBlendOutTime;}
	void	setBlendMask(int handle, float weight);
	void	setLoop(bool loop);
	
	void	setWeight(float value);
	void	addEvent(float etime, tEVENT type, const char* param);

	float	getLength();
	void	resetEvent();

	float	getMovingSpeed() { return m_fMovingSpeed;}
	void	setMovingSpeed(float speed) { m_fMovingSpeed = speed;}

	
	TCallback<LLCharacter> i_CharacterEventCallback;

protected:

	Entity*			m_pEntity;
	Animation*		m_pAnimation;
	AnimationState*	m_pAnimationState;
	float			m_fBlendTime;
	float			m_fBlendOutTime;
	float			m_fWeight;
	float			m_fBlendDelta;
	
	iAniEvent_Vec	m_vEventVec;
	int				m_iNumEvents;
	int				m_iNextEvent;
	float			m_fNextEventTime;

	LLAniManager*	m_pAnimMgr;
	int				m_iActionId;

	bool			m_bEventDone;
	float			m_fLength;
	float			m_fCycleElapsed;

	// moving speed (cm/sec) for walk/run kind animation
	float			m_fMovingSpeed;
};

#endif
