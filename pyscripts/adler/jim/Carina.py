from LLActivityPlugin import *
import random
from math import *
import sys

from socket import socket, AF_INET, SOCK_STREAM
import threading
import Queue
import time
import wx

###############################################################################
# Some Global Variables
###############################################################################
# dataset define: (x res, y res, scale, screen trans x, screen trans y, thumbnail)
# note: trans y affected by magicarpet dataset configuratin (mirror like)
#       current dataset use horizontal mirror(y translation * -1)
# james mac
#gDataSets = [(29566.0, 14321.0, 0.05000, -900.0, 0.0, 'carina_coord_sample.jpg'), \
#			 (32195.0, 19464.0, 0.03679,  900.0, 0.0, 'cd.jpg'), \
#			 (369032.0, 13520.0, 0.01, 0.0, -1000.0, 'sp.jpg') ]
#gHost = '131.193.77.190'
#gPort =	7110
#gScreenX = 1440.0
#gScreenY = 900.0

# nec wall
#gDataSets = [(29566.0, 14321.0, 0.12, -900.0, 0.0, 'carina_coord_sample.jpg'), \
#			 (32195.0, 19464.0, 0.0883,  900.0, 0.0, 'cd.jpg'), \
#			 (369032.0, 13520.0, 0.01, 0.0, -1000.0, 'sp.jpg') ]
#gHost = '131.193.77.124'
#gPort =	7110
#gScreenX = 4080.0
#gScreenY = 2304.0

# caesar
gDataSets = [(29566.0, 14321.0, 0.05000, -900.0, 0.0, 'carina_coord_sample.jpg'), \
			 (32195.0, 19464.0, 0.03679,  900.0, 0.0, 'cd.jpg'), \
			 (369032.0, 13520.0, 0.01, 0.0, -1000.0, 'sp.jpg') ]
gHost = '131.193.77.157'
gPort =	7110
gScreenX = 1920.0
gScreenY = 1080.0

gNumDataSet = 2
gDataSetRatio = [(1.0, 1.0), (1.0, 1.0), (1.0, 1.0)]	# image set screen size ratio compared to the first set
gDataRect = [wx.Rect(10, 10, 100, 100), wx.Rect(10, 10, 100, 100), wx.Rect(10, 10, 100, 100)]

gBBox = [0.0, 0.0, 0.0, 0.0]  		# centerX, centerY, width, height for the first dataset
gCamera = [0.0, 0.0, 1.0]			# camera locx, locy, zoom
gZoomStep = 0.5						# for relative zoom msg: multiply this value with current zoom level
gPanStep = gScreenX * 0.4			# for relative pan msg: use RE_ZOOM msg with this amount of pixel values

###############################################################################
# Some Global Variables
###############################################################################
# image pixel value when zoom is 1.0, offset x, offset y, ratiox, rationy compared to the first set
gImagePixel = []
gInitialzed = False					# got initial bbox
gROIZoomFactor = 4
gCalibrating = False
gCalibrated = False
gZoomRate = 1.0
gPanRate = 1.0
gFPS = 30.0

###############################################################################
# Region Of Interst description
###############################################################################
gROIDescription = [
"the center of image",
"Eta Carinae is a highly variable hypergiant blue star that is 100 times the mass of our sun, 100 times the radius of our sun, and 4 million times more luminous. Eta Carinae is one of the most massive stars and only a few dozen stars in our galaxy are this large.",
"The Keyhole Nebula is a small dark cloud of cold molecules and dust, containing bright filaments of hot, fluorescing gas, silhouetted against the much brighter background nebula.",
"The Homunculus Nebula is an emission nebula surrounding the massive star Eta Carinae. The Homunculus is believed to have been ejected in an enormous outburst from Eta Carinae which was visible from Earth back in the 1840s.",
"The Trumpler 14 Star Cluster is a region of massive star formation in the Kuhreenah Nebula.",
"Herbig Haro objects, such as HD <spell>666</spell> are emission nebulae that result from shocks in the outflowing jets from young stellar objects. These outflows are an integral part of the accretion process that is believed to form low- and intermediate-mass stars. Such jets give us a direct indication that star formation is still occurring nearby.",
"HD <spell>93250</spell> is another giant blue star in the Kuhreenah Nebula. Because of its large size it will have a very short lifetime."
]


###############################################################################
# whiteboard related urls
###############################################################################
# bookmarks
mapmark = "<bookmark mark=\"[whiteboard][URL][web/carina/carina_map.jpg]\"/>"
joystickmark = "<bookmark mark=\"[whiteboard][URL][web/carina/joystick.jpg]\"/>"
helmet0mark = "<bookmark mark=\"[ACTION][HELMET][0]\"/>"
helmet1mark = "<bookmark mark=\"[ACTION][HELMET][1]\"/>"
carina01mark = "<bookmark mark=\"[whiteboard][URL][web/carina/carina01.jpg]\"/>"
cdfsmark = "<bookmark mark=\"[whiteboard][URL][web/carina/cdfs.jpg]\"/>"

# url
titleurl = "web/carina/title.jpg"
mapurl = "web/carina/carina_map.jpg"
carina01url = "web/carina/carina01.jpg"
cdfsurl = "web/carina/cdfs.jpg"

