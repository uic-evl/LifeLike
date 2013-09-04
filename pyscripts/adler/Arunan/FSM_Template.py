from LLActivityPlugin import *
import random


###############################################################################
# Activity Class Derived from C++
###############################################################################

class PythonActivity( LLActivityBase ):

	def initialize( self ):

		# Map (input, current_state) --> (action, next_state)
		# action is state related function assigned to it
		self.state_transitions = {}
		self.exiting = False
		self.active = False
		
		# initial state
		self.initial_state = None
		self.current_state = self.initial_state
		self.action = None
		self.next_state = None
		self.prev_state = None
		
		# let load fsm file: what's name of the file?
		parseFSM(self, "storyteller.xml")
		
		#self.setName("WhoAreYou Activity");
		
	def setActive( self, str):

		if self.grammarIDs.has_key(self.current_state):
			self.setCurrentGrammar(self.grammarIDs[self.current_state])
		if self.action is not None:
			self.action(self, str)
		
		self.active = True

	def processRecognition( self, gid, rid, conf, listened, rulename ):
		
		# gid: current state, rid: FSM input
		# conf: SR confidence, str: recognized string

		(self.action, self.next_state) = self.getTransition(rid, self.current_state)
		if self.action is not None:
			#self.action(self, str)
			#this action from fsm is list of actions...
			for a in self.actions:
				if a[0] == "speak":
					self.speak(a[1].pop())
				else:
					pass
					# do not implement anything else than speak for now

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
			self.reset()
			return -1
		else:
			self.setCurrentGrammar(newgram)
			return 0

	def reset (self):
		
		self.current_state = self.initial_state
		self.prev_state = None
		self.exiting = False
		self.active = False
		self.action = WhoAreYouState0
		self.currentGrammarID = self.grammarIDs[self.initial_state]
		
	def suspend (self):
		
		# disable current SR grammar and get into suspend mode
		if self.grammarIDs.has_key(self.current_state):
			self.setGrammarActive(self.grammarIDs[self.current_state], False)
		
		self.active = False
	
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
			
	def update (self, addedTime):
		
		# this activity just return
		return -1

	def msg_received (self, msg):
		pass

