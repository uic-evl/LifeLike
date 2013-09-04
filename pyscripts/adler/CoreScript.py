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

from socket import socket, AF_INET, SOCK_STREAM
import threading
import Queue
import time

###############################################################################
# Network msg queue
###############################################################################
inputs  = Queue.Queue(0)
outputs = Queue.Queue(0)

###############################################################################
# Network msg sender (thread)
###############################################################################
class receiver(threading.Thread):
	def __init__(self,(caller, sock,receivedQ)):
		self._stopevent = threading.Event()
		self.sleepperiod = 0.2
		threading.Thread.__init__(self)
		
		self.sockobj = sock
		self.que = receivedQ
		self.size = 1024
		self.client = caller

	def run(self):
		#
		while not self._stopevent.isSet():
			try:
				msg = self.sockobj.recv(1024)
				if msg:
					self.que.put(msg)
					self.client.msg_received(msg)
				else:
					self._stopevent.wait(self.sleepperiod)
			except:
				pass
				
	def join(self, timeout=None):
		self._stopevent.set()
		threading.Thread.join(self, timeout)

###############################################################################
# Network msg sender (thread)
###############################################################################
class sender(threading.Thread):
	def __init__(self,(caller, sock,sendQ)):
		self._stopevent = threading.Event()
		self.sleepperiod = 0.2
		threading.Thread.__init__(self)
		
		self.sockobj = sock
		self.que = sendQ
		self.size = 1024
		self.client = caller

	def run(self):
		while not self._stopevent.isSet():
			if not self.que.empty():
				msg = self.que.get()
				#print "message sending thread: " + msg
				try:
					self.sockobj.send(msg)
					self.que.task_done()
				except:
					pass
			else:
				self._stopevent.wait(self.sleepperiod)
				
	def join(self, timeout=None):
		self._stopevent.set()
		threading.Thread.join(self, timeout)

###############################################################################
# Network manager
###############################################################################
class Client:
	def __init__(self):
		#self.host = '131.193.77.166'
		#self.host = 'localhost'
		self.host = '131.193.77.190'
		self.port = 7110
		self.size = 1024
		self.threads = []
		self.socketobj = None

	def open_socket(self):
		self.socketobj = socket(AF_INET, SOCK_STREAM)
		self.socketobj.settimeout(2)
		self.socketobj.connect((self.host, self.port))

	def msg_received(self, msg):
		#print "Client msg_received() called: " + msg
		return
		
	def start(self):
		# open socket
		self.open_socket()
    
		# create thread object
		thread1 = receiver((self, self.socketobj, inputs))
		thread1.start()
		self.threads.append(thread1)
		thread2 = sender((self, self.socketobj, outputs))
		thread2.start()
		self.threads.append(thread2)
	
	def close(self):
		# close all threads
		self.socketobj.close()
		for c in self.threads:
			c.join()

###############################################################################
# Activity Class Derived from C++
# Adler script includes network module for magic carpet
###############################################################################
class PythonActivity( LLActivityBase ):

	def initialize( self ):
		gramid = self.addGrammar("Core_GRAMMAR")
		self.elapsedTime = 0
		self.client = Client()
		self.client.start()

	def deinitialize( self):
		# do some clean up stuff
		self.client.close()
		
	def setActive( self ):
		self.setCurrentGrammar(1)

	def processRecognition( self, gid, rid, conf, str ):
		if rid > 100:
			return rid
		else:
			return 0

	def reset (self):
		#print 'reset...'
		return
		
	def suspend (self):
		# disable current SR grammar and get into suspend mode
		#print 'suspending...'
		return
		
	def resume (self):
		# enable current SR grammar and get back to work
		#print 'resuming...'
		return
		
	def msg_received (self, msg):
		#print "message received: " + msg
		# need to parse message here
		# this method only called by boost C++ side broadcast interface
		#if self.elapsedTime > 10:
		#	self.speak(msg)
		#	self.elapsedTime = 0
		return
	
	def sendMsg (self, msg):
		#print "enqueue outgoing msg: " + msg
		# enqueue msg
		# this method can be called by other activity via boost binding
		# only this core activity can send msg to network
		outputs.put(msg)
		return
		
	def update (self, addedTime):
		#tick function call from C++. Just dummy for core
		self.elapsedTime += addedTime
		
		# check incoming msg queue if there is new msg arrived
		if not inputs.empty():
			msg = inputs.get()
			inputs.task_done()
			self.broadcastMsg(msg)