# whiteboard contents
ROIcontents0 = "<p>Interesting Features</p><p id=\"c\">Eta Carinae</p><p id=\"c\">Keyhole Nebula</p><p id=\"c\">Homunculus Nebula</p><p id=\"c\">Trumpler 14</p><p id=\"c\">Herbig-Haro object</p><p id=\"c\">HD 93250</p>"

###############################################################################
# Tour Parser for simplifed flat file
###############################################################################
# parse tag
def parseTag(tag):
	# what type of tag it is?
	tokens = tag.split()
	newTag = ""
	if tokens[0] == 'nav':
		# nav x y zoom
		newTag = "<bookmark mark=\"[ACTIVITY][BRD][NAV:%s:%s:%s]\"/>" % (tokens[1], tokens[2], tokens[3])
	elif tokens[0] == 'move':
		# mov x y
		newTag = "<bookmark mark=\"[ACTIVITY][BRD][MOV:%s:%s:1.0]\"/>" % (tokens[1], tokens[2])
	elif tokens[0] == 'zoom':
		# zoom value
		newTag = "<bookmark mark=\"[ACTIVITY][BRD][ZOM:%s:1.0]\"/>" % (tokens[1])
	elif tokens[0] == 'pause':
		# pause secs
		newTag = "<silence msec=\"%d\"/>" % (int(float(tokens[1]) * 1000))
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
	fileIN = open(filename, "r")
	line = fileIN.readline()
	tourSpots = []
	spotID = 0
	
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
			name = line[5:]
			contents = fileIN.readline()
			tourSpots.append( [spotID, buildEntry(name, contents)] )
			spotID += 1
	
		# read next line
		line = fileIN.readline()
	
	fileIN.close()
	return tourSpots

###############################################################################
# Network msg queue
###############################################################################
outputs = Queue.Queue(0)			# network msg outgoing queue

###############################################################################
# Network msg receiver (thread)
###############################################################################
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
					vars = []
					vars = msg.split()
					#if len(vars) != 0:
					if len(vars) != 0 and vars[0] == '27':
						x0 = float(vars[1])
						x1 = float(vars[2])
						y0 = float(vars[3])
						y1 = float(vars[4].strip('\00'))
						
						global gCamera, gImagePixel, gDataSets, gDataRect
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

###############################################################################
# Network Client class
###############################################################################
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
		

###############################################################################
# translate image space coordinate to proper screen pixel translation value
# x: -0.5 ~ 0.5, y: -0.5 ~ 0.5 (within image area)
###############################################################################
def loc2pixel(setid, x, y):
	global gCamera, gImagePixel, gDataSets
	pv = [0.0, 0.0]
	#pv[0] = -gImagePixel[setid][0] * x * gCamera[2] * 2.0 - gDataSets[setid][3]*gCamera[2]
	#pv[1] = -gImagePixel[setid][1] * y * gCamera[2] * 2.0 - gDataSets[setid][4]*gCamera[2]
	
	# do not use zoom level (gCamera[2]) here. Just use absolution value @ zoom 1.0
	# then, mc will take care of it
	pv[0] = -gImagePixel[setid][0] * x * 2.0 - gDataSets[setid][3]
	pv[1] = -gImagePixel[setid][1] * y * 2.0 - gDataSets[setid][4]
	return pv

