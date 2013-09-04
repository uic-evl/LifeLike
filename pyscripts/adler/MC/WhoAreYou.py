from LLActivityPlugin import *
import random

# state 0 function : IDLE
# who are you activity returns immediately after speak corresponding answer
def WhoAreYouState0(activity, str):
	if str == 'how are you':
		activity.speak(activity.howareyouspeech.pop())
	elif str == 'how are you doing':
		activity.speak(activity.howdoingspeech.pop())
	elif str == 'are you real':
		activity.speak("Well..., no. I'm a virtual tour guide.")
	elif str == 'where are you':
		speech = "<bookmark mark=\"[whiteboard][URL][web/carina/ISS.jpg]\"/>"
		activity.speak(speech + activity.whereareyouspeech.pop())
	elif str == 'I have a question':
		speech = activity.ihaveaquestionspeech.pop()
		activity.speak(speech)
	else:
		activity.speak(activity.whoareyouspeech.pop())
		
	activity.reset()
	activity.exiting = True

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
		self.initial_state = 'IDLE'
		self.current_state = self.initial_state
		self.action = WhoAreYouState0
		self.next_state = None
		self.prev_state = None
		
		# register transition to self from ActivityManager
		# use ':' as delim for multiple recognizable inputs
		self.registerTransition("who are you:what are you:what is your name:how are you:how are you doing:are you real:where are you:I have a question")
		
		# initialize all states and its function
		# step1: add grammar -> add default transition
		# step2: add rule -> add transition with rule id
		#  repeat step2 till all required transitions added
		
		# IDLE state (initial one)
		gramid = self.addGrammar("WhoAreYou_IDLE")
		self.currentGrammarID = gramid
		self.grammarIDs = {'IDLE':gramid}
		ruleid = self.addGrammarRule(gramid, "ACTIVITYA_IDLE_RULE0", "repeat:say it again:what:sorry")
		self.addTransition(ruleid, 'IDLE', WhoAreYouState0, 'IDLE')
		
		global gAvatarName, gAvatarFirstName
		
		# speech pool
		self.whoareyouspeech = SpeechPool()
		self.whoareyouspeech.add("My name is " + gAvatarFirstName + ". I am a LifeLike avatar. designed to interact with people.")
		self.whoareyouspeech.add("I'm " + gAvatarFirstName)
		self.whoareyouspeech.add("I'm " + gAvatarName)
		
		self.howareyouspeech = SpeechPool()
		self.howareyouspeech.add("I'm fine")
		self.howareyouspeech.add("OK")
		self.howareyouspeech.add("Allright. Thanks.")

		self.howdoingspeech = SpeechPool()
		self.howdoingspeech.add("I'm doing well.")
		self.howdoingspeech.add("Doing fine.")
		self.howdoingspeech.add("I am doing OK. Thanks.")

		self.whereareyouspeech = SpeechPool()
		self.whereareyouspeech.add("I'm in the International Space Station.")
		

		self.ihaveaquestionspeech = SpeechPool()
		self.ihaveaquestionspeech.add("OK. What is it?")
		self.ihaveaquestionspeech.add("OK. Please ask me a question.")
		self.ihaveaquestionspeech.add("Alright, go ahead.")


		self.setName("WhoAreYou Activity");
		
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

