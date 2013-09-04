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

#from socket import socket, AF_INET, SOCK_STREAM
import socket
import threading


###############################################################################
class mw_server(threading.Thread):
	def __init__(self,(caller)):
		self._stopevent = threading.Event()
		self.sleepperiod = 0.01
		threading.Thread.__init__(self)
		
		self.size = 1024
		self.avatar = caller

		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 
		self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		self.sock.bind(('',3829)) 
		self.sock.listen(5)
		self.sock.settimeout(None)

	def run(self):
		#
		client, address = self.sock.accept()
		
		while not self._stopevent.isSet():
			try:
				msg = client.recv(self.size)
				if msg:
					vars = []
					vars = msg.split(':')
					if len(vars) != 0 and vars[0] == 'SPD':
						self.avatar.stopspeak()
						self.avatar.speak(vars[1])
				else:
					self._stopevent.wait(self.sleepperiod)
			except:
				pass
				
	def join(self, timeout=None):
		self._stopevent.set()
		self.sock.close()
		threading.Thread.join(self, timeout)

###############################################################################
# Activity Class Derived from C++
###############################################################################

class PythonActivity( LLActivityBase ):

	def initialize( self ):
		gramid = self.addGrammar("Core_GRAMMAR")
		self.elapsedTime = 0
		self.threads = []
		thread1 = mw_server((self))
		thread1.start()
		self.threads.append(thread1)

	def setActive( self, str ):
		self.setCurrentGrammar(1)
		
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