###############################################################################
# Navigator Class
###############################################################################
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

		
		# need to add region of interests [setid, x, y, radius, name]
		# all local coordinate with dataset: ratio values
		# Carina Nebula ROI
		roi = [[0.0, 0.0, 0.1, 'the center of Kuhreenah nebula']]
		roi.append([-0.2770,  0.0030, 0.0240, 'Eta Carinae'])
		roi.append([-0.1680, -0.0100, 0.1280, 'Keyhole Nebula'])
		roi.append([-0.27643, 0.014705, 0.0400, 'Homunculus Nebula'])
		roi.append([ 0.1950,  0.0880, 0.0820, 'Trumper 14'])
		roi.append([ 0.2650, -0.0920, 0.1200, 'Herbig Haro'])
		roi.append([ 0.0060, -0.2320, 0.0420, 'HD <spell>93250</spell>'])
		self.ROI = []
		self.ROI.append(roi)
		# CDFS ROI
		roi = [[0.0, 0.0, 0.1, 'the center of CDFS']]
		self.ROI.append(roi)
		
		# auto tour sequences: list of [id, speech]
		self.tourSet = []
		self.tourSet.append( parseTourFile("../../pyscripts/adler/jim/carina.tour") )
		self.tourSet.append( parseTourFile("../../pyscripts/adler/jim/cdfs.tour") )
		
		self.tournodes = []			# list of [time, x, y, zoom, speech]
		self.tourtimer = 0.0		# per tour node timer
		self.lastvisited = 0
		self.currentvisiting = 0
		self.moving = False
		self.zooming = False
		self.navigating = False
		self.nearest = -1
		self.pilotdecay = 0.0
		self.pilots = []			# three tuple set [type, value1, value2] [zoom, 1.5, 0.0] [move, x, y]
		
	def loadDataSet(self, filename):
		"""
		file = open(filename, 'r')
		line = file.readline()
		vars = []
		self.tourSet = []
		while len(line) != 0:
			vars = line.split('|')
			if vars[0] == 'Dataset':
				num = int(vars[1])
				# nested loop for each set
				line = file.readline()		# must be Set
				line = file.readline()		# Tour|duration|x|y|zoom|speech
				vars = line.split('|')
				while True:
					tour = []
					tour.append([float(vars[1]), float(vars[2]), float(vars[3]), vars[4]])
					line = file.readline()
					vars = line.split('|')
				
			line = file.readline()
		"""
		pass
		
	def zoomInt(self, z, int):
		if z < 0.0:
			return
		
		# this only use for absolute zoom level
		msg = "[102][%f][%i]" % (z, int)
		outputs.put(msg)
		
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
		z = 1.0 / (self.ROI[setid][id][2] * gROIZoomFactor)
		self.navigateTo(self.ROI[setid][id][0], self.ROI[setid][id][1], z)
	
	def navigateToTarget(self):
		if self.targetid != -1:
			self.navigateToROI(self.currentSet, self.targetid)
			self.targetid = -1
	
	def navigateToRecentSet(self):
		global gImagePixel, gScreenX
		z = gScreenX / gImagePixel[self.currentSet][0]
		self.navigateTo(self.ROI[self.currentSet][0][0], self.ROI[self.currentSet][0][1], z)
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
		if self.mode == 1:
			return self.currentSet
		
		# which dataset is focused : based on the center of screen
		# AABB test through existing sets. if none hit, then return -1
		# gCamera with dataset
		global gScreenX, gScreenY, gDataRect
		pt = [gScreenX * 0.5, gScreenY * 0.5]
		for n in range(gNumDataSet):
			if gDataRect[n].Inside(pt):
				self.currentSet = n
				return n
		
		return -1
		
	def searchInterests(self):
		# what am I looking at? what is this? kind of questions
		# use current location to search ROI
		found = [-1, -1, 'nothing', 0.0]	# [setid, id, name, distance from the center of ROI]
		
		id = -1
		mindist = 10000
		near = -1
		
		# check dataset first
		setid = self.searchDataset()
		if setid == -1:
			return found
		
		global gScreenX, gScreenY, gDataRect
		
		# relative coordinate within dataset
		Loc = [0.0, 0.0]
		rpt = gDataRect[setid].Get()
		Loc[0] = (gScreenX*0.5 - rpt[0]) / rpt[2] - 0.5
		Loc[1] = (gScreenY*0.5 - rpt[1]) / rpt[3] - 0.5
        
		# check ROI within focused dataset
		for spot in self.ROI[setid]:
			id += 1
			xx = Loc[0] - spot[0]
			yy = Loc[1] - spot[1]
			dd = sqrt(pow(xx,2)+pow(yy,2))
			
			# update nearest roi spot
			if dd < mindist:
				mindist = dd
				near = id
			
			# distance check: within radius?
			if dd < spot[2]:
				found = [setid, id, spot[3], dd]
				self.currentROI = id
				return found
		
		# nothing interesting in current location
		# then, return nearest one from the current location
		# [setid, id=-1, name, nearid]
		found[0] = setid
		found[2] = self.ROI[setid][near][3]
		found[3] = near
		self.targetid = near
		
		return found
		
	
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
		self.tourtimer = 0.0
		del self.pilots[:]
		del self.tournodes[:]
		# add tour stops
		for stop in self.tourSet[dataset]:
			self.tournodes.append(stop)
		
		# pop the fist one and start
		#stop = self.tournodes.pop(0)	# time, x, y, zoom, speech
		#self.tourtimer = stop[0]
		#self.owner.speak(stop[1])

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
		self.tourtimer = 0.0
		del self.pilots[:]
		del self.tournodes[:]
		
	def nextTour(self):
		if self.mode != 1:
			return
		
		if len(self.tournodes) != 0:
			# take the next tour stop
			stop = self.tournodes.pop(0)	# time, x, y, zoom, speech
			self.tourtimer = stop[0]
			self.owner.speaklisten(stop[1])
		else:
			# tour finished
			self.mode = 0
			self.tourtimer = 0.0
			self.owner.tourEnded()
		
	def update(self, addedTime):
		# update current location
		if self.mode == 0:
			self.current = [gCamera[0], gCamera[1], gCamera[2]]
		"""
		elif self.mode == 1:	# tour mode
			self.tourtimer -= addedTime
			if self.tourtimer < 0.0:
				if len(self.tournodes) != 0:
					# take the next tour stop
					stop = self.tournodes.pop(0)	# time, x, y, zoom, speech
					#self.navigateTo(stop[1], stop[2], stop[3])
					self.tourtimer = stop[0]
					self.owner.speak(stop[1])
				else:
					# tour finished
					self.mode = 0
					self.tourtimer = 0.0
					self.owner.tourEnded()
		elif self.mode == 2:	# tour suspended
			pass
		"""
		
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


