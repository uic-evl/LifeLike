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
* Filename: LLAnimation.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/
#include "LLAnimation.h"
#include "LLAniManager.h"
#include "LLdefine.h"
#include "LLCharacter.h"

//-----------------------------------------------------------------------------
LLAnimation::LLAnimation(LLAniManager* mgr, Entity* ent, const char* name)
{
	m_pAnimMgr = mgr;
	m_pEntity = ent;

	SkeletonInstance* skel = m_pEntity->getSkeleton();
	m_pAnimation = skel->getAnimation(name);
	m_pAnimationState = m_pEntity->getAnimationState (name);
	m_pAnimationState->setEnabled(false);
	m_pAnimationState->setLoop(false);
	m_pAnimationState->setWeight(0);
	m_pAnimationState->setTimePosition(0);
	m_pAnimationState->createBlendMask(skel->getNumBones());

	m_fBlendTime = 0.5f;		// 0.5 sec blending
	m_fBlendOutTime = 0.5f;		// 0.5 sec fadeout
	m_fWeight = 1.0f;			// full weight
	m_fBlendDelta = 0.0f;		// blend delta 0

	m_iNumEvents = 0;
	m_iNextEvent = -1;
	m_fNextEventTime = 0.0f;
	m_bEventDone = false;

	m_iActionId = -1;

	m_fLength = getLength();
	m_fCycleElapsed = 0.0f;

	m_fMovingSpeed = 0.0f;
}

//-----------------------------------------------------------------------------
LLAnimation::~LLAnimation(void)
{
	int size = (int)m_vEventVec.size();
	for (int i=0; i<size; i++)
	{
		LLEvent* ae = m_vEventVec[i];
		delete ae;
		m_vEventVec[i] = NULL;
	}
	
	m_vEventVec.clear();
}

//-----------------------------------------------------------------------------
void LLAnimation::destroyNodeTrack(const char * boneName)
{
	// manual head bone control
	SkeletonInstance* skel = m_pEntity->getSkeleton();
	Bone* mbone = skel->getBone(boneName);
	mbone->setManuallyControlled(true);
	m_pAnimation->destroyNodeTrack(mbone->getHandle());
	mbone->resetOrientation();
}

//-----------------------------------------------------------------------------
void LLAnimation::update(Real addedTime)
{
	if (!m_pAnimationState)
		return;
	if (!m_pAnimationState->getEnabled())
		return;

	// calculate new weight
	if (m_fBlendDelta != 0.0f)
	{
		m_fWeight += m_fBlendDelta*addedTime;
		m_fWeight = CLAMP(m_fWeight, 0.0f, 1.0f);
		if (m_fWeight == 1.0f)
		{
			m_fBlendDelta = 0.0f;

		}
		else if (m_fWeight == 0.0f)
		{
			// fade out done
			m_pAnimationState->setWeight(m_fWeight);
			endAnimation();
			m_pAnimMgr->animationEnded(this);
			return;
		}
	}

	// apply weight & update
	//m_pAnimationState->setWeight(m_fWeight);
	//m_pAnimationState->addTime(addedTime);
	
	// cycle check
	m_fCycleElapsed += addedTime;
	if (m_fCycleElapsed > m_fLength)
	{
		// loop animation case
		if (m_pAnimationState->getLoop())
		{
			m_pAnimationState->setWeight(m_fWeight);
			m_pAnimationState->addTime(addedTime);

			m_fCycleElapsed = 0;
			resetEvent();
		}
		else	// non-loop animation
		{
			// what if I am in the middle of fade-out
			// animation manager should take care of this
			/*if (m_fBlendDelta != 0.0f)
			{
				// let's just make it loop while fade in/out
				m_fCycleElapsed -= m_fLength;
				m_pAnimationState->setTimePosition(m_fCycleElapsed);
			}
			else*/
			{
				// report back to animation manager
				endAnimation();
				m_pAnimMgr->animationEnded(this);
				return;
			}

		}
	}
	else
	{
		m_pAnimationState->setWeight(m_fWeight);
		m_pAnimationState->addTime(addedTime);

	}

	// end test (only for not looping animation)
/*	if (m_pAnimationState->hasEnded())
	{
		endAnimation();
		m_pAnimMgr->animationEnded(this);
		return;
	}
*/
	// check event associated with this animation
	if (m_iNumEvents && !m_bEventDone/* && m_fBlendDelta == 0*/)
	{
		float currPos = m_pAnimationState->getTimePosition();
		
		// there is possibility that multiple events have the same timestamp...
		// should do some kind of iteration.
		bool running = true;
		while (running)
		{
			if (m_fNextEventTime < currPos)
			{
				// for thesis testing: CAT_SIMUL event only occur once
				if (m_vEventVec[m_iNextEvent]->ids[0] == CAT_SIMUL)
				{
					if (!m_vEventVec[m_iNextEvent]->simUsed)
					{
						m_vEventVec[m_iNextEvent]->simUsed = true;
						i_CharacterEventCallback.execute(m_vEventVec[m_iNextEvent]);
					}
				}
				else
					i_CharacterEventCallback.execute(m_vEventVec[m_iNextEvent]);
				// end of thesis testing

				//i_CharacterEventCallback.execute(m_vEventVec[m_iNextEvent]);

				// move to next event
				m_iNextEvent++;
				if (m_iNextEvent == m_iNumEvents)
				{
					// reached the last event
					m_iNextEvent = 0;
					m_bEventDone = true;
					running = false;
				}
				m_fNextEventTime = m_vEventVec[m_iNextEvent]->eTime;
			}
			else
				running = false;
		}

	}
}

