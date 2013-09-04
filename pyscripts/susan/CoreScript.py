#-*- coding: latin1 _*_

###############################################################################
# Core Activity Class (Root Activity)
# Root Activity only has single state or you may say not state
# Therefore only one grammar (id 1) exist
# This script will be loaded first before any other scripts
# Core Activity only takes care of top level transition to each Activity
# and its SR grammar is always ACTIVE
# Transition rules will be added by other activity scripts and registered 
# after id 100 (the first 100 is for special purpose)
# just long list of stwitch case statement
###############################################################################

# import necessary modules
from LLActivityPlugin import *
import random
import os
import sys
import fileinput

# global variable: avatarName
gAvatarName = ""
gAvatarFirstName = ""

gStory = []

# parse in story
# first (training), second & third
#for line in fileinput.input("../../pyscripts/susan/story_train.txt"):
#for line in fileinput.input("../../pyscripts/susan/story_first.txt"):
for line in fileinput.input("../../pyscripts/susan/story_second.txt"):
	global gStory
	gStory.append(line)

counter = 0
size = len(gStory)

###############################################################################
# Activity Class Derived from C++ LL framework
# this is main part you can implement your logic to talk to Dialog Manager
# and Avatar
###############################################################################
class PythonActivity( LLActivityBase ):
  # 
	def initialize( self ):
		# some sapi grammar stuff
		gramid = self.addGrammar("Core_GRAMMAR")
		self.elapsedTime = 0.0
		self.setName("Core Activity")
		
		# set avatar name: retrieve it from c++ side
		global gAvatarName, gAvatarFirstName
		gAvatarName = self.getOwnerName()
		names = gAvatarName.split()
		gAvatarFirstName = names[0]
		
		self.state = 0
		
	# this function called when this activity become active
	# in fact, this is more useful when use sapi speech recognition within
	# avatar framework. otherwise no to touch it
	def setActive( self, str ):
		self.setCurrentGrammar(1)
	
	# another sapi related function
	# this is called by c++ to pass recognized grammar information
	def processRecognition( self, gid, rid, conf, listened, rulename ):
		if rid > 100:
			return rid
		else:
			return 0

	# reset function call to clear up all sapi related grammar status
	def reset (self):
		print 'reset...'

	# suspend sapi grammar
	def suspend (self):
		# disable current SR grammar and get into suspend mode
		print 'suspending...'

	# resume sapi grammar
	def resume (self):
		# enable current SR grammar and get back to work
		print 'resuming...'

	# tick function called by c++ when each update happens in main frameowork
	# if you need any logic to compute constantly (every time), add it here
	# addedTime is elapsed time since the last update call
	def update (self, addedTime):
		# tick function call from C++.
		self.elapsedTime = addedTime
		
		return 0
	
	# this is a message from c++ framwork (i.e. keyboard message)
	def msg_received (self, msg):
		# add debug message
		#self.addDebug("message received: " + msg)
		
		# parse message similar to msg_from_DM
		# we will need to add more message interface in c++ as we develop
		# application protocol
		vars = []
		vars = msg.split(':')
		
		if len(vars) != 0:
			
			if vars[0] == 'KEY':		# Key event
				
				if vars[1] == 'Space':
					# do something here
					global gStory, counter, size
					if counter == size:
						counter = 0
						
					self.speak(gStory[counter])
					counter = counter + 1
					if counter > size -1:
						counter = 0

			elif vars[0] == 'SPD':
				global gStory, counter, size
				if counter < size:
					self.speak(gStory[counter])
					counter = counter + 1
				else:
					counter = 0
				
	
	# destructor
	def deinitialize(self):
		self.socketobj.close()
		for c in self.threads:
			c.join()
