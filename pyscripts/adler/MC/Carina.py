from LLActivityPlugin import *
import random
from math import *
import sys

from socket import socket, AF_INET, SOCK_STREAM
import threading
import Queue
import time
import wx
import os

################################################################################
# Some Global Variables
################################################################################
# dataset define: (x res, y res, scale, screen trans x, screen trans y, thumbnail, tourfile, name, wbimage, descrption)
# note: trans y affected by magicarpet dataset configuratin (mirror like)
#       current dataset use horizontal mirror(y translation * -1)
# read from configuration
gDataSets = []	# [name, sizeX, sizeY, scale, transX, transY, sim_image, wb_image, tourfile, descrption]
gNumDataSet = 0
gHost = '131.193.77.190'
gPort =	7110
gScreenX = 1920.0
gScreenY = 1080.0
gMarkerTimer = 1000  # marker msg [104][Center][0][0.0][0.0][0.15][10000]  [msgid][caption str][setid][x][y][radius][duration_tick]


gDataRect = []
gBBox = [0.0, 0.0, 0.0, 0.0]  		# centerX, centerY, width, height for the first dataset
gCamera = [0.0, 0.0, 1.0]			# camera locx, locy, zoom
gPointer = [0.0, 0.0, 0.0, 0.0]
gZoomStep = 0.5						# for relative zoom msg: multiply this value with current zoom level
gPanStep = gScreenX * 0.4			# for relative pan msg: use RE_ZOOM msg with this amount of pixel values
gBBMSG = ""
gConfigPath = ""					# configuration file path (should be scrpt_path/data/)

################################################################################
# Some Global Variables
################################################################################
# image pixel value when zoom is 1.0, offset x, offset y, ratiox, rationy compared to the first set
gImagePixel = []
gInitialzed = False					# initialized bbox
gCalibrating = False
gCalibrated = False
gZoomRate = 1.0
gPanRate = 1.0
gFPS = 30.0

################################################################################
# whiteboard related urls
################################################################################
# bookmarks
joystickmark = "<bookmark mark=\"[whiteboard][URL][web/carina/joystick.jpg]\"/>"
helmet0mark = "<bookmark mark=\"[ACTION][HELMET][0]\"/>"
helmet1mark = "<bookmark mark=\"[ACTION][HELMET][1]\"/>"

# url
titleurl = "title.jpg"


################################################################################
# Tour Parser for simplifed flat file
################################################################################
gGotCoord = False
gTempX = 0.0
gTempY = 0.0
gTempZ = 0.0
gTempRadius = 0.0
gTempZFactor = 1.0	# for each dataset (use to make zoom level to show whole picture)
gTempZFactorX = 1.0	# for each dataset (use to make zoom level to show max width regardless of height)

# some consideration here.
# upon configuration... zoom factor may not work properly in previous script
# so, let's assume that all zoom factor is based on relative to a given dataset
# zoom 1.0 means show full image of this set
# zoom 2.0 means twice zoom in from the fullsize image
# parse tag
def parseTag(tag):
	# what type of tag it is?
	tokens = tag.split()
	newTag = ""
	global gGotCoord, gTempX, gTempY, gTempZ, gTempZFactor, gTempRadius
	if tokens[0] == 'nav':
		# nav x y zoom
		tzoom = float(tokens[3]) * gTempZFactorX
		newTag = "<bookmark mark=\"[ACTIVITY][BRD][NAV:%s:%s:%f]\"/>" % (tokens[1], tokens[2], tzoom)
		if gGotCoord is False:
			gTempX = float(tokens[1])
			gTempY = float(tokens[2])
			gTempZ = tzoom
			gTempRadius = 1.0 / float(tokens[3])
			gGotCoord = True
	elif tokens[0] == 'move':
		# mov x y
		newTag = "<bookmark mark=\"[ACTIVITY][BRD][MOV:%s:%s:1.0]\"/>" % (tokens[1], tokens[2])
	elif tokens[0] == 'zoom':
		# zoom value
		tzoom = float(tokens[1])*gTempZFactorX
		newTag = "<bookmark mark=\"[ACTIVITY][BRD][ZOM:%f:1.0]\"/>" % (tzoom)
	elif tokens[0] == 'pause':
		# pause secs
		newTag = "<silence msec=\"%d\"/>" % (int(float(tokens[1]) * 1000))
	elif tokens[0] == 'spell/':
		newTag = "<spell/>"
	elif tokens[0] == 'spell':
		newTag = "<speel>"
	else:
		# image.jpeg
		newTag = "<bookmark mark=\"[whiteboard][URL][web/carina/%s]\"/>" % (tag)
		
	return newTag

# write one tour entry
def buildEntry(name, contents):

	newLine = ""
	loc = 0
	contents = contents.strip()
	size = len(contents)
	global gGotCoord
	gGotCoord = False
	while loc < size:
		# find tag
		if contents[loc] == '<':
			tag = ""
			loc += 1
			while contents[loc] is not '>':
				tag += contents[loc]
				loc += 1
			# convert tag and add it
			newLine += parseTag(tag)
		else:
			newLine += contents[loc]
		loc += 1
	
	return newLine

def parseTourFile(filename):
	
	# read file
	try:
		fileIN = open(filename, "r")
	except IOError:
		print "The file does not exist"
		return []

	line = fileIN.readline()
	tourSpots = []		# [ [name, spotid, x, y, abzolute_zoomvalue, speech_contents, radius, type] ]
	spotID = 0
	type = ""
	
	while line:
		
		# check comment symbol first
		if line.startswith("#"):
			line = fileIN.readline()
			continue
			
		# strip off whitespace at the beginning and ending
		line = line.strip()
		
		# does line is empty
		if not line.strip():
			line = fileIN.readline()
			continue
		
		# does line start with spot name?
		if line.startswith("name:"):
			# now we parse content
			name = line.strip()
			if len(name) == 5:
				name = "none"
			else:
				name = line[5:]
			contents = fileIN.readline()
			# has type?
			if contents.startswith("type:"):
				type = contents[5:].strip()
				contents = fileIN.readline()
			else:
				# old stype format. check name
				if name is "none":
					type = "tour"
				else:
					type = "both"
					
			speech = buildEntry(name, contents)
			global gTempX, gTempY, gTempZ, gTempRadius
			tourSpots.append( [name, spotID, gTempX, gTempY, gTempZ, speech, gTempRadius, type] )
			spotID += 1
	
		# read next line
		line = fileIN.readline()
	
	fileIN.close()
	return tourSpots

################################################################################
# Load configuration file
################################################################################
def loadConfiguration(filename):
	# read file
	fileIN = open(filename, "r")
	line = fileIN.readline()
	global gNumDataSet, gHost, gDataSets, gDataRect
	lDataSets = []
	
	while line:
		# check comment symbol first
		if line.startswith("#"):
			line = fileIN.readline()
			continue
			
		# strip off whitespace at the beginning and ending
		line = line.strip()
		
		# does line is empty
		if not line.strip():
			line = fileIN.readline()
			continue

		# Server section first
		if line.startswith("Server"):
			# address
			line = fileIN.readline()		# Address xxx.xxx.xxx.xxx
			tokens = line.split()
			gHost = tokens[1]
			# mc resolution
			line = fileIN.readline()		# Resolution xxx yyy
			tokens = line.split()
			global gScreenX, gScreenY
			gScreenX = float(tokens[1])
			gScreenY = float(tokens[2])
		# DataSet
		elif line.startswith("Data"):
			# name on disk
			line = fileIN.readline()
			tokens = line.split()
			nameondisk = tokens[1]
			# name: need to replace '_' with whitespace
			line = fileIN.readline()		# name
			tokens = line.split()
			name = tokens[1].replace("_", " ")
			# resolution
			line = fileIN.readline()		# Size xx yy
			tokens = line.split()
			resx = float(tokens[1])
			resy = float(tokens[2])
			# scale
			line = fileIN.readline()		# Scale s
			tokens = line.split()
			scale = float(tokens[1])
			# offset
			line = fileIN.readline()		# Translate x y
			tokens = line.split()
			offsetx = float(tokens[1])
			offsety = -float(tokens[2])
			# simulator image
			line = fileIN.readline()		# Simulator_Image image
			tokens = line.split()
			image = tokens[1]
			# whiteboard image
			line = fileIN.readline()		# Whiteboard_Image image
			tokens = line.split()
			wbimage = tokens[1]

			# tour file
			line = fileIN.readline()		# tour tour_file
			tokens = line.split()
			#tourfile = tokens[1]
			tourfile = gConfigPath + tokens[1]
			
			# descrption
			line =  fileIN.readline()		# about this image
			desc = buildEntry("name", line)
			
			# add to dataset
			lDataSets.append([resx, resy, scale, offsetx, offsety, image, tourfile, name, wbimage, desc, nameondisk])
			gNumDataSet += 1
			gDataRect.append( wx.Rect(10, 10, 100, 100) )

		# read next line
		line = fileIN.readline()

	fileIN.close()
	
	# copy sorted list...
	gDataSets = sorted(lDataSets, key=lambda dataset: dataset[10])