//-----------------------------------------------------------------------------
void LLAnimation::setBlend(bool fadein, float outtime)
{
	if (fadein)	// fade in
	{
		if (m_fBlendTime == 0)
		{
			m_fWeight = 1.0f;
			m_fBlendDelta = 0.0f;
		}
		else
		{
			m_fWeight = 0.0f;
			m_fBlendDelta = 1.0f / m_fBlendTime;
		}
	}
	else		// fade out
	{
		if (outtime == 0)
			endAnimation();
		else
		{
			// what if fade-out time is longer than remaining animation in case of non-loop animation
			// let's enforce it to be same as remaing period at most
			//float remain = m_fLength - m_fCycleElapsed;
			//outtime = std::min(remain, outtime);

			m_fWeight = 1.0f;
			m_fBlendOutTime = outtime;
			m_fBlendDelta = -1.0f / m_fBlendOutTime;

			// need to take care of sound event if there is active one
			// or we may just stop all related sound
			/*int vsize = m_vEventVec.size();
			for (int i=0; i < vsize; i++)
			{
				if (m_vEventVec[i]->type == AET_SOUND)
					m_pAnimMgr->playSound(m_vEventVec[i]->ids[0], false);
			}*/
			m_bEventDone = true;
		}
	}
}

//-----------------------------------------------------------------------------
void LLAnimation::setBlendTime(float value)
{
	if (value > 0.0f)
		m_fBlendTime = value;
}

//-----------------------------------------------------------------------------
void LLAnimation::setWeight(float value)
{
	if (value >= 0.0f)
		m_fWeight = value;
}

//-----------------------------------------------------------------------------
void LLAnimation::addEvent(float etime, tEVENT type, const char* param)
{
	LLEvent* ae = new LLEvent;
	ae->eTime = etime;
	ae->type = type;

	// several special case
	strcpy(ae->param, param);
	if (type == EVT_ACTION)
	{
		ae->ids[0] = CAT_IDLE;			// just default

		if (strcmp(param, "CAT_IDLE") == 0)
			ae->ids[0] = CAT_IDLE;
		else if (strcmp(param, "CAT_READ") == 0)
			ae->ids[0] = CAT_READ;
		else if (strcmp(param, "CAT_LISTEN") == 0)
			ae->ids[0] = CAT_LISTEN;
		else if (strcmp(param, "CAT_SPEAK") == 0)
			ae->ids[0] = CAT_SPEAK;
		else if (strcmp(param, "CAT_WRITE") == 0)
			ae->ids[0] = CAT_WRITE;
		else if (strcmp(param, "CAT_DRINK") == 0)
			ae->ids[0] = CAT_DRINK;
		else if (strcmp(param, "CAT_SIMUL") == 0)		// for thesis testing
			ae->ids[0] = CAT_SIMUL;

	}
	else if (type == EVT_SHAPE)
	{
		// shape id, duration, max, mid, maxduration
		// need to tokenize
		char *p_token;
		char seps[] = " ,\t\n";
		char pstr[128];
		strcpy(pstr, param);
		p_token = strtok(pstr, seps);
		ae->ids[1] = atof(p_token);		// id
		
		// parameters: duration, max, mid, maxduration
		p_token = strtok(NULL, seps);
		int idx = 0;
		while(p_token != NULL)
		{
			ae->args[idx] = atof(p_token);
			p_token = strtok(NULL, seps);
			idx++;
		}
		
		// a bit of validation
		if (ae->args[0] == 0.0f || ae->args[2] > ae->args[0] || ae->args[0] < ae->args[3])
		{
			delete ae;
			return;
		}

	}
	else if (type == EVT_SOUND)
	{
		ae->ids[0] = atoi(param);
	}
	
	m_vEventVec.push_back(ae);

	m_iNumEvents++;
	
	if (m_iNextEvent == -1)
	{
		m_iNextEvent = 0;
		m_fNextEventTime = etime;
	}
}

//-----------------------------------------------------------------------------
void LLAnimation::startAnimation(int actionid)
{
	m_pAnimationState->setTimePosition(0);
	//m_pAnimationState->setWeight(1.0f);
	m_pAnimationState->setEnabled(true);
	m_iActionId = actionid;
	m_fWeight = 1.0f;
	m_fBlendDelta = 0.0f;
	m_fCycleElapsed = 0.0f;

	resetEvent();
}

//-----------------------------------------------------------------------------
void LLAnimation::endAnimation()
{
	m_pAnimationState->setTimePosition(0);
	m_pAnimationState->setEnabled(false);
	m_pAnimationState->setWeight(0.0f);
	
	m_fWeight = 1.0f;
	m_fBlendDelta = 0.0f;
	m_fCycleElapsed = 0.0f;

	resetEvent();
}

//-----------------------------------------------------------------------------
void LLAnimation::setLoop(bool loop)
{
	m_pAnimationState->setLoop(loop);
}

//-----------------------------------------------------------------------------
float LLAnimation::getLength()
{
	return m_pAnimationState->getLength();
}

//-----------------------------------------------------------------------------
void LLAnimation::resetEvent()
{
	if (m_iNumEvents)
	{
		m_iNextEvent = 0;
		m_fNextEventTime = m_vEventVec[m_iNextEvent]->eTime;
		m_bEventDone = false;
	}

}

//-----------------------------------------------------------------------------
void LLAnimation::setBlendMask(int handle, float weight)
{
	m_pAnimationState->setBlendMaskEntry(handle, weight);
}
