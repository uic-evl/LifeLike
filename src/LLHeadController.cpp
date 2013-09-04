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
* Filename: LLHeadController.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#include "LLHeadController.h"
#include "LLdefine.h"
#include "LLScreenLog.h"

//-----------------------------------------------------------------------------
LLHeadController::LLHeadController(Bone* head)
{
	m_pHeadBone = head;

	m_pDummyNode = NULL;
	SceneManager* pOGRESceneMgr = Ogre::Root::getSingleton().getSceneManager("LifeLikeSceneManager");
	if (pOGRESceneMgr)
	{
		m_pDummyNode = pOGRESceneMgr->createSceneNode("LifeLikeHeadControllerDummy");
	}
	
	m_fPitchOffset = 0.0f;

	m_fLogTimer = 0.0f;
}


//-----------------------------------------------------------------------------
LLHeadController::~LLHeadController()
{

}


//-----------------------------------------------------------------------------
void LLHeadController::update(Real addedTime)
{
	m_fLogTimer += addedTime;
	
	if (!m_pDummyNode || !m_CurrentAniState.pAniState)
		return;

	// current animation state update
	{
		m_CurrentAniState.pAniState->addTime(addedTime);

		// fading factor
		m_CurrentAniState.fElapsedTime += addedTime;
		if (m_CurrentAniState.fElapsedTime > m_CurrentAniState.fFadeOut && m_CurrentAniState.fFadeDelta > 0.0f)
			m_CurrentAniState.fFadeDelta *= -1.0f;

		m_CurrentAniState.fBlendFactor += m_CurrentAniState.fFadeDelta * addedTime;
		m_CurrentAniState.fBlendFactor = CLAMP(m_CurrentAniState.fBlendFactor, 0.0f, 1.0f);

		m_CurrentAniState.pAniState->setWeight(m_CurrentAniState.fBlendFactor*m_CurrentAniState.fWeight);

	}

	// the previous animation state update if exist
	if (m_PreviousAniState.pAniState)
	{
		m_PreviousAniState.pAniState->addTime(addedTime);

		// fading factor: this should be different from current one. previous one should only decrease
		m_PreviousAniState.fBlendFactor += m_PreviousAniState.fFadeDelta * addedTime;
		m_PreviousAniState.fBlendFactor = CLAMP(m_PreviousAniState.fBlendFactor, 0.0f, 1.0f);
		m_PreviousAniState.pAniState->setWeight(m_PreviousAniState.fBlendFactor*m_PreviousAniState.fWeight);
	}

	// apply head orientation
	Quaternion qt;
	qt = m_pDummyNode->getOrientation();
	m_pHeadBone->setOrientation(qt);

	// finished?
	if (m_CurrentAniState.pAniState->hasEnded() || m_CurrentAniState.fBlendFactor == 0.0f)
	{
		m_CurrentAniState.pAniState->setEnabled(false);
		m_CurrentAniState.pAniState->setTimePosition(0);
		m_CurrentAniState.pAniState = NULL;
	}
	if (m_PreviousAniState.pAniState && (m_PreviousAniState.pAniState->hasEnded() || m_PreviousAniState.fBlendFactor == 0.0f ))
	{
		m_PreviousAniState.pAniState->setEnabled(false);
		m_PreviousAniState.pAniState->setTimePosition(0);
		m_PreviousAniState.pAniState = NULL;
	}

}
//-----------------------------------------------------------------------------
void LLHeadController::activateAnimation(const char* trackName, float weight)
{
	if (!m_pDummyNode)
		return;

	// find track
	AnimationStateMap::iterator i = m_AnimationStates.find(trackName);
	if (i != m_AnimationStates.end())
	{
		if (!i->second->getEnabled())
		{
			i->second->setEnabled(true);
			
			// this should not the case. only one at one time
			if (m_CurrentAniState.pAniState)
			{
				if (m_PreviousAniState.pAniState)
					LLScreenLog::getSingleton().addText("*************** NVB head gesture overlap. Nooooo!");

				// set the current one to previous one!!! this may need to finish earlier than the original setting
				m_PreviousAniState = m_CurrentAniState;
				if (m_PreviousAniState.fFadeDelta > 0.0f)
					m_PreviousAniState.fFadeDelta * -1.0f;
			}

			m_CurrentAniState.pAniState = i->second;

			// 
			m_CurrentAniState.fFadeDelta = 1.0f / (m_CurrentAniState.pAniState->getLength() * 0.25f);
			m_CurrentAniState.fFadeOut = m_CurrentAniState.pAniState->getLength() * 0.75f;
			m_CurrentAniState.fElapsedTime = 0.0f;
			m_CurrentAniState.fWeight = weight;
		}
	}
}

