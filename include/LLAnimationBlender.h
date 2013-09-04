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
* Filename: AnimationBlender.h
* -----------------------------------------------------------------------------
* Notes:    Blending two skeletal animaiton sequences
*           Original Source code from OGRE wiki site   
* -----------------------------------------------------------------------------
*/

#ifndef __LLAnimationBlender_H_
#define __LLAnimationBlender_H_

#include <vector>
#include <Ogre.h>

using namespace Ogre;

class LLAnimationBlender
{
public:
  enum BlendingTransition
  {
     BlendSwitch,			// stop source and start dest
     BlendWhileAnimating,   // cross fade, blend source animation out while blending destination animation in
     BlendThenAnimate		// blend source to first frame of dest, when done, start dest anim
  };
  
  LLAnimationBlender( Entity *);
  ~LLAnimationBlender();

  void blend( const String &animation, BlendingTransition transition, Real duration, bool l );
  void addTime( Real );
  Real getProgress() { return mTimeleft/ mDuration; }
  AnimationState *getSource() { return mSource; }
  AnimationState *getTarget() { return mTarget; }
  void init( const String &animation );

private:
  Entity*			mEntity;
  AnimationState*	mSource;
  AnimationState*	mTarget;

  BlendingTransition mTransition;

  Real				mTimeleft;
  Real				mDuration;
  bool				complete;
  bool				loop;
};

#endif