################################################################################
# Network msg receiver (thread)
################################################################################
# network msg outgoing queue
outputs = Queue.Queue(0)
class receiver(threading.Thread):
	def __init__(self,(caller, sock)):
		self._stopevent = threading.Event()
		self.sleepperiod = 0.01
		threading.Thread.__init__(self)
		
		self.sockobj = sock
		self.size = 1024
		self.client = caller
		self.initialized = gInitialzed
		
	def run(self):
		#
		while not self._stopevent.isSet():
			try:
				msg = self.sockobj.recv(1024)
				if msg:
					#self.client.navigator.owner.addDebug(msg)
					vars = []
					vars = msg.split()
					#if len(vars) != 0:
					if len(vars) != 0 and vars[0] == '27':
						x0 = float(vars[1])
						x1 = float(vars[2])
						y0 = float(vars[3])
						y1 = float(vars[4].strip('\00'))
						
						p0 = 0.0
						p1 = 0.0
						
						if len(vars) > 6:
							p0 = float(vars[5])
							p1 = float(vars[6].strip('\00'))
						
						global gCamera, gImagePixel, gDataSets, gDataRect, gPointer
						# compute zoom value
						gCamera[2] = (gBBox[1] - gBBox[0]) / (x1 - x0)

						# the first rect: the first dataset representation
						x = -x0 * gImagePixel[0][0] * gCamera[2]
						y = -y0 * gImagePixel[0][1] * gCamera[2]
						w = gCamera[2] * gImagePixel[0][0]
						h = gCamera[2] * gImagePixel[0][1]
						gDataRect[0].Set(x, y, w, h)
						
						# if more than one set used? find origin first
						ox = x + w * 0.5 - gDataSets[0][3] * 0.5 * gCamera[2]
						oy = y + h * 0.5 - gDataSets[0][4] * 0.5 * gCamera[2]
						for n in range(1, gNumDataSet):
							# origin?
							ww = w * gImagePixel[n][4]
							hh = h * gImagePixel[n][5]
							xx = ox + (gDataSets[n][3]) * 0.5 * gCamera[2] - ww * 0.5
							yy = oy + (gDataSets[n][4]) * 0.5 * gCamera[2] - hh * 0.5
							gDataRect[n].Set(xx, yy, ww, hh)
							
						# compute camera location
						pt = [gScreenX * 0.5, gScreenY * 0.5]
						rx = float(gDataRect[0].GetX())
						ry = float(gDataRect[0].GetY())
						gCamera[0] = (pt[0] - rx) / gDataRect[0].GetWidth() - 0.5 + gDataSets[0][3]*gCamera[2] / (gImagePixel[0][0]*2.0)
						gCamera[1] = (pt[1] - ry) / gDataRect[0].GetHeight() - 0.5 + gDataSets[0][4]*gCamera[2] / (gImagePixel[0][1]*2.0)
						
						global gBBMSG
						gBBMSG = msg
				        
						pt[0] = (p0 + gScreenX)* 0.5
						pt[1] = (-p1 + gScreenY)* 0.5
						gPointer[2] = pt[0]
						gPointer[3] = pt[1]
						gPointer[0] = (pt[0] - rx) / gDataRect[0].GetWidth() - 0.5 + gDataSets[0][3]*gCamera[2] / (gImagePixel[0][0]*2.0)
						gPointer[1] = (pt[1] - ry) / gDataRect[0].GetHeight() - 0.5 + gDataSets[0][4]*gCamera[2] / (gImagePixel[0][1]*2.0)
						
						#dmsg = "cam (%f,  %f) pointer (%f, %f)" % (gCamera[0], gCamera[1], gPointer[0], gPointer[1])
						#dmsg = "cam (%f,  %f) pointer (%f, %f) rxy (%f, %f)" % (gCamera[0], gCamera[1], p0, p1)
						#self.client.navigator.owner.addDebug(dmsg)
						
					if msg.strip('\00') == '105':		# microphone off command by joystick button 3 up
						self.client.navigator.owner.unmuteMic()
					if msg.strip('\00') == '106':		# microphone off command by joystick button 3 up
						self.client.navigator.owner.muteMic()

				else:
					self._stopevent.wait(self.sleepperiod)
			except:
				pass
				
	def join(self, timeout=None):
		self._stopevent.set()
		threading.Thread.join(self, timeout)

################################################################################
# Network msg sender (thread)
################################################################################
class sender(threading.Thread):
	def __init__(self,(caller, sock,sendQ)):
		self._stopevent = threading.Event()
		self.sleepperiod = 0.01
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

################################################################################
# Network Client class
################################################################################
class Client:
	def __init__(self, navigator):
		self.host = gHost
		self.port = gPort
		self.size = 1024
		self.threads = []
		self.socketobj = None
		self.navigator = navigator
		self.start()
		
	def open_socket(self):
		self.socketobj = socket(AF_INET, SOCK_STREAM)
		self.socketobj.settimeout(2)
		self.socketobj.connect_ex((self.host, self.port))
		
	def start(self):
		# open socket
		self.open_socket()
		
		# create thread object
		thread1 = receiver((self, self.socketobj))
		thread1.start()
		self.threads.append(thread1)
		thread2 = sender((self, self.socketobj, outputs))
		thread2.start()
		self.threads.append(thread2)
		
	def deinitialize(self):
		# close all threads
		self.socketobj.close()
		for c in self.threads:
			c.join()
	

################################################################################
# translate image space coordinate to proper screen pixel translation value
# x: -0.5 ~ 0.5, y: -0.5 ~ 0.5 (within image area)
################################################################################
def loc2pixel(setid, x, y):
	global gCamera, gImagePixel, gDataSets
	pv = [0.0, 0.0]
	# do not use zoom level (gCamera[2]) here. Just use absolution value @ zoom 1.0
	# then, mc will take care of it
	pv[0] = -gImagePixel[setid][0] * x * 2.0 - gDataSets[setid][3]
	pv[1] = -gImagePixel[setid][1] * y * 2.0 - gDataSets[setid][4]
	return pv

