#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Sep 14 14:55:25 2017

@author: gc1y17
"""

from dynamic_binaural_renderer_serial import DynamicBinauralRendererSerial
import visr
import rcl
import time
import rrl
import audiointerfaces as ai
from extractDelayInSofaFile import extractDelayInSofaFile

import os
from urllib.request import urlretrieve
from sys import platform

class DynamicBinauralRendererSAW( visr.CompositeComponent ):
         def __init__( self,
                     context, name, parent,
                     numberOfObjects,
                     port,
                     baud,
                     sofaFile,
                     enableSerial = True,
                     dynamicITD = True,
                     dynamicILD = True,
                     hrirInterpolation = False,
                     udpReceivePort=4242,
                     headTrackingCalibrationPort = None,
                     filterCrossfading = False
                     ):
            super( DynamicBinauralRendererSAW, self ).__init__( context, name, parent )
            self.dynamicBinauralRenderer = DynamicBinauralRendererSerial( context, "DynamicBinauralRenderer", self,
                                                                     numberOfObjects,
                                                                     port,baud,
                                                                     sofaFile,
                                                                     enableSerial = enableSerial,
                                                                     dynITD = dynamicITD,
                                                                     dynILD = dynamicILD,
                                                                     hrirInterp = hrirInterpolation,
                                                                     headTrackingCalibrationPort=headTrackingCalibrationPort,
                                                                     filterCrossfading=filterCrossfading
                                                                   )

            self.sceneReceiver = rcl.UdpReceiver( context, "SceneReceiver", self,
                                             port=udpReceivePort,
                                             mode=rcl.UdpReceiver.Mode.Asynchronous )
            self.sceneDecoder = rcl.SceneDecoder( context, "SceneDecoder", self )
            self.parameterConnection( self.sceneReceiver.parameterPort("messageOutput"),
                                 self.sceneDecoder.parameterPort("datagramInput") )
            self.parameterConnection( self.sceneDecoder.parameterPort( "objectVectorOutput"),
                                 self.dynamicBinauralRenderer.parameterPort("objectVector"))

            self.objectSignalInput = visr.AudioInputFloat( "audioIn", self, numberOfObjects )
            self.binauralOutput = visr.AudioOutputFloat( "audioOut", self, 2 )

            self.audioConnection(  self.objectSignalInput, self.dynamicBinauralRenderer.audioPort("audioIn"))
            self.audioConnection( self.dynamicBinauralRenderer.audioPort("audioOut"), self.binauralOutput)

            if headTrackingCalibrationPort is not None:
                self.calibrationTriggerReceiver = rcl.UdpReceiver( context, "CalibrationTriggerReceiver", self, port = headTrackingCalibrationPort )
                self.parameterConnection( self.calibrationTriggerReceiver.parameterPort("messageOutput"),
                                         self.dynamicBinauralRenderer.parameterPort("headTrackingCalibration"))


############ CONFIG ###############
fs = 48000
blockSize = 1024
numBinauralObjects = 64
numOutputChannels = 2

# switch dynamic tracking on and off.
useTracking = True
useDynamicITD = True
useDynamicILD = False
useHRIRinterpolation = False
useCrossfading = False

if useTracking:
    headTrackingCalibrationPort=8889
else:
    headTrackingCalibrationPort=None

# TODO: Check and adjust port names for the individual system
if platform == 'linux' or platform == 'linux2':
    port = "/dev/ttyUSB0"
elif platform == 'darwin':
    port = "/dev/cu.usbserial-AJ03GSC8"
elif platform == 'windows':
    port = "COM10"

baud = 57600
###################################

context = visr.SignalFlowContext(blockSize, fs )

sofaFile = './data/dtf b_nh169.sofa'
if not os.path.exists( sofaFile ):
    urlretrieve( 'http://sofacoustics.org/data/database/ari%20(artificial)/dtf%20b_nh169.sofa',
                       sofaFile )
if useDynamicITD:
    sofaFileTD = './data/dtf b_nh169_timedelay.sofa'
    if not os.path.exists( sofaFileTD ):
        extractDelayInSofaFile( sofaFile, sofaFileTD )
    sofaFile = sofaFileTD

renderer = DynamicBinauralRendererSAW( context, "TopLevelRenderer", None,
                                      numBinauralObjects,
                                      port,
                                      baud,
                                      sofaFile,
                                      enableSerial = useTracking,
                                      dynamicITD = useDynamicITD,
                                      dynamicILD = False,
                                      hrirInterpolation = useHRIRinterpolation,
                                      headTrackingCalibrationPort = headTrackingCalibrationPort,
                                      filterCrossfading=useCrossfading
                                      )

result,messages = rrl.checkConnectionIntegrity( renderer )
if not result:
   print(messages)

flow = rrl.AudioSignalFlow( renderer )

aiConfig = ai.AudioInterface.Configuration( flow.numberOfCaptureChannels,
                                           flow.numberOfPlaybackChannels,
                                           fs,
                                           blockSize )

jackCfg = """{ "clientname": "BinRendererSAW",
  "autoconnect" : "false",
  "portconfig":
  {
    "capture":  [{ "basename":"inObj_", "externalport" : {} }],
    "playback": [{ "basename":"outBin_", "externalport" : {} }]
  }
}"""
aIfc = ai.AudioInterfaceFactory.create("Jack", aiConfig, jackCfg)

aIfc.registerCallback( flow )

aIfc.start()

print( "Rendering started." )
#
time.sleep(1)
i = input("Enter text (or Enter to quit): ")
if not i:
    aIfc.stop()
    aIfc.unregisterCallback()
    del aIfc
