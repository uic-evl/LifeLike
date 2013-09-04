from LLActivityPlugin import *
import random

# state 0 function
def ActivityAState0(activity, str):
	activity.elapsedIdle = 0
	if str == 'Bored':
		random.seed()
		rnumber = random.randint(1,6)
		if rnumber == 1:
			activity.speak("Hey, are you still there? I am waiting for your order.")
		elif rnumber == 2:
			activity.speak("What are you looking for?")
		elif rnumber == 3:
			activity.speak("Tell me what you want!")
		elif rnumber == 4:
			activity.speak("OK. Take your time.")
		elif rnumber == 5:
			activity.speak("I am ready to help you navigate.")
		elif rnumber == 6:
			activity.speak("Well, there must be something you interesed in.")
	else:
		activity.speak("OK.")
		if 'north' in str:
			activity.requestSendMsg('[1][0][100]')
		elif 'south' in str:
			activity.requestSendMsg('[1][0][-100]')
		elif 'east' in str:
			activity.requestSendMsg('[1][-100][0]')
		elif 'west' in str:
			activity.requestSendMsg('[1][100][0]')
		elif 'zoom in' in str:
			activity.requestSendMsg('[0][-1]')
		elif 'zoom out' in str:
			activity.requestSendMsg('[0][1]')


###############################################################################
# Activity Class Derived from C++
###############################################################################

class PythonActivity( LLActivityBase ):

	def initialize( self ):

		# Map (input, current_state) --> (action, next_state)
		# action is state related function assigned to it
		self.state_transitions = {}
		self.exiting = False
		self.elapsedIdle = 0
		self.active = False
		self.randIdle = random.uniform(5, 20)
		
		# initial state
		self.initial_state = 'IDLE'
		self.current_state = self.initial_state
		self.action = ActivityAState0
		self.next_state = None
		self.prev_state = None
		
		# register transition to self from ActivityManager
		# use ':' as delim for multiple recognizable inputs
		self.registerTransition("navigation:navigation mode:alex, navigation mode")
		
		# initialize all states and its function
		# step1: add grammar -> add default transition
		# step2: add rule -> add transition with rule id
		#  repeat step2 till all required transitions added
		
		# IDLE state (initial one)
		gramid = self.addGrammar("ActivityA_IDLE")
		self.currentGrammarID = gramid
		self.grammarIDs = {'IDLE':gramid}
		ruleid = self.addGrammarRule(gramid, "ACTIVITYA_IDLE_RULE0", "north:go to north")
		self.addTransition(ruleid, 'IDLE', ActivityAState0, 'IDLE')
		ruleid = self.addGrammarRule(gramid, "ACTIVITYA_IDLE_RULE1", "south:go to south")
		self.addTransition(ruleid, 'IDLE', ActivityAState0, 'IDLE')
		ruleid = self.addGrammarRule(gramid, "ACTIVITYA_IDLE_RULE2", "east:go to east")
		self.addTransition(ruleid, 'IDLE', ActivityAState0, 'IDLE')
		ruleid = self.addGrammarRule(gramid, "ACTIVITYA_IDLE_RULE3", "west:go to west")
		self.addTransition(ruleid, 'IDLE', ActivityAState0, 'IDLE')
		ruleid = self.addGrammarRule(gramid, "ACTIVITYA_IDLE_RULE4", "zoom in")
		self.addTransition(ruleid, 'IDLE', ActivityAState0, 'IDLE')
		ruleid = self.addGrammarRule(gramid, "ACTIVITYA_IDLE_RULE5", "zoom out")
		self.addTransition(ruleid, 'IDLE', ActivityAState0, 'IDLE')
		
	def setActive( self ):

		if self.grammarIDs.has_key(self.current_state):
			self.setCurrentGrammar(self.grammarIDs[self.current_state])
		if self.action is not None:
			self.action(self, "setActive")
		
		self.active = True
		self.elapsedIdle = 0

	def processRecognition( self, gid, rid, conf, str ):
		
		# gid: current state, rid: FSM input
		# conf: SR confidence, str: recognized string

		(self.action, self.next_state) = self.getTransition(rid, self.current_state)
		if self.action is not None:
			self.action(self, str)

		# update status
		self.prev_state = self.current_state
		self.current_state = self.next_state
		self.next_state = None
		
		# need to change grammar
		if self.grammarIDs.has_key(self.current_state):
			newgram = self.grammarIDs[self.current_state]
		else:
			newgram = self.currentGrammarID
		
		if self.exiting is True:
			self.exiting = False
			self.setGrammarActive(self.currentGrammarID, False)
			self.active = False
			return -1
		else:
			self.setCurrentGrammar(newgram)
			return 0

	def reset (self):
		
		self.current_state = self.initial_state
		self.prev_state = None
		self.exiting = False
		self.elapsedIdle = 0
		self.active = False
		
	def suspend (self):
		
		# disable current SR grammar and get into suspend mode
		if self.grammarIDs.has_key(self.current_state):
			self.setGrammarActive(self.grammarIDs[self.current_state], False)
		
		self.active = False
		self.elapsedIdle = 0
	
	def resume (self):
		
		# enable current SR grammar and get back to work
		if self.grammarIDs.has_key(self.current_state):		
			self.setGrammarActive(self.grammarIDs[self.current_state], True)

	def addTransition (self, input, state, action, next_state):
	
		self.state_transitions[(input, state)] = (action, next_state)
		

	def getTransition (self, input, state):
		
		if self.state_transitions.has_key((input, state)):
			return self.state_transitions[(input, state)]
		else:
			return (None, state)
			
	def msg_received (self, msg):
		#print "message received: " + msg
		return
		
	def update (self, addedTime):
		
		#tick function call from C++
		if self.active is True:
			listening = self.isListening()
			if listening is True:
				self.elapsedIdle += addedTime
				# check overall idle time (should not accumulate speech time)
				if self.elapsedIdle > self.randIdle:
					# spit out some trash talk
					if self.action is not None:
						self.action(self, 'Bored')
					else:
						self.speak("Hey, anybody there? I get bored. Talk to me something.")
						self.elapsedIdle = 0
					# renew next random time
					random.seed()
					self.randIdle = random.uniform(5, 20)

