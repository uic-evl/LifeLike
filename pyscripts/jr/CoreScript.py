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
import Queue
import asyncore
import collections
import logging
import socket

# modules for network communication
from socket import socket, AF_INET, SOCK_STREAM
import threading

# global variable: avatarName
gAvatarName = "SBY"
gAvatarFirstName = ""

# global variable: server information
# change this address port number according to your server
gHost = '127.0.0.1'
gPort =	8000

MAX_MESSAGE_LENGTH = 1024

################################################################################
class RemoteClient(asyncore.dispatcher):
	def __init__(self, host):
		asyncore.dispatcher.__init__(self)
		self.create_socket(AF_INET, SOCK_STREAM)
		self.connect((gHost, gPort))
		self.outbox = collections.deque()
		self.host = host

	def say(self, message):
		self.outbox.append(message)

	def handle_connect(self):
		pass # connection succeeded

	def handle_read(self):
		client_message = self.recv(MAX_MESSAGE_LENGTH)
		self.host.msg_from_DM(client_message)

	def handle_close(self):
		self.close()

	def handle_write(self):
		if not self.outbox:
			return
		message = self.outbox.popleft()
		if len(message) > MAX_MESSAGE_LENGTH:
			raise ValueError('Message too long')
		self.send(message)

################################################################################
class worker(threading.Thread):
	
	# initialize thread
	def __init__(self):
		self._stopevent = threading.Event()
		self.sleepperiod = 0.01
		threading.Thread.__init__(self)
		
	# thread execution function
	def run(self):
		# this loop totally stuck there! 
		asyncore.loop()
				
	# thread join function
	def join(self, timeout=None):
		self._stopevent.set()
		threading.Thread.join(self, timeout)
		
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
		#global gAvatarName, gAvatarFirstName
		#gAvatarName = self.getOwnerName()
		#names = gAvatarName.split()
		#gAvatarFirstName = names[0]
		
		# thread array
		self.threads = []

		# new network comm method using asyncore
		self.remote_clients = RemoteClient(self)
		thread1 = worker()
		thread1.start()
		self.threads.append(thread1)

		self.remote_clients.send("0::DLM::" + gAvatarName + "::RDY\r\n")
		
		
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
	
	# process message from external app via network (i.e. DM)
	# this function called by network receiver theread when it gets new msg
	def msg_from_DM (self, msg):
		# add debug message (can show this on screen F12 key)
		self.addDebug("DM message received:" + msg)
		
		# example message here is "msg::SBY::DLM::SAY::hi there" from DM
		# '::' is delim
		# the first three letters is command and second token string is associated
		# values. in this example, it is string to be spoken
		vars = []
		vars = msg.split('::')
		if not msg.find("Welcome client ") == -1:
			#self.remote_clients.send("0::DLM::" + gAvatarName + "::ACK\r\n")
			return
		if len(vars) !=0:
			if vars[3] == 'SAY':
				if vars[1] == gAvatarName:
					self.speak(vars[4])
					self.remote_clients.send(vars[0] + "::DLM::" + gAvatarName + "::ACK\r\n")
					return
			# if vars[3] == 'LOD':    return
			self.remote_clients.send(vars[0] + "::DLM::" + gAvatarName + "::ACK\r\n")
			return
			# can add more command here

		# improper message length, error
		self.remote_clients.send(vars[0] + "::DLM::" + gAvatarName + "::ERR::Far too short winded, sir!\r\n")
		#self.remote_clients.send(vars[0] + "::DLM::" + gAvatarName + "::ACK\r\n")
		
		
	# this is a message from c++ framwork (i.e. keyboard message)
	def msg_received (self, msg):
		# add debug message
		self.addDebug("Avatar message received: " + msg)
		
		# parse message similar to msg_from_DM
		# we will need to add more message interface in c++ as we develop
		# application protocol
		vars = []
		vars = msg.split('::')
		if len(vars) != 0:
			if vars[0] == 'GUI':		# GUI:RETURN:message
				if vars[1] == "RETURN":
					# send message to server. simply insert msg to output queue
					# self.remote_clients.send("NEW:"+vars[2])
					pass

	# destructor
	def deinitialize(self):
		self.remote_clients.close()
		for c in self.threads:
			c.join()
		