###############################################################################
# Activity State Function
###############################################################################

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
			activity.speak(helmet0mark + carina01mark + "Hello, my name is Jim Lovell. I am your tour guide through these images of the universe." + joystickmark + "<silence msec=\"500\"/> you can use the joystick to move the image and zoom in and out. I can also take you on a tour.")
			activity.lastspeech = "I am your tour guide through these images of the universe.<silence msec=\"500\"/> you can use the joystick to move the image and zoom in and out. I can also take you on a tour."
			activity.setWBURL(carina01url, 20.0)
		elif set == 0:	# carina nebula
			activity.speak(helmet0mark + carina01mark + "Hello, my name is Jim Lovell. I am your tour guide through these images of the universe. This current image on the display is the Kuhreenah Nebula." + joystickmark + "<silence msec=\"500\"/> you can use the joystick to move the image and zoom in and out. I can also take you on a tour.")
			activity.lastspeech = "I am your tour guide through these images of the universe. This current image on the display is the Kuhreenah Nebula.<silence msec=\"500\"/> you can use the joystick to move the image and zoom in and out. I can also take you on a tour."
			activity.setWBURL(carina01url, 20.0)
		elif set == 1:	# CDFS
			activity.speak(helmet0mark + cdfsmark + "Hello, my name is Jim Lovell. I am your tour guide through these images of the universe. This current image on the display is the Chandra Deep Field South." + joystickmark + "<silence msec=\"500\"/> you can use the joystick to move the image and zoom in and out. I can also take you on a tour.")
			activity.lastspeech = "I am your tour guide through these images of the universe. This current image on the display is the Chandra Deep Field South.<silence msec=\"500\"/> you can use the joystick to move the image and zoom in and out. I can also take you on a tour."
			activity.setWBURL(cdfsurl, 20.0)
	
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
		activity.speak("<bookmark mark=\"[whiteboard][URL][web/carina/carina_sky.jpg]\"/>When looking up into the sky the Kuhreenah nebula looks like this photo next to me. On the wall you see a high-resolution false color image composed of 48 different photos taken by the Hubble Space Telescope. In this false color image Red shows sulfur, green shows hydrogen, and blue shows oxygen, making it easier to see the structure and components of the nebula.")
		activity.lastspeech = "When looking up into the sky the Kuhreenah nebula looks like this photo next to me. On the wall you see a high-resolution false color image composed of 48 different photos taken by the Hubble Space Telescope. In this false color image Red shows sulfur, green shows hydrogen, and blue shows oxygen, making it easier to see the structure and components of the nebula."
		activity.setWBURL(carina01url, 15.0)

# tour state function : tour
# automatic tour guide mode: jim controls navigation and explanations.
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
	
	if activity.rulename == 'MC_TOUR_WHERE':
		activity.speak("<bookmark mark=\"[whiteboard][URL][web/carina/carina_constellation.jpg]\"/>The Kuhreenah Nebula is roughly 7,500 light years away. It is located in the constellation Kuhreenah which is only visible in Southern Hemisphere, so we can not see it here in Chicago.")
		activity.lastspeech = "The Kuhreenah Nebula is roughly 7,500 light years away. It is located in the constellation Kuhreenah which is only visible in Southern Hemisphere, so we can not see it here in Chicago."
		activity.setWBURL(mapurl, 15.0)