################################################################################
# Navigator Class
################################################################################
class MCNavigator:
	def __init__(self, activity):

		self.mode = 0	# 0: stop, 1: tour 2: suspended
		self.current = [0.0, 0.0, 1.0]
		self.owner = activity
		self.targetid = -1
		self.currentSet = 0
		self.currentROI = -1
		self.bbtimer = 0
		
		# calculate initial dataset coordinate (dataset 0)
		global gDataSets, gImagePixel, gBBox, gNumDataSet
		for n in range(gNumDataSet):
			# screen space pixel values
			w = gDataSets[n][0] * gDataSets[n][2] * 0.5		# width @ zoom 1.0 and gScreen resolution
			h = gDataSets[n][1] * gDataSets[n][2] * 0.5		# height @ zoom 1.0 and gScreen resolution
			x = gDataSets[n][3] * 0.5						# center x of dataset 0
			y = gDataSets[n][4] * 0.5						# center y of dataset 0
		
			# set original bbox for it: used to calculate zoom level
			l = -(x - w*0.5) / w
			r = (gScreenX - (x - w*0.5)) /  w
			t = -(y - h*0.5) / h
			b = (gScreenY - (y - h*0.5)) /  h
			rw = (gDataSets[n][0]*gDataSets[n][2]) / (gDataSets[0][0]*gDataSets[0][2])
			rh = (gDataSets[n][1]*gDataSets[n][2]) / (gDataSets[0][1]*gDataSets[0][2])
			# only set gBBox if this is the first dataset
			if n == 0:
				gBBox = [l, r, t, b]
				gImagePixel.append([w, h, x, y, 1.0, 1.0])
			else:
				gImagePixel.append([w, h, x, y, rw, rh])

		# auto tour sequences: 
		# list of [name, spotid, x, y, radius, speech_contents] per set
		# load tour informatio from file
		self.tourSet = []
		self.ROI = []
		tset = []
		for n in range(0, gNumDataSet):
			# need to set dataset zoomfactor : gTempZFactor
			# 1.0 * gTempZFactor => should show whole picture of this set
			global gTempZFactor, gTempZFactorX
			zx = (gScreenX / gImagePixel[n][0])
			zy = (gScreenY / gImagePixel[n][1])
			gTempZFactor = min(zx, zy)
			gTempZFactorX = zx
			tset = parseTourFile(gDataSets[n][6])
			tourset = []
			roiset = []
			
			for tspot in tset:
				dmsg = "adding tour spot type: %s" % (tspot[7])
				self.owner.addDebug(dmsg)
				if tspot[7] == 'tour':
					tourset.append(tspot)
				elif tspot[7] == 'roi':
					roiset.append([ tspot[0], tspot[2], tspot[3], tspot[6], tspot[5], tspot[4] ])
				elif tspot[7] == 'both':
					tourset.append(tspot)
					roiset.append([ tspot[0], tspot[2], tspot[3], tspot[6], tspot[5], tspot[4] ])
				else:
					# this is "none" case?
					dmsg = "adding tour spot type error: %s" % (tspot[7])
					self.owner.addDebug(dmsg)
					pass
			
			self.tourSet.append( tourset )
			self.ROI.append(roiset)
		
		# temp storage for tour: list of [name, id, x, y, zoom, speech]
		self.tournodes = []
		self.lastvisited = 0
		self.currentvisiting = 0
		self.moving = False
		self.zooming = False
		self.navigating = False
		self.nearest = -1
		self.pilotdecay = 0.0
		self.pilots = []		# three tuple set [type, value1, value2] [zoom, 1.5, 0.0] [move, x, y]
		
	def zoomInt(self, z, int):
		if z < 0.0:
			return
		
		# this only use for absolute zoom level
		msg = "[102][%f][%i]" % (z, int)
		outputs.put(msg)

		dmsg = "zoomInt: %f, %i" % (z, int)
		self.owner.addDebug(dmsg)
		
		# how long would this zoom take?
		self.pilotdecay = (int / gZoomRate ) * 1.2
		self.navigating = True
		
	def zoom(self, z, absolute = True):
		
		# range check: only take positive values
		if z < 0:
			return
		
		zoomlevel = z
		int = 1.5		# just use fixed interpolation time
		
		if absolute == False:
			# relative value comes
			zoomlevel = gCamera[2] * z
		
		# how long this will take?
		self.pilotdecay = int * 1.2
		
		# send network zoom msg
		msg = "[102][%f][%i]" % (zoomlevel, int * gZoomRate)
		outputs.put(msg)
		self.navigating = True
	
		dmsg = "zoom: %f" % (z)
		self.owner.addDebug(dmsg)

	def moveInt(self, x, y, int):
		
		coord = loc2pixel(self.currentSet, x, y)
		msg = "[101][%f][%f][%i]" % (coord[0], coord[1], int)
		outputs.put(msg)
		self.pilotdecay = (int / gZoomRate ) * 1.2
		self.navigating = True

	def move(self, x, y, absolute = True):
		
		if absolute == True:
			# x,y coordinate parameter is image space absolution value
			# range from (-0.5, -0.5) to (0.5, 0.5)
		
			xx = gCamera[0] - x
			yy = gCamera[1] - y
			dd = sqrt(pow(xx,2)+pow(yy,2))	# distance - img range -0.5 to 0.5
		
			# calculate proper interpolation value
			# based on screen space navigate distance, set the interpolator
			#interpolator = dd * gCamera[2] * gInterpolatorPan
			self.pilotdecay = 2.5 * 1.2
			
			# send network move msg
			# the coordinate is screen space absolution pixel value
			coord = loc2pixel(self.currentSet, x, y)
			msg = "[101][%f][%f][%i]" % (coord[0], coord[1], 2.5 * gPanRate)
			outputs.put(msg)
			self.navigating = True
			
		else:
			# relative move: parameters are screenspace pixel value
			coord = loc2pixel(self.currentSet, gCamera[0], gCamera[1])
			msg = "[101][%f][%f][%i]" % (coord[0]+x, coord[1]+y, 2.5 * gPanRate)
			outputs.put(msg)
			
		
	def navigateTo(self, x, y, z):
		# x,y coordinate parameter is image space absolution value
		# range from (-0.5, -0.5) to (0.5, 0.5)
		# z is zoom level
		
		# need some fancy effect...
		# basic movement: zoom -> pan -> zoom
		# zoom0: level 1.5
		# pan: to the destination
		# zoom1: level specified in ROI based on radius...
		
		# enqueue: zoom entity
		self.pilots.append([0, z, 0.0])
		
		self.move(x,y)
	
	def navigateToROI(self, setid, id):
		# navigate to the ROI[id]
		self.currentSet = setid
		
		# computer proper zoom level from radius of ROI
		# [name, x, y, radius, speech]
		#z = 0.5 / (self.ROI[setid][id][3])
		z = (self.ROI[setid][id][5]) * 0.5			# check radius!!!
		self.navigateTo(self.ROI[setid][id][1], self.ROI[setid][id][2], z)
	
	def navigateToTarget(self):
		if self.targetid != -1:
			self.navigateToROI(self.currentSet, self.targetid)
			self.targetid = -1
	
	def navigateToRecentSet(self):
		global gImagePixel, gScreenX, gScreenY
		# which size of image is longer?
		zx = (gScreenX / gImagePixel[self.currentSet][0])
		zy = (gScreenY / gImagePixel[self.currentSet][1])
		z = min(zx, zy)
		
		# what if current set does not have roi entry? => in fact, no need to worry about this
		# just get to the center of recent dataset!
		self.navigateTo(0, 0, z)
		"""
		rlist = self.getROIList(self.currentSet)
		if rlist is "":
			self.navigateTo(0, 0, z)
		else:
			self.navigateTo(self.ROI[self.currentSet][0][1], self.ROI[self.currentSet][0][2], z)
		"""
		
		return self.currentSet
		
	def getCoordinate(self):
		# this function returns image relative coordinate of center of screen
		# when user looking at a point within image, will return its coordinate
		pt = [0.0, 0.0]
		global gCamera, gDataRect, gScreenX, gScreenY, gDataSets
		if self.currentSet != -1:
			rx = float(gDataRect[self.currentSet].GetX())
			ry = float(gDataRect[self.currentSet].GetY())
			pt[0] = (gScreenX * 0.5 - rx) / gDataRect[self.currentSet].GetWidth() - 0.5
			pt[1] = (gScreenY * 0.5 - ry) / gDataRect[self.currentSet].GetHeight() - 0.5
			
		return pt
		
	def searchDataset(self):
		# exception. if touring, just return current set
		#if self.mode == 1:
		#	return self.currentSet
		
		# which dataset is focused : based on the center of screen
		# AABB test through existing sets. if none hit, then return -1
		# gCamera with dataset
		global gScreenX, gScreenY, gDataRect
		#pt = [gScreenX * 0.5, gScreenY * 0.5]
		pt = [gPointer[2], gPointer[3]]
		
		for n in range(gNumDataSet):
			if gDataRect[n].Inside(pt):
				self.currentSet = n
				return n

		return -1
		
	def searchInterests(self):
		# what am I looking at? what is this? kind of questions
		# use current location to search ROI
		found = [-1, -1, 'nothing', -1, 'speech']	# [setid, id, name, nearid, speech]
		
		id = -1
		mindist = 10000
		near = -1
		
		# check dataset first
		setid = self.searchDataset()
		if setid == -1:
			return found
		
		found[0] = setid
		
		global gScreenX, gScreenY, gDataRect
		
		# relative coordinate within dataset
		Loc = [0.0, 0.0]
		rpt = gDataRect[setid].Get()
		Loc[0] = (gPointer[2] - rpt[0]) / rpt[2] - 0.5
		Loc[1] = (gPointer[3] - rpt[1]) / rpt[3] - 0.5
		
		# check ROI within focused dataset
		# [name, x, y, radius, speech]
		for spot in self.ROI[setid]:
			id += 1
			xx = Loc[0] - spot[1]
			yy = Loc[1] - spot[2]
			dd = sqrt(pow(xx,2)+pow(yy,2))
			
			# update nearest roi spot
			if dd < mindist:
				mindist = dd
				near = id
			
			# distance check: within radius?
			#if dd < spot[3]:
			if dd < spot[3]*0.6:
				found = [setid, id, spot[0], dd, spot[4]]
				self.currentROI = id
				dmsg = "found roi id: %i, radius: %f, dist: %f, name: %s" % (id, spot[3], dd, spot[0])
				self.owner.addDebug(dmsg)
				return found
		
		# nothing interesting in current location
		# then, return nearest one from the current location
		# [setid, id=-1, name, nearid]
		if near is not -1:
			found[0] = setid
			found[2] = self.ROI[setid][near][0]
			found[3] = near
			self.targetid = near
		
		return found
	
	# single speech for one roi	
	def getROIDescription(self, setid, id):
		desc = self.ROI[setid][id][4]
		return desc
	
	# this is used to generate list of roi for speech 	
	def getROIList(self, setid):
		list = ""
		for r in self.ROI[setid]:
			if len(r) is not 0:
				list += r[0]
				list += ", "
		return list
		
	def getROINames(self, setid):
		names = []
		id = -1
		for r in self.ROI[setid]:
			id += 1
			names.append([r[0], id])
		return names

	# this is used to generate list of roi for whiteboard 	
	def getROIWBList(self, setid):
		list = "<p>Interesting Features</p>"
		for r in self.ROI[setid]:
			if len(r) is not 0:
				list += "<p id=\"c\">"
				list += r[0]
				list += "</p>"
		return list
	
	def reset(self):
		# reset all internal vars without changing view
		if len(self.pilots) != 0:
			self.pilots = []
		self.targetid = -1
		self.lastvisited = 0
		self.currentvisiting = 0
		self.moving = False
		self.zooming = False
		self.navigating = False
		self.nearest = -1
		self.pilotdecay = 0.0

	def resetView(self):
		msg = "[20]"
		outputs.put(msg)
		
		# pilot entities
		if len(self.pilots) != 0:
			self.pilots = []
	
	def startTour(self):
		# find next destination: start with closest one?
		
		# current dataset?
		dataset = self.searchDataset()
		if dataset == -1:
			dataset = self.currentSet
		else:
			self.currentSet = dataset

		self.mode = 1
		
		# initial auto pilot mode
		# empty existing navigation entries
		self.navigating = False
		self.pilotdecay = 0.0
		del self.pilots[:]
		del self.tournodes[:]
		# add tour stops
		for stop in self.tourSet[dataset]:
			self.tournodes.append(stop)
		
	def pauseTour(self):
		self.mode = 2
		# well, it's good to recover current tour node
		remain = len(self.tournodes)
		originalsize = len(self.tourSet[self.currentSet])
		del self.pilots[:]
		del self.tournodes[:]
		for stop in self.tourSet[self.currentSet]:
			self.tournodes.append(stop)
		
		for n in range(originalsize - remain -1):
			self.tournodes.pop(0)

	def resumeTour(self):
		self.mode = 1
		# set the current visiting again
		
	def stopTour(self):
		self.mode = 0
		self.navigating = False
		self.pilotdecay = 0.0
		del self.pilots[:]
		del self.tournodes[:]
		
	def nextTour(self, forced = False):

		# not in tour mode
		if self.mode != 1:
			return
		
		if len(self.tournodes) == 0:
			#return
			self.mode = 0
			self.owner.tourEnded()
			
			if forced is True:
				self.owner.speak("there is no more interesting spot available so we end tour. feel free to ask questions")
				
			return
			
		# take the next tour stop
		del self.pilots[:]
		stop = self.tournodes.pop(0)	# [name, id, x, y, abs_zoom, speech, radius]
		self.owner.speaklisten(stop[5])
		
		# show visual marker here. (skip "none" case. typically the fisrt one)
		if stop[0] != 'none':
			msg = "[104][%s][%i][%f][%f][%f][%i]" % (stop[0], self.currentSet, stop[2]*2.0, -stop[3]*2.0, stop[6], gMarkerTimer)
			outputs.put(msg)

	def previousTour(self):
		
		# not in tour mode
		if self.mode != 1:
			return
		
		size0 = len(self.tourSet[self.currentSet])
		size1 = len(self.tournodes)
		
		# this is the first one. no previous
		if (size0 - size1) < 2:
			# should say. this is the first one. and keep the tour...
			return
		
		# rewind tour spot
		del self.pilots[:]
		prev_stop = self.tourSet[self.currentSet][size0 - size1 - 2]
		curr_stop = self.tourSet[self.currentSet][size0 - size1 - 1]
		
		self.tournodes.insert(0, curr_stop)
		
		self.owner.speaklisten(prev_stop[5])
		
		if prev_stop[0] != 'none':
			msg = "[104][%s][%i][%f][%f][%f][%i]" % (prev_stop[0], self.currentSet, prev_stop[2]*2.0, -prev_stop[3]*2.0, prev_stop[6], gMarkerTimer)
			outputs.put(msg)
		
	
	def update(self, addedTime):
		# update current location
		if self.mode == 0:
			self.current = [gCamera[0], gCamera[1], gCamera[2]]

		# check the next pilot entry
		if self.navigating == True:
			self.pilotdecay -= addedTime
			if self.pilotdecay < 0.0:
				self.pilotdecay = 0.0
				self.navigating = False
				if len(self.pilots) != 0:
					nextpilot = self.pilots.pop(0)
					if nextpilot[0] == 0:	# zoom
						self.zoom(nextpilot[1])
					elif nextpilot[0] == 1:	# pan
						self.move(nextpilot[1], nextpilot[2])
				else:
					self.navigating = False
						
		# send update bbox msg 20 times per second
		self.bbtimer += 1.0
		if self.bbtimer > 20.0:
			msg = "[27]"
			outputs.put(msg)
			self.bbtimer = 0.0
			
			# also update current dataset focus
			self.searchDataset()


