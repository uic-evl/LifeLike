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
* Filename: LLNavigator.h
* -----------------------------------------------------------------------------
* Notes:    Navigator class - for cave like navigation utility
* -----------------------------------------------------------------------------
*/

#ifndef __LLNAVIGATOR_H__
#define __LLNAVIGATOR_H__

#include <Ogre.h>
using namespace Ogre;

//sound class
class LLNavigator
{
public:
	LLNavigator(Camera* cam, Vector3 &pos, Vector3 &headPos);
	~LLNavigator();

	void update();
	void reset();

	Vector3 & setHeadPosition(Vector3 &pos);
	void moveNavigator(Vector3 &pos);
	void rotateNavigator(float angle);

	void setScreenWidth(float w);
	void setScreenBottom(float b);

protected:

	Camera*			m_pCamera;
	SceneNode*		m_pCameraNode;
	SceneNode*		m_pNaviGatorNode;
	SceneNode*		m_pHeadNode;

	Vector3			m_vInitPosition;
	Vector3			m_vInitHeadPosition;

	Vector3			m_vPosition;
	Vector3			m_vHeadPosition;
	float			m_fScreenWidth;
	float			m_fScreenHeight;
	float			m_fScreenBottom;
	float			m_fFOVy;
	float			m_fNear;
	float			m_fHeadDistance;

};

#endif
