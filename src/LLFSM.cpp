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
* Filename: LLFSM.cpp
* -----------------------------------------------------------------------------
* Notes:    Reference source code (Game Programming Gems)
* -----------------------------------------------------------------------------
*/
#include "LLFSMState.h"
#include "LLFSM.h"
#include "tinyxml2.h"

using namespace tinyxml2;

//-----------------------------------------------------------------------------
LLFSM::LLFSM(const char* filename)
{
	loadFSM(filename);
}

//-----------------------------------------------------------------------------
LLFSM::~LLFSM()
{
}

//-----------------------------------------------------------------------------
void LLFSM::loadFSM(const char* filename)
{
	// init xml parser
	XMLDocument	xmlDoc;

	// load xml file
	xmlDoc.LoadFile(filename);
	XMLElement* element = xmlDoc.RootElement();		// FSM node
	
	// Parse DOM
	int nTransitions=0,nStateId=0;
	unsigned int istate, iev, iout;
	LLFSMState* pState = NULL;

	XMLElement* element1 = element->FirstChildElement("state");
	while (element1)
	{
		// state element
		istate = element1->IntAttribute("id");

		// count number of transition element
		nTransitions = element1->countChildren("transition");

		pState = new LLFSMState(istate,0,nTransitions);
		XMLElement* element2 = element1->FirstChildElement("transition");
		while (element2)
		{
			iev = element2->IntAttribute("event");
			iout = element2->IntAttribute("output");
			pState->addTransition( iev , iout );

			element2 = element2->NextSiblingElement("transition");
		}
		
		addState(pState);

		element1 = element1->NextSiblingElement("state");
	}

	setIsCreation(true);
	m_iCurrentStateId = 0;
}

//-----------------------------------------------------------------------------
void LLFSM::addState(LLFSMState* pState)
{
	LLFSMState* pTempState = NULL;
	
	if( !m_mapState.empty() ) {
		m_miState = m_mapState.find(pState->getStateId());
		if( m_miState != m_mapState.end() ) {
			pTempState = (LLFSMState*)((*m_miState).second);
		}
	}

	if (pTempState != NULL) {
		return;
	}
	
	if(m_mapState.empty()) {
		m_iCurrentStateId = pState->getStateId();	
	}
	
	m_mapState.insert( iStateMapVt(pState->getStateId(), pState) );
}

//-----------------------------------------------------------------------------
void LLFSM::removeState(int nStateId)
{
	LLFSMState* pTempState = NULL;
	if( !m_mapState.empty() ) {
		m_miState = m_mapState.find(nStateId);
		if (m_miState != m_mapState.end()) {
			pTempState = (LLFSMState*)((*m_miState).second);
		}
	}
	
	if(pTempState!=NULL && pTempState->getStateId() == nStateId) {
		m_mapState.erase(m_miState);
		delete pTempState;  //check!!
	}
	else {
	}
}

//-----------------------------------------------------------------------------
void LLFSM::setIsCreation(bool bCreation)
{
	m_bCreation = bCreation;
}

//-----------------------------------------------------------------------------
bool LLFSM::getIsCreation(void)
{
	return m_bCreation;
}

//-----------------------------------------------------------------------------
LLFSMState*  LLFSM::getCurrentState(void)
{
	LLFSMState* pTempState = NULL;
 	m_miState = m_mapState.find(m_iCurrentStateId);
	pTempState = (*m_miState).second;	
	return pTempState;

}

//-----------------------------------------------------------------------------
void LLFSM::setCurrentStateId(int nStateId)
{
	m_iCurrentStateId = nStateId;
}

//-----------------------------------------------------------------------------
int LLFSM::getCurrentStateId(void)
{
	return m_iCurrentStateId;
}

//-----------------------------------------------------------------------------
int LLFSM::setEvent(int nEvent)
{
	int nextState;
	LLFSMState* state = getCurrentState();
	nextState = state->getOutputState(nEvent);
	
	if (nextState == -1)
		return -1;

	setCurrentStateId(nextState);
	return nextState;
}