################################################################################
# Activity State Function
################################################################################

# initial state function
def MCINIStateFunc(activity, str):

	if str == 'Bored':
		# just remain silent
		if activity.idleCounter > 1:
			# set to initial state
			activity.reset()
			activity.active = True
			activity.speak("state 0 bored")
			
	elif activity.rulename == 'MC_INI_ATTENTION':	# from "hi, there"
		set = activity.navigator.searchDataset()
		if set == -1:
			activity.speak(helmet0mark + "Hello, my name is " + gAvatarFirstName + ". I am your tour guide through these images of the universe." + joystickmark + "<silence msec=\"500\"/> you can use the joystick to move the image and zoom in and out. I can also take you on a tour.")
			activity.lastspeech = "I am your tour guide through these images of the universe.<silence msec=\"500\"/> you can use the joystick to move the image and zoom in and out. I can also take you on a tour."
		else:
			speech = helmet0mark + "<bookmark mark=\"[whiteboard][URL][web/carina/" +gDataSets[set][8] + "]\"/> Hello, my name is " + gAvatarFirstName + ". I am your tour guide through these images of the universe. This current image on the display is the "
			speech += gDataSets[set][7]
			speech += joystickmark + "<silence msec=\"500\"/> you can use the joystick to move the image and zoom in and out. I can also take you on a tour."
			activity.lastspeech = speech
			activity.speak(speech)
	else:
		# this is the first time call
		activity.speak("ready")
		activity.lastspeech = ""
		

# interactive state function
def MCINTERACTIVEStateFunc(activity, str):
	
	if str == 'Bored':
		# just remain silent and quitely goes back to initial if too long idle
		if activity.idleCounter > 1:
			# disable current grammar
			if activity.grammarIDs.has_key(activity.current_state):
				activity.setGrammarActive(activity.grammarIDs[activity.current_state], False)
			# set to initial state
			activity.reset()
			activity.active = True
			activity.setCurrentGrammar(activity.currentGrammarID)
			activity.speak("state 1 bored")

	elif activity.rulename == 'MC_INTERACTIVE_ATTENTION':
		speech = activity.tourattentionspeech.pop()
		activity.speak(speech)
		activity.lastspeech = speech
	
	elif activity.rulename == 'MC_INTERACTIVE_PHOTO':	# "tell me more about this photo"
		set = activity.navigator.searchDataset()
		speech = ""
		if set == -1:
			speech = "I'm sorry. You are looking at empty space."
		else:
			speech = gDataSets[set][9]
			activity.setWBURL(gDataSets[set][8], 15.0)
		activity.lastspeech = speech
		activity.speak(speech)

	elif activity.rulename == 'MC_INTERACTIVE_HELP':
		speech = "I am here to help you explore the Carina Nebula. Please use the lower left hand box as a guide to questions you can ask me."  + joystickmark + "You can use the joy stick to move the image and zoom in and out. I can also take you on a tour. "
		activity.speak(speech)
		activity.lastspeech = speech


# tour state function : tour
# automatic tour guide mode: xxx controls navigation and explanations.
def MCTOURStateFunc(activity, str):
	
	if str == 'Bored':
		if activity.idleCounter > 1:
			# disable current grammar
			if activity.grammarIDs.has_key(activity.current_state):
				activity.setGrammarActive(activity.grammarIDs[activity.current_state], False)
			# set to initial state
			activity.reset()
			activity.active = True
			activity.setCurrentGrammar(activity.currentGrammarID)
			activity.speak("state 3 bored")

# navigator to ROI
def MCNAVROI(activity, str):
	
	if activity.roi_dictionary.has_key(activity.rulename):
		(setid, rid) = activity.roi_dictionary[activity.rulename]
		speech = activity.navigator.getROIDescription(setid, rid)
		activity.lastspeech = speech
		activity.speak(speech)
		
		spot = activity.navigator.ROI[setid][rid]
		msg = "[104][%s][%i][%f][%f][%f][%i]" % (spot[0], setid, spot[1]*2.0, -spot[2]*2.0, spot[3], gMarkerTimer)
		outputs.put(msg)

	# For the idle call, let's set the current action(function) to interactive steate function
	activity.action = MCINTERACTIVEStateFunc


# resetview function
def MCResetView(activity, str):

	activity.speak("OK")
	activity.lastspeech = "I just reset current view"
	activity.navigator.resetView()

	# For the idle call, let's set the current action(function) to interactive steate function
	activity.action = MCINTERACTIVEStateFunc

