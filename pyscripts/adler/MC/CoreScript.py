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

from LLActivityPlugin import *
import random
import os

# global variable: avatarName
gAvatarName = ""
gAvatarFirstName = ""
gScriptDir = "../../pyscripts/adler/MC"

###############################################################################
# Activity Class Derived from C++
###############################################################################

class PythonActivity( LLActivityBase ):

	def initialize( self ):
		gramid = self.addGrammar("Core_GRAMMAR")
		self.elapsedTime = 0.0
		self.setName("Core Activity")
		
		global gAvatarName, gAvatarFirstName
		gAvatarName = self.getOwnerName()
		names = gAvatarName.split()
		gAvatarFirstName = names[0]
		
		global gScriptDir
		gScriptDir = self.getScriptDir().replace("\\\\", "/")
		self.addDebug("script directory1: " + gScriptDir)
		
	def setActive( self, str ):
		self.setCurrentGrammar(1)
		self.setTipString("Carina Application", 10.0)
		
	def processRecognition( self, gid, rid, conf, listened, rulename ):
		if rid > 100:
			return rid
		else:
			return 0

	def reset (self):
		print 'reset...'

	def suspend (self):
		# disable current SR grammar and get into suspend mode
		print 'suspending...'

	def resume (self):
		# enable current SR grammar and get back to work
		print 'resuming...'

	def update (self, addedTime):
		#tick function call from C++. Just dummy for core
		self.elapsedTime = addedTime
		
		return 0
	
	def msg_received (self, msg):
		pass
		
	def deinitialize(self):
		pass

###############################################################################
# Common utility class managing speech sentence pool
###############################################################################
class SpeechPool:
    
    def __init__(self):
    	self.speech = []
    	self.pool = []
    
    def add(self, str):
    	self.speech.append(str)
    	self.pool.append(str)
    
    def pop(self):
    	if len(self.speech) == 0:
    		return "error"
    	
        if len(self.speech) == 1:
        	return self.speech[0]
		
        pick = self.pool.pop(random.randint(0, len(self.pool)-1))
        
        if len(self.pool) == 0:
        	for x in self.speech:
        		if x != pick:
        			self.pool.append(x)
        
        return pick