# navigator to ROI
def MCNAVROI(activity, str):
	
	if activity.rulename == 'MC_ROI_HOWBIG':
		activity.speak("This photo shows an area that is 50 light years wide and 30 light years tall. When seen from the Earth the Kuhreenah Nebula looks as big as the full moon in the sky.")
		activity.lastspeech = "This photo shows an area that is 50 light years wide and 30 light years tall. When seen from the Earth the Kuhreenah Nebula looks as big as the full moon in the sky."
	elif activity.rulename == 'MC_ROI_PHOTO':
		activity.speak("<bookmark mark=\"[whiteboard][URL][web/carina/carina_sky.jpg]\"/>When looking up into the sky the Kuhreenah nebula looks like this photo next to me. On the display you see a high-resolution false colour image composed of 48 different photos taken by the Hubble Space Telescope. In this false color image Red shows sulfer, green shows hydrogen, and blue shows oxygen, making it easier to see the structure and components of the nebula.")
		activity.lastspeech = "When looking up into the sky the Kuhreenah nebula looks like this photo next to me. On the table you see a high-resolution false colour image composed of 48 different photos taken by the Hubble Space Telescope. In this false color image Red shows sulfer, green shows hydrogen, and blue shows oxygen, making it easier to see the structure and components of the nebula."
		activity.setWBURL(mapurl, 25.0)
	elif activity.rulename == 'MC_ROI_ETA':
		activity.speak("<bookmark mark=\"[whiteboard][URL][web/carina/eta01.jpg]\"/>Eta Carinae is a highly variable hypergiant blue star that is 100 times the mass of our sun, 80 to 180 times the radius of our sun, and 4 million times more luminous. Eta Carinae is one of the most massive stars known and only a few dozon stars in our galaxy are this large. In 1843, Eta Carinae exploded, briefly becoming one of the brightest stars in the southern sky, but the star survived. This explosion produced 2 lobes and a large disk of expanding matter moving outwards at 1.5 million miles per hour. Each of the knots of ejected material are roughly the same size as our solar system. Eta Carinae is expected to explode again and become a supernova within the next million years. <bookmark mark=\"[whiteboard][URL][web/carina/eta02.jpg]\"/> Here is a composite image showing visible light in blue, X-Ray in orange from the hubble and Chandra telescopes. In February 2008, evidence was found that Eta Carinae is actually a double star.")
		activity.lastspeech = "<bookmark mark=\"[whiteboard][URL][web/carina/eta01.jpg]\"/>Eta Carinae is a highly variable hypergiant blue star that is 100 times the mass of our sun, 80 to 180 times the radius of our sun, and 4 million times more luminous. Eta Carinae is one of the most massive stars known and only a few dozon stars in our galaxy are this large. In 1843, Eta Carinae exploded, briefly becoming one of the brightest stars in the southern sky, but the star survived. This explosion produced 2 lobes and a large disk of expanding matter moving outwards at 1.5 million miles per hour. Each of the knots of ejected material are roughly the same size as our solar system. Eta Carinae is expected to explode again and become a supernova within the next million years. <bookmark mark=\"[whiteboard][URL][web/carina/eta02.jpg]\"/> Here is a composite image showing visible light in blue, X-Ray in orange from the hubble and Chandra telescopes. In February 2008, evidence was found that Eta Carinae is actually a double star."
		activity.navigator.navigateToROI(0, 1)
		activity.setWBURL(mapurl, 67.0)
	elif activity.rulename == 'MC_ROI_KEYHOLENEBULA':
		activity.speak("<bookmark mark=\"[whiteboard][URL][web/carina/keyhole01.jpg]\"/>This is NGC <spell>3324</spell>. The Keyhole Nebula. It is 7 light years in diameter. <bookmark mark=\"[whiteboard][URL][web/carina/keyhole02.jpg]\"/>Here is another photo of the Keyhole Nebula taken by the Hubble Space Telescope. The Keyhole Nebula is a small dark cloud of cold molecules and dust, containing bright filaments of hot, fluorescing gas, silhouetted against the much brighter background nebula.")
		activity.lastspeech = "<bookmark mark=\"[whiteboard][URL][web/carina/keyhole01.jpg]\"/>This is NGC <spell>3324</spell>. The Keyhole Nebula. It is 7 light years in diameter. <bookmark mark=\"[whiteboard][URL][web/carina/keyhole02.jpg]\"/>Here is another photo of the Keyhole Nebula taken by the Hubble Space Telescope. The Keyhole Nebula is a small dark cloud of cold molecules and dust, containing bright filaments of hot, fluorescing gas, silhouetted against the much brighter background nebula."
		activity.navigator.navigateToROI(0, 2)
		activity.setWBURL(mapurl, 30.0)
	elif activity.rulename == 'MC_ROI_HOMUNCULUS':
		activity.speak("The Homunculus Nebula is an emission nebula surrounding the massive star Eta Carinae. The Homunculus is believed to have been ejected in an enormous outburst from Eta Carinae which was visible from Earth back in 1843.")
		activity.lastspeech = "The Homunculus Nebula is an emission nebula surrounding the massive star Eta Carinae. The Homunculus is believed to have been ejected in an enormous outburst from Eta Carinae which was visible from Earth back in 1843."
		activity.navigator.navigateToROI(0, 3)
	elif activity.rulename == 'MC_ROI_TRUMPLER14':
		activity.speak("The Trumpler 14 Star Cluster is a region of massive star formation in the Kuhreenah Nebula.")
		activity.lastspeech = "The Trumpler 14 Star Cluster is a region of massive star formation in the Kuhreenah Nebula."
		activity.navigator.navigateToROI(0, 4)
	elif activity.rulename == 'MC_ROI_HERBIG':
		activity.speak("Herbig Haro objects, such as HD <spell>666</spell> are emission nebulae that result primarily from shocks in the outflowing jets from young stellar objects. These outflows are an integral part of the accretion process that is believed to form low- and intermediate-mass stars. Such jets give us a direct indication that star formation is still occurring nearby.")
		activity.lastspeech = "Herbig Haro objects, such as HD <spell>666</spell> are emission nebulae that result primarily from shocks in the outflowing jets from young stellar objects. These outflows are an integral part of the accretion process that is believed to form low- and intermediate-mass stars. Such jets give us a direct indication that star formation is still occurring nearby."
		activity.navigator.navigateToROI(0, 5)
	elif activity.rulename == 'MC_ROI_HD93250':
		activity.speak("HD <spell>93250</spell> is another giant blue star in the Kuhreenah Nebula. Because of its large size it will have a very short lifetime.")
		activity.lastspeech = "HD <spell>93250</spell> is another giant blue star in the Kuhreenah Nebula. Because of its large size it will have a very short lifetime."
		activity.navigator.navigateToROI(0, 6)
	
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

	# if in tour mode, stop it.
	#activity.navigator.stopTour()
	#activity.stopTour()
	
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
	# check the current dataset and ROI
	roi = activity.navigator.searchInterests()
	if roi[0] == -1:	# no dataset focused
		speech = "I don't see anything interesting here. Let's take a look at one of the images."
		activity.speak(speech)
		activity.lastspeech = speech
		activity.setWBURL(carina01url, 15.0)
		activity.navigator.navigateToRecentSet()
		# prevent changing state function
		return
	elif roi[0] == 0:	# carina nebula
		if roi[1] == -1:		#nothing
		
			if gCamera[2] < 1.5:
				#this is the first time question what from the initial state
				activity.speak(carina01mark + "This is NGC <spell>3372</spell>. the Kuhreenah Nebula. <silence msec=\"500\"/>It is the largest diffuse nebula in the sky, bigger and brighter than the orion nebula.<silence msec=\"500\"/> There are several interesting features in this image. I can tell you about the Kuhreenah Nebula, the star Eta Carinae, the Keyhole Nebula, the Homunculus Nebula, or I can take you on a tour.")
				activity.lastspeech = "This is NGC <spell>3372</spell>. the Kuhreenah Nebula. <silence msec=\"500\"/>It is the largest diffuse nebula in the sky, bigger and brighter than the orion nebula.<silence msec=\"500\"/> There are several interesting features in this image. I can tell you about the Kuhreenah Nebula, the star Eta Carinae, the Keyhole Nebula, the Homunculus Nebula, or I can take you on a tour."
				activity.nextwhiteboard = ''
			else:
				speech = "I'm sorry, I don't see any interesting features here. The nearest one is " + roi[2] + "Do you want me to take you there?"
				activity.lastspeech = "I'm sorry, I don't see any interesting features here. The nearest one is " + roi[2] + "Do you want me to take you there?"
				activity.speak(speech)
				# yes/no question: some state change necessary...
				activity.questionresponse[0] = 'OK. No problem'
				activity.questionresponse[1] = 'OK. Take your time. Just let me know if you need help.'
				activity.setQuestionState('YESNO', 'GOTOROI','STATE_INTERACTIVE')
		
		elif roi[1] == 0:		#near center
			if gCamera[2] < 3.0:		# user looking whole picture
				activity.speak(carina01mark + "This is NGC <spell>3372</spell>. the Kuhreenah Nebula. <silence msec=\"500\"/>It is the largest diffuse nebula in the sky, bigger and brighter than the orion nebula.")
				activity.lastspeech = "This is NGC <spell>3372</spell>. the Kuhreenah Nebula."
			else:
				activity.speak("you are looking at the " + roi[2])
				activity.lastspeech = "you are looking at the " + roi[2]
		else:
			activity.speak("you are looking at the " + roi[2] + "<silence msec=\"500\"/> Do you want to know more about it?")
			activity.lastspeech = "you are looking at the " + roi[2] + "<silence msec=\"500\"/> Do you want to know more about it?"
			# yes/no question
			activity.questionresponse[0] = 'Good.'
			activity.questionresponse[1] = 'OK.'
			activity.setQuestionState('YESNO', 'EXPLAINROI','STATE_INTERACTIVE')
	elif roi[0] == 1: 	# CDFS
		if gCamera[2] < 3.0:
			activity.speak("This is the Chandra Deep Field South image. I can take you on a tour if you wish.")
			activity.lastspeech = "This is the Chandra Deep Field South image. I can take you on a tour if you wish."
		else:
			activity.speak("You are look at part of the Chandra Deep Field South image. I do not have any specific interesting area here but I can take you on a tour.")
			activity.lastspeech = "You are look at part of the Chandra Deep Field South image. I do not have any specific interesting area here but I can take you on a tour."

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
			speech = gROIDescription[activity.navigator.currentROI]
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
		if set == 0:	# carina nebula
			speech = "In the Kuhreenah Nebula Image, your location is x coordinate %2.2f, y coordinate %2.2f, zoom level %2.1f" % (coord[0], coord[1], gCamera[2])
			activity.speak(speech)
			activity.lastspeech = speech
		elif set == 1:	# cdfs
			speech = "In the Chandra Deep Field South Image, your location is x coordinate %2.2f, y coordinate %2.2f, zoom level %2.1f" % (coord[0], coord[1], gCamera[2])
			activity.speak(speech)
			activity.lastspeech = speech
		

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
		
	if set == 0:	# carina nebula
		activity.speak("I can tell you about the Kuhreenah Nebula, the star Eta Carinae, the Keyhole Nebula, the Homunculus Nebula, or I can take you on a tour.")
		activity.lastspeech = "I can tell you about the Kuhreenah Nebula, the star Eta Carinae, the Keyhole Nebula, the Homunculus Nebula, or I can take you on a tour."
		activity.setWBContents(ROIcontents0)
		activity.setWBURL(mapurl, 15.0)
	elif set == 1:	# CDFS
		activity.speak("I don't have any specific topics for the Chandra Deep Field South image. But, I can take you on a tour.")
		activity.lastspeech = "I don't have any specific topics for the Chandra Deep Field South image. But, I can take you on a tour."

	# For the idle call, let's set the current action(function) to interactive steate function
	activity.action = MCINTERACTIVEStateFunc

