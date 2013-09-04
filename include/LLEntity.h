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
* Filename: LLEntity.h
* -----------------------------------------------------------------------------
* Notes:    Base class of LifeLike object. All game object inherit from this
* -----------------------------------------------------------------------------
*/

#ifndef __LLENTITY_H_
#define __LLENTITY_H_

#include <Ogre.h>

//-----------------------------------------------------------------------------
//  Basic Entity Type
//-----------------------------------------------------------------------------
enum eEntityType {
/*	ET_ENTRY		= 0x00000000,		// Just... Entry
	ET_ACTIVE_ENTRY	= 0x10000000,		// active entry
	ET_CHARACTER	= 0x20000000, 		// Player Character
	ET_NPC			= 0x30000000, 		// Non Player Character
	ET_ITEM			= 0x40000000, 		// Item
	ET_CONSTRUCT	= 0x50000000,		// Construction Item
	ET_ETC			= 0x60000000,		// others
*/					// ...
	ET_ENTRY		= 1<<0,		// Just... Entry
	ET_ACTIVE_ENTRY	= 1<<1,		// active entry
	ET_CHARACTER	= 1<<2, 	// Player Character
	ET_NPC			= 1<<3, 	// Non Player Character
	ET_ITEM			= 1<<4, 	// Item
	ET_CONSTRUCT	= 1<<5,		// Construction Item
	ET_ETC			= 1<<6,		// others
};

using namespace Ogre;

//-----------------------------------------------------------------------------
//  Entity (basic object)
//-----------------------------------------------------------------------------
class LLEntity
{
public:
	// -------------------------------------------------------------------------
	// Constructors and Destructor
	// -------------------------------------------------------------------------
	//LLEntity(void);
	LLEntity(SceneNode* ParentNode);
	virtual ~LLEntity(void);

	virtual Entity*	createEntity(SceneManager* mgr, const String& entityName, const String& meshName, const String& materialName="");

	// -------------------------------------------------------------------------
	// Tick
	// -------------------------------------------------------------------------
	virtual void update(Real addedTime);

	// -------------------------------------------------------------------------
	// Attribute setter & getter
	// -------------------------------------------------------------------------
	void			setID(unsigned int id) { m_ID = id;}
	unsigned int	getID()	{ return m_ID;}

	const String&	getName();
	
	void			setType(eEntityType type) { m_eType = type; }
	eEntityType		getType() { return m_eType; }
	
	void			setPosition(Vector3 &pos);
	void			setOrientation(Quaternion &q);
	void			setScale(Vector3 &scale);
	const Vector3 &		getPosition(void);
	const Quaternion &	getOrientation(void);

	void			setParent(SceneNode* parent);
	SceneNode*		getParent() { return m_pParentNode;}
	
	Entity*			getEntity() { return m_pEntity;}
	SceneNode*		getNode() { return m_pNode;}

	void			setVisible(bool visible);
	void			setVisible(const String& subEntity, bool visible);
	void			toggleVisible();
	bool			isVisible();
	void			rotate(Quaternion &q);

protected:

	LLEntity(void);
	virtual void	preLoadMesh(const String& meshName);

	// -------------------------------------------------------------------------
	// Common variables
	// -------------------------------------------------------------------------
	unsigned int	m_ID;
	eEntityType		m_eType;

	// -------------------------------------------------------------------------
	// OGRE related variables
	// -------------------------------------------------------------------------
	Entity*			m_pEntity;
	SceneNode*		m_pNode;
	SceneNode*		m_pParentNode;

};

#endif
