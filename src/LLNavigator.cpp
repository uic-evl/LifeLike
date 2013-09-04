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
* Filename: LLNavigator.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#include "LLNavigator.h"
#include "LLdefine.h"

//-----------------------------------------------------------------------------
LLNavigator::LLNavigator(Camera* cam, Vector3 &pos, Vector3 &headPos)
{
	m_pCamera = cam;
	m_vInitPosition = pos;
	m_vPosition = pos;
	m_vInitHeadPosition = headPos;
	m_vHeadPosition = headPos;

	m_fFOVy = 30.0f;
	m_fScreenWidth = 53.0f;	// centimeter (for 24" monitor)
	m_fScreenHeight = 30.0f;
	m_fScreenBottom = 110.0f;
	m_fHeadDistance = headPos.z;
	m_fNear = m_pCamera->getNearClipDistance();
	
	//
	m_pCameraNode = (SceneNode*)m_pCamera->getParentNode();
	m_pNaviGatorNode = m_pCamera->getSceneManager()->getRootSceneNode()->createChildSceneNode("LLNavigator", pos);
	m_pHeadNode = m_pNaviGatorNode->createChildSceneNode("LLNaviHead", headPos);
}

//-----------------------------------------------------------------------------
LLNavigator::~LLNavigator(void)
{
}

//-----------------------------------------------------------------------------
void LLNavigator::update()
{
	// compute camera postion, lookat, and FOV
	
	// translation
	Vector3 camPos = m_pHeadNode->_getDerivedPosition();
	//m_pCamera->setPosition(camPos);
	m_pCameraNode->setPosition(camPos);

	// frustom update
	float left, right, top, bottom, nd;
	nd = m_fNear / m_fHeadDistance;
	left = (-m_fScreenWidth*0.5 - m_vHeadPosition.x) * nd;
	right = (m_fScreenWidth*0.5 - m_vHeadPosition.x) * nd;
	top = (m_fScreenBottom + m_fScreenHeight - m_vHeadPosition.y) * nd;
	bottom = (m_fScreenBottom - m_vHeadPosition.y) * nd;
	m_pCamera->setFrustumExtents(left, right, top, bottom);

	// orientation
	Quaternion q = m_pNaviGatorNode->getOrientation();
	q = q.Inverse();
	Ogre::Vector3 dir = q * Vector3(0, 0, 1);
	Vector3 lookAt = camPos + (dir.normalisedCopy() * -m_vHeadPosition.z);
	//m_pCamera->lookAt(lookAt);
	m_pCameraNode->lookAt(lookAt, Node::TransformSpace::TS_WORLD);

	// FOV
	Radian a = 2 * Math::ATan(m_fScreenWidth/(2 * m_fHeadDistance * m_pCamera->getAspectRatio()));
	m_pCamera->setFOVy(a);

}

//-----------------------------------------------------------------------------
void LLNavigator::reset()
{
	m_vPosition = m_vInitPosition;
	m_vHeadPosition = m_vInitHeadPosition;
	
	m_pNaviGatorNode->setPosition(m_vPosition);
	m_pNaviGatorNode->resetOrientation();
	m_pHeadNode->setPosition(m_vHeadPosition);
	m_pHeadNode->resetOrientation();

	update();
}

//-----------------------------------------------------------------------------
// head position is relation position of head in navigator space
Vector3 & LLNavigator::setHeadPosition(Vector3 &pos)
{
	// head position is set relative to parent (navigator) node
	m_vHeadPosition = pos;

	// limit check: head should be within navigator volume
	m_vHeadPosition.x = CLAMP(m_vHeadPosition.x, -m_fScreenWidth, m_fScreenWidth);
	m_vHeadPosition.y = CLAMP(m_vHeadPosition.y, m_fScreenBottom, m_fScreenBottom + m_fScreenHeight*2.0f);
	m_vHeadPosition.z = CLAMP(m_vHeadPosition.z, m_fScreenWidth*0.1f, m_fScreenWidth*5);

	m_pHeadNode->setPosition(m_vHeadPosition);

	// distance from head to screen
	// since our head is alwyas located inside navigator node
	// relative z coordinate of head node is distance to screen
	m_fHeadDistance = m_vHeadPosition.z;

	update();

	return m_vHeadPosition;
}

//-----------------------------------------------------------------------------
void LLNavigator::moveNavigator(Vector3 &pos)
{
	Quaternion q = m_pNaviGatorNode->getOrientation();
	Vector3 newpos = pos;
	newpos = q.Inverse() * newpos;

	// navigator always move along its local coordinate system
	m_pNaviGatorNode->translate(newpos);
	m_vPosition = m_pNaviGatorNode->_getDerivedPosition();


	update();
}

//-----------------------------------------------------------------------------
void LLNavigator::rotateNavigator(float angle)
{
	// navigator rotate only along y axis
	m_pNaviGatorNode->yaw(Radian(Degree(angle)));

	update();
}


//-----------------------------------------------------------------------------
void LLNavigator::setScreenWidth(float w)
{
	// only expose width setter. then, compute height based on viewport
	m_fScreenWidth = w;
	//  ratio: aspect = width / height
	m_fScreenHeight = m_fScreenWidth / m_pCamera->getAspectRatio();

	update();
}

//-----------------------------------------------------------------------------
void LLNavigator::setScreenBottom(float b)
{
	m_fScreenBottom = b;
}