def MCROI(activity, str):
	# user request: what are the interesting features here?
	# where am I looking at?
	set = activity.navigator.searchDataset()
	if set == -1:	# nothing
		# if looking at the empty space, move to least recent dataset and continue explanation.
		set = activity.navigator.navigateToRecentSet()
		"""
		speech = "I don't see anything interesting here. Let's take a look at on of the images."
		activity.speak(speech)
		activity.lastspeech = "I don't see anything interesting here. Let's take a look at on of the images."
		"""
		
	if set == 0:	# carina nebula
		activity.speak("There are six interesting features here. The star Eta Carinae, the Keyhole Nebula, the Homunculus Nebula, Trumpler 14, Herbig Haro, and HD <spell>93250</spell>.")
		activity.lastspeech = "There are six interesting features here. The star Eta Carinae, the Keyhole Nebula, the Homunculus Nebula, Trumpler 14, Herbig Haro, and HD <spell>93250</spell>."
		activity.setWBContents(ROIcontents0)
		activity.setWBURL(mapurl, 15.0)
	elif set == 1:	# CDFS
		activity.speak("I don't have any specific features in the Chandra Deep Field South image. But, I can take you on a tour.")
		activity.lastspeech = "I don't have any specific features in the Chandra Deep Field South image. But, I can take you on a tour."

	# For the idle call, let's set the current action(function) to interactive steate function
	activity.action = MCINTERACTIVEStateFunc
	