//-----------------------------------------------------------------------------
void LLHeadController::activateAnimation(int type, float weight, float oweight)
{
	if (!m_pDummyNode)
		return;

	// manual control: thesis testing purpose (0.5, 0.1, comment out)
	if (type < 4)
		sprintf(m_cDebugString, "*** Nod Head Gesture Value: %f %f %f", m_fLogTimer, oweight, weight);
	else
		sprintf(m_cDebugString, "*** Shake Head Gesture Value: %f %f %f", m_fLogTimer, oweight, weight);
	LLScreenLog::getSingleton().addText(m_cDebugString);
	//weight = 0.5f;
																																																																					
	// this is more appropriate for external call by type of NVB
	// there could be multiple of aniamtion tracks for this type																														
	// random non-repeatitive use of aniamtions for the same type
	const char* trackName = NULL;

	// find mvb group with given type
	nvbMap::iterator it = m_nvbGroupMap.find(type);
	if (it != m_nvbGroupMap.end())
	{
		int poolsize = it->second.sVec.size();

		// has only one?
		if (poolsize == 1)
			trackName = it->second.sVec[0].c_str();
		else
		{
			// pool is empty?
			if (it->second.pool.empty())
			{
				for (int i=0; i<poolsize; i++)
				{
					if (i != it->second.lastused)
						it->second.pool.push_back(i);
				}
				std::random_shuffle(it->second.pool.begin(), it->second.pool.end());
			}

			// pop the first one
			it->second.lastused = it->second.pool[0];
			it->second.pool.erase(it->second.pool.begin());

			trackName = it->second.sVec[it->second.lastused].c_str();
		}
	}

	if (trackName)
	{
		activateAnimation(trackName, weight);
	}
}

int findNVBType(const char* type)
{
	int t=0;

	// type should be all capital letters
	if (strcmp(type, "NVB_HEADSHAKE_SHORT")==0) t=NVB_HEADSHAKE_SHORT;
	else if (strcmp(type, "NVB_HEADSHAKE_MID")==0) t=NVB_HEADSHAKE_MID;
	else if (strcmp(type, "NVB_HEADSHAKE_LONG")==0) t=NVB_HEADSHAKE_LONG;
	else if (strcmp(type, "NVB_HEADNOD_SHORT")==0) t=NVB_HEADNOD_SHORT;
	else if (strcmp(type, "NVB_HEADNOD_MID")==0) t=NVB_HEADNOD_MID;
	else if (strcmp(type, "NVB_HEADNOD_LONG")==0) t=NVB_HEADNOD_LONG;
	else if (strcmp(type, "NVB_HEADSWEEP")==0) t=NVB_HEADSWEEP;

	return t;
}

//-----------------------------------------------------------------------------
void LLHeadController::loadAnimationFromFile(const char* trackName, const char* type, const char* filename)
{
	// load predefined animatino data
	// one track per file
	// we only need orientation data. ignore position, scale data

	// find lifelike default scene manager
	SceneManager* pOGRESceneMgr = Ogre::Root::getSingleton().getSceneManager("LifeLikeSceneManager");
	if (!pOGRESceneMgr || !m_pDummyNode)
		return;

	// open file
	FILE *fp;
	if (!(fp=fopen(filename,"r"))) 
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
	AnimationState* aniState = NULL;

	if (pOGRESceneMgr->hasAnimation(trackName))
	{
		pOGRESceneMgr->destroyAnimationState(trackName);
		pOGRESceneMgr->destroyAnimation(trackName);
	}

	// create animation and track
	Animation* anim = pOGRESceneMgr->createAnimation(trackName, anitime);
	NodeAnimationTrack* track = anim->createNodeTrack(0, m_pDummyNode);

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
			
		// orientation: computer euler angles to quaternion
		mat.FromEulerAnglesXYZ(Radian(fvalues[idx+4]), Radian(fvalues[idx+5]), Radian(fvalues[idx+6]));
		qt.FromRotationMatrix(mat);
		key->setRotation(qt);
	}

	// create animation state and add it to list
	// need some mechanism to keep all these user defined animations with scene manager
	aniState = pOGRESceneMgr->createAnimationState(trackName);
	aniState->setLoop(false);
	m_AnimationStates[trackName] = aniState;

	// find type
	int t = findNVBType(type);
	// find mvb group & insert this trackname
	nvbMap::iterator it = m_nvbGroupMap.find(t);
	if (it != m_nvbGroupMap.end())
		it->second.sVec.push_back(trackName);
	else
	{
		sNVBMotionGroup newGroup;
		newGroup.type = t;
		newGroup.sVec.push_back(trackName);
		m_nvbGroupMap[t] = newGroup;
	}

	// clean up temp storage
	delete fvalues;
	fclose(fp);

	return;

}