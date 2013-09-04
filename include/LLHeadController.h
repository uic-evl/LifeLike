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
* Filename: LLHeadController.h
* -----------------------------------------------------------------------------
* Notes:   
* -----------------------------------------------------------------------------
*/

#ifndef __LLHEADCONTROLLER_H_
#define __LLHEADCONTROLLER_H_

#include <Ogre.h>
using namespace Ogre;

// container for tracknames with the same type NVB
typedef struct fNVBMotinoGroup
{
	int type;
	int lastused;
	std::vector <std::string> sVec;	// vector holding tracknames
	std::vector <int> pool;			// temp random pool
} sNVBMotionGroup;
typedef std::map<int,sNVBMotionGroup> nvbMap;

typedef struct fNVBAnimState
{
	float	fBlendFactor;
	float	fFadeDelta;
	float	fFadeOut;
	float	fElapsedTime;
	float	fWeight;			
	AnimationState*		pAniState;

	fNVBAnimState()
	{
		fWeight = 1.0f;
		pAniState = NULL;
	}
} sNVBAnimState;


class LLHeadController
{
public:
	LLHeadController(Bone* head);
	~LLHeadController();

	void	setPitchOffset(float pitch) { m_fPitchOffset = pitch;}

	void	update(Real addedTime);

	void	loadAnimationFromFile(const char* trackName, const char* type, const char* filename);

	// weight value is 0.0 ~ 1.0 to blend its value
	void	activateAnimation(int type, float weight=1.0f, float oweight=1.0f);

protected:

	void	activateAnimation(const char* trackName, float weight=1.0f);

	float		m_fPitchOffset;
	Bone*		m_pHeadBone;
	SceneNode*	m_pDummyNode;

	AnimationStateMap	m_AnimationStates;

	// should have at least two active animation state
	// when new gesture comes in while current one is still acitve, we need to blend it nicely
	// let's just add one more as previous state so that we can play them at most two in case
	sNVBAnimState		m_CurrentAniState;
	sNVBAnimState		m_PreviousAniState;


	nvbMap		m_nvbGroupMap;

	char		m_cDebugString[128];
	float		m_fLogTimer;
};

#endif