def MCStartTour(activity, str):
	if activity.touring == False:
		if activity.navigator.currentSet == 0:
			activity.speak(mapmark + "OK. Let's start. Feel free to interrupt me <bookmark mark=\"[ACTIVITY][BRD][TOUR]\"/>at any moment.")
		else:
			activity.speak(cdfsmark + "OK. Let's start. Feel free to interrupt me <bookmark mark=\"[ACTIVITY][BRD][TOUR]\"/>at any moment.")
			
		activity.lastspeech = "I told you feel free to interrupt me at any moment."
		activity.nextwhiteboard = ''
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
		if activity.navigator.currentSet == 0:
			activity.setWBURL(carina01url, 0.0)
		else:
			activity.setWBURL(cdfsurl, 0.0)
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

	
###############################################################################
# Activity Class Derived from C++
###############################################################################
class PythonActivity( LLActivityBase ):

	def initialize( self ):

		# Map (input, current_state) --> (action, next_state)
		# action is state related function assigned to it
		self.state_transitions = {}
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
		
		# register transition to self from ActivityManager
		# use ':' as delim for multiple recognizable inputs
		self.registerTransition("Carina Application")
		
		# initialize all states and its function
		# step1: add grammar -> add default transition
		# step2: add rule -> add transition with rule id
		#  repeat step2 till all required transitions added
		
		# Initial state (initial one)
		gramid = self.addGrammar("MC_STATE_INI")
		self.currentGrammarID = gramid
		self.grammarIDs = {'STATE_INI':gramid}
		ruleid = self.addGrammarRule(gramid, "MC_INI_ATTENTION", "hi:hey:hello:hi there:hey jim")
		self.addTransition(ruleid, 'STATE_INI', MCINIStateFunc, 'STATE_INTERACTIVE')
		self.state_tip['STATE_INI'] = "Hello\nHi, there.\nHey, Jim."
		
		# STATE_INTRACTIVE state (): interaction with image and avatar
		gramid = self.addGrammar("MC_STATE_INTERACTIVE")
		self.grammarIDs['STATE_INTERACTIVE'] = gramid
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_ATTENTION", "hey:jim:hey jim")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCINTERACTIVEStateFunc, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_WHAT", "what is this:where is this:what am I looking at:what's on the display:what's that")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCWhatIsThis, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_PHOTO", "tell me more about this photo:tell me more about the picture:where did this photo come from")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCINTERACTIVEStateFunc, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_SOMETHING", "something:show me something")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCShowSomething, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_ROI", "interesting features:what are the interesting features here")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCROI, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_WHERE", "where is it:where is it located:how far away is it:where is the carina nebula")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCINTERACTIVEStateFunc, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_HOWBIG", "how big is it:how big is the nebula")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCINTERACTIVEStateFunc, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_TOUR", "tour:tour please:take me on a tour:give me the tour")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCStartTour, 'STATE_TOUR')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_REPEAT", "repeat:say it again:what:sorry")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCRepeat, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_RESETVIEW", "reset view:show the whole picture:start over")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCResetView, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_INTERACTIVE_EXIT", "exit:good bye")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCExit, 'STATE_INI')
		
		# navigate to ROI
		ruleid = self.addGrammarRule(gramid, "MC_ROI_KEYHOLENEBULA", "keyhole nebula:tell me about keyhole nebula:show me the keyhole nebula:where is the keyhole nebula")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCNAVROI, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_ROI_HOMUNCULUS", "homunculus:homunculus nebula:tell me about the homunculus nebula:show me the homunculus nebula:Where is the homunculus nebula")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCNAVROI, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_ROI_ETA", "eta carinae:tell me about eta carinae:show me eta carinae:where is eta carinae")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCNAVROI, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_ROI_TRUMPLER14", "trumpler:trumpler fourteen:tell me about trumpler fourteen:show me trumpler fourteen:where is trumpler fourteen")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCNAVROI, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_ROI_HERBIG", "herbig haro objects:tell me about herbig haro objects:show me herbig haro objects:where are the herbig haro objects")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCNAVROI, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_ROI_HD93250", "HD nine three two five zero:tell me about HD nine three two five zero:show me HD nine three two five zero:where is HD nine three two five zero")
		self.addTransition(ruleid, 'STATE_INTERACTIVE', MCNAVROI, 'STATE_INTERACTIVE')
		
		self.state_tip['STATE_INTERACTIVE'] = "What is this?\nTell me more about this photo.\nWhat are the interesting features here?\nTake me on a tour."
		
		# STATE_TOUR state (): tour
		gramid = self.addGrammar("MC_STATE_TOUR")
		self.grammarIDs['STATE_TOUR'] = gramid
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_STOPTOUR", "stop:stop tour")
		self.addTransition(ruleid, 'STATE_TOUR', MCStopTour, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_PAUSETOUR", "pause tour:hold on")
		self.addTransition(ruleid, 'STATE_TOUR', MCPauseTour, 'STATE_TOUR_PAUSE')
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_EXIT", "exit:good bye")
		self.addTransition(ruleid, 'STATE_TOUR', MCExit, 'MC_STATE_INI')
		
		self.state_tip['STATE_TOUR'] = "Stop Tour\nPause Tour\nExit or Good-bye."
		
		# STATE_TOUR_PAUSED state (): tour paused
		gramid = self.addGrammar("MC_STATE_TOUR_PAUSED")
		self.grammarIDs['STATE_TOUR_PAUSE'] = gramid
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_PAUSE_RESUMETOUR", "resume:resume tour")
		self.addTransition(ruleid, 'STATE_TOUR_PAUSE', MCResumeTour, 'STATE_TOUR')
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_STOPTOUR", "stop:stop tour")
		self.addTransition(ruleid, 'STATE_TOUR_PAUSE', MCStopTour, 'STATE_INTERACTIVE')
		ruleid = self.addGrammarRule(gramid, "MC_TOUR_PAUSE_EXIT", "exit:good bye")
		self.addTransition(ruleid, 'STATE_TOUR_PAUSE', MCExit, 'MC_STATE_INI')
		self.state_tip['STATE_TOUR_PAUSE'] = "Resume Tour\nStop Tour\nExit"
		
		# Yes/No state: need some exception handling...
		# need to know what the question was
		gramid = self.addGrammar("MC_YESNO")
		self.grammarIDs['YESNO'] = gramid
		ruleid = self.addGrammarRule(gramid, "MC_YES", "yes:yes please:sure:why not:yes I do:please do")
		self.addTransition(ruleid, 'YESNO', MCYesNo, 'YESNO')
		ruleid = self.addGrammarRule(gramid, "MC_NO", "no:no thanks:no thank you:never mind:not at all:no I don't")
		self.addTransition(ruleid, 'YESNO', MCYesNo, 'YESNO')
		ruleid = self.addGrammarRule(gramid, "MC_YESNO_REPEAT", "repeat:say it again:what:sorry")
		self.addTransition(ruleid, 'YESNO', MCYesNo, 'YESNO')
		self.state_tip['YESNO'] = "Yes or No."
		
		# idle state: temporary state - tighten SR (only allow attention call)
		# similar to yes/no state, this needs to remember the last state
		# so that it can go back to the last one when it gets attention
		gramid = self.addGrammar("MC_IDLE")
		self.grammarIDs['IDLE'] = gramid
		ruleid = self.addGrammarRule(gramid, "MC_IDLE_ATTENTION", "hey:hey jim:jim")
		self.addTransition(ruleid, 'IDLE', MCIdle, 'IDLE')
		
		# Speech Pool: avoid unnatural repetition of speech sentences
		# Attention Pool: when user say "Hey, Jim"
		self.tourattentionspeech = SpeechPool()
		self.tourattentionspeech.add("Yes.")
		self.tourattentionspeech.add("Yes, I'm here.")
		self.tourattentionspeech.add("Yes, what do you want?")
		self.tourattentionspeech.add("Yes, how can I help you?")
		
		# some prefix for repeat request
		self.repeatspeech = SpeechPool()
		self.repeatspeech.add("OK. <silence msec=\"500\"/>")
		
		# Navigator & Network stuff
		self.navigator = MCNavigator(self)
		self.client = Client(self.navigator)

		
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
		
		if time == 0.0:
			self.nextwhiteboard = ''
			self.whiteboardtimer = 0.0
			self.setWhiteboardURL(url)
		else:
			self.nextwhiteboard = url
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
		
		# special consideration for whiteboard
		if self.touring == True:
			self.setWBURL(mapurl, 5.0)
		else:
			self.setWBURL(carina01url, 5.0)

	def idle (self):
		# assumption: user is still there, interaction with magicarpet
		# how jim know if there is interaction going or not (user's presence)?
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
				self.navigator.navigateTo(x, y, z)
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
		
		
	def tourEnded (self):
		# this only be called by navigator when its touring is ended
		self.touring = False
		
		# some other necessary stuff: state change?
		# when auto pilot tour ends, go back to interactive user mode
		self.reset()
		self.current_state = 'STATE_INTERACTIVE'
		self.currentGrammarID = self.grammarIDs['STATE_INTERACTIVE']
		self.active = True
		self.setCurrentGrammar(self.currentGrammarID)
		
		# tips
		tips = self.state_tip[self.current_state]
		if tips is not None:
			self.setTipString(tips, 10.0)

		
	def update (self, addedTime):
		
		# tick function call from C++
		"""
		if self.active is True:
			listening = self.isListening()
			if listening is True:
				self.elapsedIdle += addedTime
				# check overall idle time (should not accumulate speech time)
				if self.elapsedIdle > self.randIdle:
					self.elapsedIdle = 0
					self.idleCounter += 1
					# spit out some trash talk
					if self.action is not None:
						self.action(self, 'Bored')
					else:
						if self.idleCounter > 2:
							# set to initial state
							self.reset()
							self.active = True
							self.setCurrentGrammar(self.currentGrammarID)
							self.idleCounter = 0
						else:
							self.speak("Hey, anybody there? I get bored. Talk to me something.")
					# renew next random time
					#self.randIdle = random.uniform(100, 120)
					self.randIdle = random.uniform(5, 7)
		"""
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
				#speech = "Calibration is done. zoom facotr is %2.3f and pan factor is %2.3f" % (gZoomRate, gPanRate)
				#self.speak(speech)
				msg = "[102][1.0][%d]" % (1.0 * gZoomRate)
				outputs.put(msg)
				
		return 0