# exit function
def MCExit(activity, str):

	activity.exiting = True
	
	if activity.rulename == 'MC_TOUR_EXIT' or activity.rulename == 'MC_TOUR_PAUSE_EXIT':
		# need to stop speaking
		activity.stopspeak()
		activity.stopTour()
	
	if 'good' in str:	# good bye
		activity.speak("<silence msec=\"500\"/>Please, come again." + helmet1mark)
		activity.lastspeech = ""
	else:				# exit
		activity.speak("<silence msec=\"500\"/>Allright. If you have any questions just ask." + helmet1mark)
		activity.lastspeech = "You left Kuhreenah application and I told you ask me any question if you wish."

	activity.setTipString("Carina Application", 10.0)
	activity.setWBURL(titleurl, 0.0)

def MCRepeat(activity, str):
	# user requested repeat of the last speech
	# for now just speak it again
	if len(activity.lastspeech) != 0: 
		speech = activity.repeatspeech.pop() + activity.lastspeech
		activity.speak(speech)
	else:
		activity.speak("I'm sorry I do not remember what I said.")

	# For the idle call, let's set the current action(function) to interactive steate function
	activity.action = MCINTERACTIVEStateFunc

# what is this function
def MCWhatIsThis(activity, str):
	activity.addDebug("what is this function start...")
	# check the current dataset and ROI
	# [setid, id, name, nearid, speech]
	roi = activity.navigator.searchInterests()
	if roi[0] == -1:	# no dataset focused
		activity.addDebug("no dataset detected...")
		speech = "I don't see anything interesting here. Let's take a look at one of the images."
		activity.speak(speech)
		activity.lastspeech = speech
		activity.setWBURL(gDataSets[0][8], 5.0)
		activity.navigator.navigateToRecentSet()
		# prevent changing state function
		return
	else:
		activity.addDebug("got dataset...")
		if roi[1] == -1:		#nothing
			activity.addDebug("got no roi...")
			
			# is this set have no roi?
			if roi[3] == -1:
				activity.setWBURL(gDataSets[roi[0]][8], 0.0)
				speech = "This is the " + gDataSets[roi[0]][7] + " image."
				speech += "I do not have any specific interesting area here."
				activity.lastspeech = speech
				activity.speak(speech)
			else:
				zx = (gScreenX / gImagePixel[activity.navigator.currentSet][0]) 
				zy = (gScreenY / gImagePixel[activity.navigator.currentSet][1]) 
				z = min(zx, zy)
				if gCamera[2]*2.0 < z:	# looking at the whole picture...
					activity.setWBURL(gDataSets[roi[0]][8], 0.0)
					speech = "This is the " + gDataSets[roi[0]][7] + " image."
					rlist = activity.navigator.getROIList(roi[0])
					speech += "<silence msec=\"500\"/> There are several interesting features in this image. I can tell you about "
					speech += rlist
					speech += " or I can take you on a tour."
					activity.lastspeech = speech
					activity.speak(speech)
					# may send all roi marker with short time period!!!
					for spot in activity.navigator.ROI[roi[0]]:
					  msg = "[104][%s][%i][%f][%f][%f][%i]" % (spot[0], roi[0], spot[1]*2.0, -spot[2]*2.0, spot[3], gMarkerTimer)
					  outputs.put(msg)

				else:
					speech = "I'm sorry, I don't see any interesting features here. The nearest one is " + roi[2] + "Do you want me to take you there?"
					activity.lastspeech = "I'm sorry, I don't see any interesting features here. The nearest one is " + roi[2] + "Do you want me to take you there?"
					activity.speak(speech)
					# yes/no question: some state change necessary...
					activity.questionresponse[0] = 'OK. No problem'
					activity.questionresponse[1] = 'OK. Take your time. Just let me know if you need help.'
					activity.setQuestionState('YESNO', 'GOTOROI','STATE_INTERACTIVE')
					
					# send marker of the nearest one
					# marker msg [104][Center][0][0.0][0.0][0.15][10000]  [msgid][caption str][setid][x][y][radius][duration_tick]
					spot = activity.navigator.ROI[roi[0]][roi[3]]
					msg = "[104][%s][%i][%f][%f][%f][%i]" % (roi[2], roi[0], spot[1]*2.0, -spot[2]*2.0, spot[3], gMarkerTimer)
					outputs.put(msg)
					
		else:
			activity.addDebug("got roi...")
			activity.lastspeech = "you are looking at the " + roi[2] + "<silence msec=\"500\"/> Do you want to know more about it?"
			activity.speak("you are looking at the " + roi[2] + "<silence msec=\"500\"/> Do you want to know more about it?")
			activity.questionresponse[0] = 'Good.'
			activity.questionresponse[1] = 'OK.'
			activity.setQuestionState('YESNO', 'EXPLAINROI','STATE_INTERACTIVE')
			# definitely send marker message
			# marker msg [104][Center][0][0.0][0.0][0.15][10000]  [msgid][caption str][setid][x][y][radius][duration_tick]
			spot = activity.navigator.ROI[roi[0]][roi[1]]
			msg = "[104][%s][%i][%f][%f][%f][%i]" % (roi[2], roi[0], spot[1]*2.0, -spot[2]*2.0, spot[3], gMarkerTimer)
			outputs.put(msg)

	
	# For the idle call, let's set the current action(function) to interactive steate function
	activity.action = MCINTERACTIVEStateFunc


# yes/no function
def MCYesNo(activity, str):

	# no input from user for a while: this called from update rutine
	if str == 'Bored':
		if activity.idleCounter > 1:
			# disable current grammar
			if activity.grammarIDs.has_key(activity.current_state):
				activity.setGrammarActive(activity.grammarIDs[activity.current_state], False)
			
			# set to initial state
			activity.current_state = activity.question_target_state
			activity.question_target_state = None
			activity.questioning = False
			activity.question_type = 'NONE'
			activity.lastspeech = ''
			
			# since this is passive state change without sr
			# need to set grammar stuff all manually
			activity.currentGrammarID = activity.grammarIDs[activity.current_state]
			activity.setCurrentGrammar(activity.currentGrammarID)

	if activity.rulename == 'MC_YESNO_REPEAT':
		MCRepeat(activity, str)
		return
		
	if activity.rulename == 'MC_YES':
		
		speechpre = activity.questionresponse[0]
		speech = ""
		
		if activity.question_type == 'GOTOROI':
			activity.navigator.navigateToTarget()
		elif activity.question_type == 'EXPLAINROI':
			speech = activity.navigator.getROIDescription(activity.navigator.currentSet, activity.navigator.currentROI)
			activity.speak(speech)

		activity.speak(speechpre + speech)
		if len(speech) == 0:
			activity.lastspeech = speechpre
		else:
			activity.lastspeech = speech
			
	elif activity.rulename == 'MC_NO':
		activity.speak(activity.questionresponse[1])
		activity.lastspeech = activity.questionresponse[1]
	else:
		activity.speak("something wrong in yes no answer")
		activity.lastspeech = "I said something wrong in yes no question"

	# reset vars
	activity.next_state = activity.question_target_state
	activity.question_target_state = None
	activity.questioning = False
	activity.question_type = 'NONE'

# idle transition function
def MCIdle(activity, str):
	# two cases here
	# str == 'Bored' -> still idling...
	# ruleid == MC_IDLE_ATTENTION
	if str == 'Bored':
		# increment internal idle counter. if condition met, may change state
		pass
	elif activity.rulename == 'MC_IDLE_ATTENTION':
		# got user's attention request: wake up!
		activity.wakeUp()

def MCLocation(activity, str):
	### this need to be changed to use dataset based coordinate
	# what if location is not within the sets? maybe say "you are out side of image."
	set = activity.navigator.searchDataset()
	if set == -1:	# nothing on center of screen
		speech = "You are looking at the empty space. It is hard to tell your location. I only can give you relative location within image."
		activity.speak(speech)
		activity.lastspeech = speech
	else:
		# get image relative coord
		coord = activity.navigator.getCoordinate()
		speech = "In the " + gDataSets[set][7] + " Image, your location is x coordinate %2.2f, y coordinate %2.2f, zoom level %2.1f" % (coord[0], coord[1], gCamera[2])
		activity.lastspeech = speech
		activity.speak(speech)

	# For the idle call, let's set the current action(function) to interactive steate function
	activity.action = MCINTERACTIVEStateFunc
	
def MCZoom(activity, str):
	# send fixed relative amount of zoom msg to MC
	if 'in' in str:
		activity.navigator.zoom(1.0+gZoomStep, False)
	else:
		activity.navigator.zoom(1.0-gZoomStep, False)
	
	activity.speak("OK")
	activity.lastspeech = "I just zoomed screen for you."

def MCPan(activity, str):
	# send fixed relative amount of move msg to MC
	if 'left' in str:
		activity.navigator.move(gPanStep, 0.0, False)
	elif 'right' in str:
		activity.navigator.move(-gPanStep, 0.0, False)
	elif 'up' in str:
		activity.navigator.move(0.0, gPanStep, False)
	else:
		activity.navigator.move(0.0, -gPanStep, False)
	
	activity.speak("OK")
	activity.lastspeech = "Well, nothing special. I just moved screen for you."

