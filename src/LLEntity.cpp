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
* Filename: LLEntity.cpp
* -----------------------------------------------------------------------------
* Notes:    
* -----------------------------------------------------------------------------
*/
static unsigned int g_iLLEntityID = 0;

#include "LLEntity.h"
#include "LLScreenLog.h"

//-----------------------------------------------------------------------------
LLEntity::LLEntity(void)
{
	m_ID			= g_iLLEntityID;
	m_eType			= ET_ENTRY;
	m_pEntity		= NULL;
	m_pNode			= NULL;
	m_pParentNode	= NULL;

	g_iLLEntityID++;
}

//-----------------------------------------------------------------------------
LLEntity::LLEntity(SceneNode* ParentNode)
{
	m_ID			= g_iLLEntityID;
	m_eType			= ET_ENTRY;
	m_pEntity		= NULL;
	m_pNode			= NULL;
	m_pParentNode	= ParentNode;

	g_iLLEntityID++;
}

//-----------------------------------------------------------------------------
LLEntity::~LLEntity(void)
{

}

//-----------------------------------------------------------------------------
Entity* LLEntity::createEntity(SceneManager* mgr, const String& entityName, const String& meshName, const String& materialName)
{
	if (m_pParentNode == NULL)
		m_pParentNode = mgr->getRootSceneNode();

	
	// Quad?
	m_eType	= ET_ITEM;
	if (strncmp(meshName.c_str(), "Plane", 5)==0)
	{
		// base size is 200 x 200
		m_pEntity = mgr->createEntity(entityName, Ogre::SceneManager::PT_PLANE);
	}
	else if (strncmp(meshName.c_str(), "Cube", 5)==0)
	{
		// base size is 100 x 100 x 100
		m_pEntity = mgr->createEntity(entityName, Ogre::SceneManager::PT_CUBE);
	}
	else if (strncmp(meshName.c_str(), "Sphere", 5)==0)
	{
		// base radious is 50
		m_pEntity = mgr->createEntity(entityName, Ogre::SceneManager::PT_SPHERE);
	}
	else
	{
		preLoadMesh(meshName);
		m_pEntity = mgr->createEntity(entityName, meshName);
		m_eType			= ET_ENTRY;
	}

	m_pNode = m_pParentNode->createChildSceneNode(entityName);
	
	m_pNode->attachObject(m_pEntity);

	if (strlen(materialName.c_str()) != 0)
		m_pEntity->setMaterialName(materialName);

	m_pEntity->setQueryFlags(m_eType);

	return m_pEntity;
}

//-----------------------------------------------------------------------------
void LLEntity::update(Real addedTime)
{
	// Entity specific update routine for each frame
}

//-----------------------------------------------------------------------------
void LLEntity::setPosition(Vector3 &pos)
{
	if (m_pNode)
		m_pNode->setPosition(pos);

	m_pNode->getPosition();
}

//-----------------------------------------------------------------------------
const Vector3 & LLEntity::getPosition(void)
{
	if (m_pNode)
		return m_pNode->getPosition();
	else
		return Vector3(0,0,0);
}

//-----------------------------------------------------------------------------
void LLEntity::setOrientation(Quaternion &q)
{
	if (m_pNode)
		m_pNode->setOrientation(q);
}

//-----------------------------------------------------------------------------
const Quaternion & LLEntity::getOrientation()
{
	if (m_pNode)
		return m_pNode->getOrientation();
	else
		return Quaternion();
}
//-----------------------------------------------------------------------------
void LLEntity::setScale(Vector3 &scale)
{
	if (m_pNode)
		m_pNode->setScale(scale);
}

//-----------------------------------------------------------------------------
void LLEntity::setParent(SceneNode* parent)
{
	if (parent == NULL)
		return;

	// remove this from currnet parent node
	unsigned short numChild = m_pParentNode->numChildren();
	for (int i=0; i < numChild ; i++)
	{
		Node* nd = m_pParentNode->getChild(i);
		if (nd == m_pNode)
		{
			m_pParentNode->removeChild(i);
			break;
		}
	}

	// add this node to parent child list
	m_pParentNode = parent;
	m_pParentNode->addChild(m_pNode);
}

//-----------------------------------------------------------------------------
void LLEntity::preLoadMesh(const String& meshName)
{
	// Pre-load the mesh so that we can tweak it with a manual animation if it has Poses
    MeshPtr mesh = MeshManager::getSingleton().load(meshName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	
	// custom stuff for this entity
    unsigned short src, dest;
    if (!mesh->suggestTangentVectorBuildParams(VES_TANGENT, src, dest))
    {
        mesh->buildTangentVectors(VES_TANGENT, src, dest);
    }

	// nothing for LLENTITY
}

//-----------------------------------------------------------------------------
void LLEntity::rotate(Quaternion &q)
{
	m_pNode->rotate(q);
}

//-----------------------------------------------------------------------------
void LLEntity::setVisible(bool visible)
{
	m_pNode->setVisible(visible);
}

void LLEntity::setVisible(const String& subEntity, bool visible)
{
	if (!m_pEntity)
		return;

	LLScreenLog::getSingleton().addText("Entity setVisible: "+ Ogre::String(subEntity));

	// need exception handling
	SubEntity* sub = m_pEntity->getSubEntity(subEntity);

	if (sub)
		sub->setVisible(visible);
	
}

//-----------------------------------------------------------------------------
void LLEntity::toggleVisible()
{
	if (!m_pEntity)
		return;

	if (m_pEntity->isVisible())
		m_pNode->setVisible(false);
	else
		m_pNode->setVisible(true);
}

//-----------------------------------------------------------------------------
bool LLEntity::isVisible()
{
	return m_pEntity->isVisible();
}

const String& LLEntity::getName()
{
	if (!m_pEntity)
		return "";

	return m_pEntity->getName();

}
