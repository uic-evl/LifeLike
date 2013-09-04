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
* Filename: LLFSM.h
* -----------------------------------------------------------------------------
* Notes:    Reference source code (Game Programming Gems)
* -----------------------------------------------------------------------------
*/
#ifndef __LLFSM_H_
#define __LLFSM_H_

using namespace std;

class LLFSMState;

typedef std::map <int, LLFSMState*> iStateMap;
typedef std::map <int, LLFSMState*>::iterator iStateMapIter;
typedef std::map <int, LLFSMState*>::value_type iStateMapVt;

class LLFSM 
{
public:
	LLFSM(const char* filename);
	~LLFSM();

	void		loadFSM(const char* filename);
	void		addState(LLFSMState* pState);
	void		removeState(int nStateId);
	void		setIsCreation(bool fgCreation);
	bool		getIsCreation(void);
	
	LLFSMState*	getCurrentState(void);
	void		setCurrentStateId(int nCStateId);
	int			getCurrentStateId(void);
	int			setEvent(int nEvent);

private:

	int				m_iCurrentStateId;
	iStateMap		m_mapState;
	iStateMapIter	m_miState;
	bool			m_bCreation;
	
};


#endif