def MCShowSomething(activity, str):
	# user request: show me something...
	# where am I looking at?
	set = activity.navigator.searchDataset()
	if set == -1:	# nothing
		# if looking at the empty space, move to least recent dataset and continue explanation.
		set = activity.navigator.navigateToRecentSet()
	else:
		rois = activity.navigator.getROIWBList(set)
		if rois == "<p>Interesting Features</p>":
			speech = "I don't have any specific topics for "
			speech += gDataSets[set][7]
			#speech += " image. But, I can take you on a tour."
			speech += " image."
			activity.lastspeech = speech
			activity.speak(speech)
		else:
			activity.setWBContents(rois)
			speech = "I can tell you about "
			speech += activity.navigator.getROIList(set)
			speech += "or I can take you on a tour."
			activity.lastspeech = speech
			activity.speak(speech)
			
			# may send all roi marker with short time period!!!
			for spot in activity.navigator.ROI[set]:
				msg = "[104][%s][%i][%f][%f][%f][%i]" % (spot[0], set, spot[1]*2.0, -spot[2]*2.0, spot[3], gMarkerTimer)
				outputs.put(msg)

	
	# For the idle call, let's set the current action(function) to interactive steate function
	activity.action = MCINTERACTIVEStateFunc

def MCROI(activity, str):
	# user request: what are the interesting features here?
	# where am I looking at?
	set = activity.navigator.searchDataset()
	if set == -1:	# nothing
		# if looking at the empty space, move to least recent dataset and continue explanation.
		set = activity.navigator.navigateToRecentSet()
	else:
		rois = activity.navigator.getROIWBList(set)
		if rois == "<p>Interesting Features</p>":
			speech = "I don't have any specific topics for "
			speech += gDataSets[set][7]
			#speech += " image. But, I can take you on a tour."
			speech += " image."
			activity.lastspeech = speech
			activity.speak(speech)
		else:
			count = rois.count("<p id=\"c\">")
			activity.setWBContents(rois)
			speech = "There are "
			speech += ('%d' % count)
			speech += " interesting features here. "
			speech += activity.navigator.getROIList(set)
			activity.lastspeech = speech
			activity.speak(speech)

			# may send all roi marker with short time period!!!
			for spot in activity.navigator.ROI[set]:
				msg = "[104][%s][%i][%f][%f][%f][%i]" % (spot[0], set, spot[1]*2.0, -spot[2]*2.0, spot[3], gMarkerTimer)
				outputs.put(msg)

	
	# For the idle call, let's set the current action(function) to interactive steate function
	activity.action = MCINTERACTIVEStateFunc
	
def MCStartTour(activity, str):
	# does current set has tour spots?
	# if not do not start touring...!
	
	if activity.touring == False:
		activity.navigator.currentSet
		rlist = activity.navigator.getROIList(activity.navigator.currentSet)
		if rlist is not "":
			activity.speak("OK. Let's start. Feel free to interrupt me <bookmark mark=\"[ACTIVITY][BRD][TOUR]\"/>at any moment.")
			activity.setWBURL(gDataSets[activity.navigator.currentSet][8], 0.0)
			activity.lastspeech = "I told you feel free to interrupt me at any moment."
			activity.nextwhiteboard = ''
		else:
			activity.speak("Sorry, I do not have tour information for this set. Please try others.")
			activity.lastspeech = "I told you I do not have tour information for this set. Please try others."
			activity.tourEnded()
	else:
		#activity.speak("well, we are already in tour mode.")
		#activity.lastspeech = "I said you are already in tour mode"
		pass
		
def MCStopTour(activity, str):
	if activity.touring == True:
		activity.stopspeak()
		activity.speak("<silence msec=\"1000\"/> No problem. Take your time to explore this image.")
		activity.lastspeech = "You just stopped tour mode."
		activity.stopTour()
		activity.setWBURL(gDataSets[activity.navigator.currentSet][8], 0.0)
	else:
		activity.speak("You are not in tour mode.")
		activity.lastspeech = "You are not in tour mode."

def MCPauseTour(activity, str):
	
	# no input from user for a while: this called from update rutine
	if str == 'Bored':
		if activity.idleCounter > 1:
			# disable current grammar
			if activity.grammarIDs.has_key(activity.current_state):
				activity.setGrammarActive(activity.grammarIDs[activity.current_state], False)
			# stop tour mode
			activity.navigator.stopTour()
			activity.tourEnded()

	if activity.touring == True:
		activity.stopspeak()
		activity.speak("<silence msec=\"500\"/> OK, the tour is paused.")
		activity.pauseTour()
	else:
		activity.speak("You are not in tour mode.")
		activity.lastspeech = "You are not in tour mode."
	
def MCResumeTour(activity, str):
	if activity.touring == True:
		activity.speak("<silence msec=\"500\"/> Sure. let's resume our tour.")
		activity.resumeTour()
	else:
		activity.speak("You are not in tour mode.")
		activity.lastspeech = "You are not in tour mode."

def MCRestartTour(activity, str):
	if activity.touring == True:
		activity.stopspeak()
		activity.speak("<silence msec=\"500\"/> OK. let's start it again.")
		activity.restartTour()
	else:
		activity.speak("You are not in tour mode.")
		activity.lastspeech = "You are not in tour mode."

def MCNextTour(activity, str):
	if activity.touring == True:
		activity.stopspeak()
		activity.nextTour(True)
	else:
		activity.speak("You are not in tour mode.")
		activity.lastspeech = "You are not in tour mode."

def MCPreviousTour(activity, str):
	if activity.touring == True:
		activity.stopspeak()
		activity.previousTour()
	else:
		activity.speak("You are not in tour mode.")
		activity.lastspeech = "You are not in tour mode."

def MCTerminology(activity, str):
	if activity.rulename == 'MC_SUPERNOVA':
		speech = "They are not in this map, they are quite rare. They happen only once in a hundred years in our galaxy. Since the Milky Way galaxy is so big, we may not even see the next one that happens. The next Supernova we will see will happen between ten thousand and one hundred thousand years. "
		activity.speak(speech)
		activity.lastspeech = speech
	elif activity.rulename == 'MC_DOUBLESTAR':
		speech = "A double start is multiple star system. One star is revolving around another."
		activity.speak(speech)
		activity.lastspeech = speech
	elif activity.rulename == 'MC_WHEREEARTH':
		speech = "Earth is not in this picture. This locates about 7,000 light years from Earth. 6 trillion miles is one light year."
		activity.speak(speech)
		activity.lastspeech = speech
	elif activity.rulename == 'MC_LIGHTYEAR':
		speech = "Lightyear is a unit of length. A lightyear is the distance that light travels in one year. It is equal to about 6 trillion miles."
		activity.speak(speech)
		activity.lastspeech = speech
	elif activity.rulename == 'MC_NEBULA':
		speech = "A nebula is an interstellar cloud of dust, hydrogen gas, helium gas and other ionized gases. Originally, nebula was a general name for any extended astronomical object, including galaxies beyond the Milky Way."
		activity.speak(speech)
		activity.lastspeech = speech
	else:
		speech = "no knowledge about this terminology"
		activity.speak(speech)
		activity.lastspeech = speech


def MCQuestionFunc(activity, str):
	# this function is designed to answer many of general questions
	# list of QAs is in separate file (i.e. general.qa)
	# will need some kind of map storage to find appropriate asnwer pool
	# once get pool, then pop up one answer and speak.
	
	pass
	



