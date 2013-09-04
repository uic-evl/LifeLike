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
* Filename: LLAnimationBlender.cpp
* -----------------------------------------------------------------------------
* Notes: Original Source code from OGRE wiki site   
* -----------------------------------------------------------------------------
*/
#include "LLAnimationBlender.h"

//-----------------------------------------------------------------------------
LLAnimationBlender::LLAnimationBlender( Entity *entity ) : mEntity(entity) 
{
}

//-----------------------------------------------------------------------------
LLAnimationBlender::~LLAnimationBlender( )
{
}

//-----------------------------------------------------------------------------
void LLAnimationBlender::init(const String &animation)
{
 mSource = mEntity->getAnimationState( animation );
 mSource->setEnabled(true);
 mSource->setWeight(1);
 mTimeleft = 0;
 mDuration = 1;
 mTarget = 0;
 complete=false;
} 

//-----------------------------------------------------------------------------
void LLAnimationBlender::blend( const String &animation, BlendingTransition transition, Real duration, bool l )
{
 loop=l;
 if( transition == LLAnimationBlender::BlendSwitch )
 {
    if( mSource != 0 )
    mSource->setEnabled(false);
    mSource = mEntity->getAnimationState( animation );
    mSource->setEnabled(true);
    mSource->setWeight(1);
    mSource->setTimePosition(0);
    mTimeleft = 0;
 } 
 else 
 { 
    AnimationState *newTarget = mEntity->getAnimationState( animation );
    if( mTimeleft > 0 )
    {
       // oops, weren't finished yet
       if( newTarget == mTarget )
       {
          // nothing to do! (ignoring duration here)
       }
       else if( newTarget == mSource )
       {
          // going back to the source state, so let's switch
          mSource = mTarget;
          mTarget = newTarget;
          mTimeleft = mDuration - mTimeleft; // i'm ignoring the new duration here
       }
       else
       {
          // ok, newTarget is really new, so either we simply replace the target with this one, or
          // we make the target the new source
          if( mTimeleft < mDuration * 0.5 )
          {
             // simply replace the target with this one
             mTarget->setEnabled(false);
             mTarget->setWeight(0);
          }
          else
          {
             // old target becomes new source
             mSource->setEnabled(false);
             mSource->setWeight(0);
             mSource = mTarget;
          } 
          mTarget = newTarget;
          mTarget->setEnabled(true);
          mTarget->setWeight( 1.0 - mTimeleft / mDuration );
          mTarget->setTimePosition(0);
       }
    }
    else
    {
       // assert( target == 0, "target should be 0 when not blending" )
       // mSource->setEnabled(true);
       // mSource->setWeight(1);
       mTransition = transition;
       mTimeleft = mDuration = duration;
       mTarget = newTarget;
       mTarget->setEnabled(true);
       mTarget->setWeight(0);
       mTarget->setTimePosition(0);
    }
 }
}

//-----------------------------------------------------------------------------
void LLAnimationBlender::addTime( Real time )
{
 if( mSource != 0 )
 {
    if( mTimeleft > 0 )
    {
       mTimeleft -= time;
       if( mTimeleft < 0 )
       {
          // finish blending
          mSource->setEnabled(false);
          mSource->setWeight(0);
          mSource = mTarget;
          mSource->setEnabled(true);
          mSource->setWeight(1);
          mTarget = 0;
       }
       else
       {
          // still blending, advance weights
          mSource->setWeight(mTimeleft / mDuration);
          mTarget->setWeight(1.0 - mTimeleft / mDuration);
          if(mTransition == LLAnimationBlender::BlendWhileAnimating)
             mTarget->addTime(time);
       }
    }
    if (mSource->getTimePosition() >= mSource->getLength())
    {
       complete=true;
    }
    else
    {
      complete=false;
    }
    mSource->addTime(time);
    mSource->setLoop(loop);
 }
}

