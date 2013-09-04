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
* Filename: LLItem.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/

#include "LLItem.h"

//-----------------------------------------------------------------------------
LLItem::LLItem(void)
{

}

//-----------------------------------------------------------------------------
LLItem::LLItem(SceneNode* ParentNode)
{
	m_pParentNode = ParentNode;
}

//-----------------------------------------------------------------------------
LLItem::~LLItem(void)
{

}

//-----------------------------------------------------------------------------
Entity* LLItem::createEntity(SceneManager* mgr, const String& entityName, const String& meshName)
{

	if (m_pParentNode == NULL)
		m_pParentNode = mgr->getRootSceneNode();

	preLoadMesh(meshName);

	m_pEntity = mgr->createEntity(entityName, meshName);
	m_pNode = m_pParentNode->createChildSceneNode(entityName);
	m_pNode->attachObject(m_pEntity);
	
	return m_pEntity;
}

//-----------------------------------------------------------------------------
void LLItem::preLoadMesh(const String& meshName)
{
	// Pre-load the mesh so that we can tweak it with a manual animation if it has Poses
    MeshPtr mesh = MeshManager::getSingleton().load(meshName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	
	// custom stuff for this entity

	// nothing for LLItem for now

}