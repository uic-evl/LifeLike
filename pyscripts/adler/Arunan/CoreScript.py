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


###############################################################################
# Activity Class Derived from C++
###############################################################################

class PythonActivity( LLActivityBase ):

	def initialize( self ):
		gramid = self.addGrammar("Core_GRAMMAR")
		self.elapsedTime = 0.0
		self.setName("Core Activity")

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
    
    def addList(self, str):
    	# str is colon separated list of speeches
    	tokens = str.split(":")
    	for sp in tokens:
    		self.speech.append(sp)
    		self.pool.append(sp)
    	
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

###############################################################################
# Common utility functin to load scxml fsm file
###############################################################################
import xml.dom.minidom

def parseFSM(caller, filename):
    doc = xml.dom.minidom.parse(filename)
    
    # only need to parse states, transition, and actions
    #caller.initial_state = ?
    #self.initial_state = ?
    initial_state = ""
    count = 0
    gramid = 0
    ruleid = 0
    
    # states
	for e in testdoc.getElementsByTagName("state"):
		
		state = e.getAttribute("id")
		print "state: ", state
		
		# got the first one? this is supposed to be the root node for all activity
		# then, get the transition and register it
		#caller.registerTransition("keyword0:keyword1")
		if state == "root":
			keywords = ""
			caller.registerTransition(keywords)
			continue	
		
		# initial state?
		if count == 0:		# may use specific name for it...
			caller.initial_state = state
			caller.current_state = state
		else:
			# add new state => new grammar
			gname = "%s_%s" % (filename, state)
			gramid = caller.addGrammar(gname)
		
		if count == 0:
			caller.currentGrammarID = gramid
			
		# register gram_name, id map
		caller.grammarIDs = {state:gramid}
		
		
		# retrieve transitions
		for trans in e.getElementsByTagName("transition"):
		
			# listening keywords
			event = trans.getAttribute("event")
			# destination state
			target = trans.getAttribute("target")
			
			print "\tevent: ", event
			print "\t\ttarget: ", trans.getAttribute("target")
			actions = []
			
			# actions
			for a in trans.getElementsByTagName("send"):
			
				# action (i.e. speak)
				action = a.getAttribute("event")
				# parameter (i.e. "I want to speak this!"
				param = a.getAttribute("namelist")
				
				speeches = SpeechPool()
				speeches.addList(param)
				
				# add to list
				actions.append([action, speeches])
				
				# register actions: (action,params)
				print "\t\taction: ", action, " param: ", param
		
			# register transition rule wiht action/params pairs
			rulename = "%s_%s_RULE%i" % (filename, state, count)
			ruleid = caller.addGrammarRule(gramid, rulename, event)
			caller.addTransition(ruleid, state, actions, target)
			
		# increment state counter
		count += 1

    return