################################################################################
# Activity Class Derived from C++
################################################################################
class PythonActivity( LLActivityBase ):

	def initialize( self ):

		# Load configuration file
		global gConfigPath, gScriptDir
		gConfigPath = gScriptDir + "/data/"
		cfile = gConfigPath + "config.conf"
		loadConfiguration(cfile)
		
		# Map (input, current_state) --> (action, next_state)
		# action is state related function assigned to it
		self.state_transitions = {}
		self.roi_dictionary = {}
		self.state_tip = {}
		self.exiting = False
		self.elapsedIdle = 0
		self.idleCounter = 0
		self.active = False
		self.suspended = False
		#self.randIdle = random.uniform(100, 120)
		self.randIdle = random.uniform(5, 10)
		self.whiteboardtimer = 0.0
		self.nextwhiteboard = ''
		
		# special case handling storage
		self.questionresponse = ['yes', 'no']
		self.questioning = False
		self.question_type = 'NONE'
		self.question_target_state = None
		
		# memory for the last speech
		self.lastspeech = ""
		
		# initial state
		self.initial_state = 'STATE_INI'
		self.current_state = self.initial_state
		self.action = MCINIStateFunc
		self.next_state = None
		self.prev_state = None
		self.touring = False
		self.rulename = None
		
		# Navigator & Network stuff
		self.navigator = MCNavigator(self)
		self.client = Client(self.navigator)

		# Avatar Name
		#self.avatarName = self.getOwnerName()
		#names = self.avatarName.split()
		#self.avatarFirstName = names[0]

		# register transition to self from ActivityManager
		# use ':' as delim for multiple recognizable inputs
		self.registerTransition("Carina Application")
		
		# initialize all states and its function
		# step1: add grammar -> add default transition
		# step2: add rule -> add transition with rule id
		#  repeat step2 till all required transitions added
		
		########################################################################
		# Initial state (initial one)
		gramid = self.addGrammar("MC_STATE_INI")
		self.currentGrammarID = gramid
		self.grammarIDs = {'STATE_INI':gramid}
		ruleid = self.addGrammarRule(gramid, "MC_INI_ATTENTION", "hello:hi there:hey " + gAvatarFirstName)
		self.addTransition(ruleid, 'STATE_INI', MCINIStateFunc, 'STATE_INTERACTIVE')
		self.state_tip['STATE_INI'] = "Hello\nHi, there.\nHey, " + gAvatarFirstName
		
		########################################################################
		# STATE_INTRACTIVE state (): interaction with image and avatar
		gramid = self.addGrammar("MC_STATE_INTERACTIVE")
		self.grammarIDs['STATE_INTERACTIVE'] = gramid
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_ATTENTION", gAvatarFirstName + ":hey " + gAvatarFirstName)
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCINTERACTIVEStateFunc, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_WHAT", "what is this:where is this:what am I looking at:what's on the display:what's that")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCWhatIsThis, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_PHOTO", "tell me more about this photo:tell me more about the picture:where did this photo come from")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCINTERACTIVEStateFunc, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_SOMETHING", "something:show me something")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCShowSomething, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_ROI", "interesting features:what are the interesting features here")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCROI, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_TOUR", "tour:tour please:take me on a tour:give me the tour")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCStartTour, 'STATE_TOUR')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_REPEAT", "repeat")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCRepeat, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_RESETVIEW", "reset view:show the whole picture:start over")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCResetView, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_EXIT", "finish application:good bye")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCExit, 'STATE_INI')
		
		# some terminology questions
		ruleid = self.addGrammarRule(gramid, "MC_SUPERNOVA", "what is a supernova:tell me about a supernova:explain a supernova")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCTerminology, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_DOUBLESTAR", "what is a double star:tell me about a double star:explain a double star")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCTerminology, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_WHEREEARTH", "where is earth:show me earth")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCTerminology, 'STATE_INTERACTIVE')
		

		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_HELP", "help:please help:help me:I don't know what to do:can you help me:what am I supposed to do:how does this work")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCINTERACTIVEStateFunc, 'STATE_INTERACTIVE')		

		########################################################################
		# navigate to ROI
		for n in range(gNumDataSet):
			roinames = self.navigator.getROINames(n)
			for rname in roinames:
				rulestr = "MC_ROI_%s" % rname[0]
				srstring = "%s:tell me about %s:show me the %s:where is the %s" % (rname[0], rname[0], rname[0], rname[0])
				ruleid = self.addGrammarRule(gramid, rulestr, srstring)
				self.addTransition(ruleid, 'STATE_INTERACTIVE', MCNAVROI, 'STATE_INTERACTIVE')
				self.roi_dictionary[rulestr] = (n, rname[1])
		self.state_tip['STATE_INTERACTIVE'] = "What is this?\nTell me more about this photo.\nWhat are the interesting features here?\nTake me on a tour.\nHelp"
		########################################################################
		
		########################################################################
		# STATE_TOUR state (): tour
		gramid = self.addGrammar("MC_STATE_TOUR")
		self.grammarIDs['STATE_TOUR'] = gramid
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_STOPTOUR", "stop tour")
		self.addTransition(ruleid, 'STATE_TOUR', MCStopTour, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_PAUSETOUR", "pause tour:hold on")
		self.addTransition(ruleid, 'STATE_TOUR', MCPauseTour, 'STATE_TOUR_PAUSE')
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_NEXT", "next one:skip current:skip this:skip this one:move to next")
		self.addTransition(ruleid, 'STATE_TOUR', MCNextTour, 'STATE_TOUR')
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_PREVIOUS", "previous one:back to previous:go back")
		self.addTransition(ruleid, 'STATE_TOUR', MCPreviousTour, 'STATE_TOUR')
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_RESTART", "restart tour")
		self.addTransition(ruleid, 'STATE_TOUR', MCRestartTour, 'STATE_TOUR')
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_EXIT", "finish application:good bye")
		self.addTransition(ruleid, 'STATE_TOUR', MCExit, 'MC_STATE_INI')
		
		self.state_tip['STATE_TOUR'] = "Stop Tour\nPause Tour\nNext One\nPrevious One\nRestart Tour\nExit or Good-bye."
		
		########################################################################
		# STATE_TOUR_PAUSED state (): tour paused
		gramid = self.addGrammar("MC_STATE_TOUR_PAUSED")
		self.grammarIDs['STATE_TOUR_PAUSE'] = gramid
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_PAUSE_RESUMETOUR", "resume:resume tour")
		self.addTransition(ruleid, 'STATE_TOUR_PAUSE', MCResumeTour, 'STATE_TOUR')
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_STOPTOUR", "stop tour")
		self.addTransition(ruleid, 'STATE_TOUR_PAUSE', MCStopTour, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_RESTART", "restart tour")
		self.addTransition(ruleid, 'STATE_TOUR_PAUSE', MCRestartTour, 'STATE_TOUR')
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_PAUSE_EXIT", "finish application:good bye")
		self.addTransition(ruleid, 'STATE_TOUR_PAUSE', MCExit, 'MC_STATE_INI')

		ruleid = self.addGrammarRule(gramid, "MC_SUPERNOVA", "what is a supernova:tell me about a supernova:explain a supernova")
		self.addTransition(ruleid, 'STATE_TOUR_PAUSE', MCTerminology, 'STATE_TOUR_PAUSE')
		ruleid = self.addGrammarRule(gramid, "MC_DOUBLESTAR", "what is a double star:tell me about a double star:explain a double star")
		self.addTransition(ruleid, 'STATE_TOUR_PAUSE', MCTerminology, 'STATE_TOUR_PAUSE')
		ruleid = self.addGrammarRule(gramid, "MC_LIGHTYEAR", "what is a light year:tell me about light year:explain light year")
		self.addTransition(ruleid, 'STATE_TOUR_PAUSE', MCTerminology, 'STATE_TOUR_PAUSE')
		ruleid = self.addGrammarRule(gramid, "MC_NEBULA", "what is a nebula:tell me about a nebula:explain a nebula")
		self.addTransition(ruleid, 'STATE_TOUR_PAUSE', MCTerminology, 'STATE_TOUR_PAUSE')

		self.state_tip['STATE_TOUR_PAUSE'] = "Resume Tour\nRestart Tour\nStop Tour\nExit"
		
		########################################################################
		# Yes/No state: need some exception handling...
		# need to know what the question was
		gramid = self.addGrammar("MC_YESNO")
		self.grammarIDs['YESNO'] = gramid
		ruleid = self.addGrammarRule(gramid, "MC_YES", "yes:yes please:sure:why not:yes I do:please do")
		self.addTransition(ruleid, 'YESNO', MCYesNo, 'YESNO')
		ruleid = self.addGrammarRule(gramid, "MC_NO", "no:no thanks:no thank you:never mind:not at all:no I don't")
		self.addTransition(ruleid, 'YESNO', MCYesNo, 'YESNO')
		ruleid = self.addGrammarRule(gramid, "MC_YESNO_REPEAT", "repeat")
		self.addTransition(ruleid, 'YESNO', MCYesNo, 'YESNO')
		self.state_tip['YESNO'] = "Yes or No."
		
		########################################################################
		# Experimental General Questions session
		# 	need to load up separate file... but just try some.
		########################################################################
		
		
		
		########################################################################
		# idle state: temporary state - tighten SR (only allow attention call)
		# similar to yes/no state, this needs to remember the last state
		# so that it can go back to the last one when it gets attention
		gramid = self.addGrammar("MC_IDLE")
		self.grammarIDs['IDLE'] = gramid
		ruleid = self.addGrammarRule(gramid, "MC_IDLE_ATTENTION", "hey " + gAvatarFirstName + ":" + gAvatarFirstName)
		self.addTransition(ruleid, 'IDLE', MCIdle, 'IDLE')
		
		########################################################################
		# Speech Pool: avoid unnatural repetition of speech sentences
		# Attention Pool: when user say "Hey, XXX"
		self.tourattentionspeech = SpeechPool()
		self.tourattentionspeech.add("Yes.")
		self.tourattentionspeech.add("Yes, I'm here.")
		self.tourattentionspeech.add("Yes, what do you want?")
		self.tourattentionspeech.add("Yes, how can I help you?")
		
		# some prefix for repeat request
		self.repeatspeech = SpeechPool()
		self.repeatspeech.add("OK. <silence msec=\"500\"/>")
		
		self.setName("Carina Activity");
		self.setWBURL(titleurl, 0.0)
		

	def setActive( self , str):

		if self.grammarIDs.has_key(self.current_state):
			self.setCurrentGrammar(self.grammarIDs[self.current_state])
		
		# are we resuming?
		if self.suspended == True:
			self.suspended = False
		elif self.action is not None:
			self.action(self, str)
		
		self.active = True
		self.elapsedIdle = 0
		
		# tips
		tips = self.state_tip[self.current_state]
		if tips is not None:
			self.setTipString(tips, 10.0)
			
		
		# calibration...
		global gCalibrating, gCalibrationTime, gCalibrated, gCamera
		if gCalibrated == False and gCalibrating == False:
			outputs.put("[20]")
			gCamera[2] = 1.0
			time.sleep(1)
			outputs.put("[102][2.0][20]")
			gCalibrating = True
			time.clock()
		

	def setQuestionState(self, qstate, type, targetstate):
		
		self.prev_state = self.current_state
		self.current_state = qstate
		self.next_state = None
			
		self.questioning = True
		self.question_target_state = targetstate
		self.question_type = type

	def setWBURL(self, url, time = 0.0):
		newurl = "web/carina/" + url
		if time == 0.0:
			self.nextwhiteboard = ''
			self.whiteboardtimer = 0.0
			self.setWhiteboardURL(newurl)
		else:
			self.nextwhiteboard = newurl
			self.whiteboardtimer = time
	
	def setWBContents(self, contents):
		# only support immediate change of whiteboard contents
		self.nextwhiteboard = ''
		self.whiteboardtimer = 0.0
		self.setWhiteboardContents(contents)
		
	def processRecognition( self, gid, rid, conf, listened, rulename ):
		
		# gid: current state, rid: FSM input
		# conf: SR confidence, str: recognized string

		self.rulename = rulename
		
		(self.action, self.next_state) = self.getTransition(rid, self.current_state)
		if self.action is not None:
			self.action(self, listened)
		
		self.elapsedIdle = 0
		self.idleCounter = 0
		
		# update status
		if self.questioning == True:		# this value set by one of action...
			# now we assume the state is already changed manually
			# so do not change it here.
			pass
		else:
			self.prev_state = self.current_state
			self.current_state = self.next_state
			self.next_state = None
		
		# need to change grammar
		if self.grammarIDs.has_key(self.current_state):
			newgram = self.grammarIDs[self.current_state]
		else:
			newgram = self.currentGrammarID
		
		if self.exiting is True:
			# disable current grammar
			#if self.grammarIDs.has_key(self.current_state):
			#	self.setGrammarActive(self.grammarIDs[self.current_state], False)
			if self.grammarIDs.has_key(self.prev_state):
				self.setGrammarActive(self.grammarIDs[self.prev_state], False)
			
			self.reset()
			return -1
		else:
			# tips
			tips = self.state_tip[self.current_state]
			if tips is not None:
				self.setTipString(tips, 10.0)
			self.setCurrentGrammar(newgram)
			return newgram

	def reset (self):
		
		self.current_state = self.initial_state
		self.prev_state = None
		self.exiting = False
		self.suspended = False
		self.elapsedIdle = 0
		self.idleCounter = 0
		self.active = False
		self.action = MCINIStateFunc
		self.currentGrammarID = self.grammarIDs[self.initial_state]
		self.touring = False
		self.whiteboardtimer = 0.0
		self.nextwhiteboard = ''

		# empty all existing navigation list
		self.navigator.reset()
		
	def suspend (self):
		
		# disable current SR grammar and get into suspend mode
		if self.grammarIDs.has_key(self.current_state):
			self.setGrammarActive(self.grammarIDs[self.current_state], False)
		
		self.active = False
		self.elapsedIdle = 0
		self.idleCounter = 0
		self.suspended = True
	
	def resume (self):
		
		# enable current SR grammar and get back to work
		if self.grammarIDs.has_key(self.current_state):		
			self.setGrammarActive(self.grammarIDs[self.current_state], True)
			
		self.suspended == False
		self.active = True
		
	def idle (self):
		# assumption: user is still there, interaction with magicarpet
		# how mark know if there is interaction going or not (user's presence)?
		# if no interaction, no user, then back to initial state or exit activity
		
		# reset counter
		self.idleCounter = 0
		self.elapsedIdle = 0
		self.randIdle = 10		# check idle in 10 seconds
		
		# store current state, disable grammar
		
		# change state to idle, enable idle grammar
		
		pass
	
	def wakeUp (self):
		pass
		
	def addTransition (self, input, state, action, next_state):
		# input(ruleid), state(current state), action(function), next_state
		self.state_transitions[(input, state)] = (action, next_state)
		
		

	def getTransition (self, input, state):
		
		if self.state_transitions.has_key((input, state)):
			return self.state_transitions[(input, state)]
		else:
			return (None, state)

	def msg_received (self, msg):
		# original msg string: [ACTIVITY][BRD][NAV:0.5:0.5:1.0]
		# msg received string: NAV:0.5:0.5:1.0
		
		# parse the command
		vars = []
		vars = msg.split(':')
		if len(vars) != 0:
			if vars[0] == 'TOUR':
				self.startTour()
			elif vars[0] == 'NAV':
				x = float(vars[1])
				y = float(vars[2])
				z = float(vars[3])
				self.navigator.navigateTo(x, y, z*0.5)		# check radius!!!
			elif vars[0] == 'MOV':		# move only with interpolator
				x = float(vars[1])
				y = float(vars[2])
				int = float(vars[3]) * gPanRate
				self.navigator.moveInt(x, y, int)
			elif vars[0] == 'ZOM':		# zoom only with interpolator
				z = float(vars[1])
				int = float(vars[2]) * gZoomRate
				self.navigator.zoomInt(z, int)
			elif vars[0] == 'SPD':		# when speech is done
				if self.touring:
					# move to the next stop
					self.navigator.nextTour()
			elif vars[0] == 'ACT':		# action invocation: call current action
				if self.action is not None:
					self.action(self, vars[1])

	def startTour (self):
		# start auto pilot touring: requested by user
		self.touring = True
		self.navigator.startTour()
	
	def restartTour (self):
		# start auto pilot touring: requested by user
		self.touring = True
		self.navigator.startTour()

	def pauseTour (self):
		# likely requested by user
		self.navigator.pauseTour()
	
	def resumeTour (self):
		# likely requested by user
		self.navigator.resumeTour()
	
	def stopTour (self):
		# this can be called from anywhere
		self.navigator.stopTour()
		self.touring = False
		
		# then, we go back to interactive user mode
		
	def nextTour (self, forced = False):
		self.navigator.nextTour(forced)
		
	def previousTour (self):
		self.navigator.previousTour()

	def tourEnded (self):
		# this only be called by navigator when its touring is ended
		self.touring = False
		
		# some other necessary stuff: state change?
		# when auto pilot tour ends, go back to interactive user mode
		self.reset()
		self.current_state = 'STATE_INTERACTIVE'
		self.next_state = 'STATE_INTERACTIVE'
		self.currentGrammarID = self.grammarIDs['STATE_INTERACTIVE']
		self.active = True
		self.setCurrentGrammar(self.currentGrammarID)
		
		# tips
		tips = self.state_tip[self.current_state]
		if tips is not None:
			self.setTipString(tips, 10.0)
	
	def sendBB (self, msg):
		self.feedback(msg)
		
	def update (self, addedTime):
		self.sendBB(gBBMSG)
		# tick function call from C++
		self.navigator.update(addedTime)
		
		# whiteboard timer update
		if len(self.nextwhiteboard) !=0:
			self.whiteboardtimer -= addedTime
			if self.whiteboardtimer < 0.0:
				# set new url
				self.setWhiteboardURL(self.nextwhiteboard)
				self.whiteboardtimer = 0.0
				self.nextwhiteboard = ''
		
		# calibration
		global gCalibrating, gCalibrationTime, gCalibrated
		if gCalibrating is True:
			# check zoom level
			delta = 2.0 - gCamera[2]
			if abs(delta) < 0.01:	# cutoff -> about 90 draw frames
				gCalibrating = False
				gCalibrated = True
				# zoom from 1.0 to 2.0 with interpolator 20: test command sent
				# set factor to use user specified interpolator 1.0 as 1 second time
				elapsedtime = time.clock()
				global gFPS, gZoomRate, gPanRate
				gFPS = 90.0 / elapsedtime		# tick count for zoom 2.0 is about 90
				gPanRate = gFPS			# interpolator 1 as 1 second translation speed
				gZoomRate = gFPS			# interpolator 1 as 1 second zoom speed
				msg = "[102][1.0][%d]" % (1.0 * gZoomRate)
				outputs.put(msg)
				
		return 0
