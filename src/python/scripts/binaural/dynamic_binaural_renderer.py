#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Sep 13 12:56:27 2017

@author: gc1y17
"""

import visr
import pml
import objectmodel as om
import rrl
import h5py
import rcl

#import objectmodel as om

import numpy as np;
import os
from readSofa import readSofaFile
from dynamic_binaural_controller import DynamicBinauralController
from urllib.request import urlretrieve

class DynamicBinauralRenderer( visr.CompositeComponent ):
    
        def __init__( self,
                     context, name, parent, 
                     numberOfObjects
                     ):
            super( DynamicBinauralRenderer, self ).__init__( context, name, parent )
            self.objectSignalInput = visr.AudioInputFloat( "audioIn", self, numberOfObjects )
            self.binauralOutput = visr.AudioOutputFloat( "audioOut", self, 2 )
            self.objectVectorInput = visr.ParameterInput( "objectVector", self, pml.ObjectVector.staticType,
                                                         pml.DoubleBufferingProtocol.staticType,
                                                         pml.EmptyParameterConfig() )
    
            self.trackingInput = visr.ParameterInput( "tracking", self, pml.ListenerPosition.staticType,
                                              pml.DoubleBufferingProtocol.staticType,
                                              pml.EmptyParameterConfig() )
            
         
            sofaFile = './data/dtf b_nh169.sofa'
            if not os.path.exists( sofaFile ):
                urlretrieve( 'http://sofacoustics.org/data/database/ari%20(artificial)/dtf%20b_nh169.sofa',
                       sofaFile )

# hrirFile = 'c:/local/s3a_af/subprojects/binaural/dtf b_nh169.sofa'

            [ hrirPos, hrirData ] = readSofaFile( sofaFile )
            self.dynamicBinauraController = DynamicBinauralController( context, "DynamicBinauralController", self,
                                                                      numberOfObjects,
                                                                      hrirPos, hrirData,
                                                                      useHeadTracking = True,
                                                                      dynamicITD = True,
                                                                      dynamicILD = True,
                                                                      hrirInterpolation = False
                                                                      )
            
            self.parameterConnection( self.objectVectorInput, self.dynamicBinauraController.parameterPort("objectVector"))
            self.parameterConnection( self.trackingInput, self.dynamicBinauraController.parameterPort("headTracking"))
            
           
            # Define the routing for the binaural convolver such that it matches the organisation of the
            # flat BRIR matrix.
            filterRouting = pml.FilterRoutingList()
            for idx in range(0, numberOfObjects ):
                filterRouting.addRouting( idx, idx, 2*idx, 1.0 )
                filterRouting.addRouting( idx, idx+numberOfObjects, 2*idx+1, 1.0 )
                
            firLength = hrirData.shape[1]
            self.convolver = rcl.FirFilterMatrix( context, 'covolutionEngine', self )
            self.convolver.setup( numberOfInputs=numberOfObjects,
                             numberOfOutputs=2*numberOfObjects,
                             maxFilters=2*numberOfObjects,
                             filterLength=firLength,
                             maxRoutings=2*numberOfObjects,
                             routings=filterRouting,
                             controlInputs=True
                             )
            self.audioConnection(self.objectSignalInput, self.convolver.audioPort("in") )
            self.parameterConnection(self.dynamicBinauraController.parameterPort("filterOutput"),self.convolver.parameterPort("filterInput") )

            blockSize = 16
            self.delayVector = rcl.DelayVector( context, "delayVector", self )
            self.delayVector.setup(numberOfObjects*2, interpolationType="lagrangeOrder3", initialDelay=0,
             controlInputs=True, 
             initialGain=1.0, 
             interpolationSteps=context.period)

            self.audioConnection( self.convolver.audioPort("out"), self.delayVector.audioPort("in"))

            self.adder = rcl.Add( context, 'add', self, numInputs = numberOfObjects, width=2)            


            for idx in range(0, numberOfObjects ):
                self.audioConnection( self.delayVector.audioPort("out"), visr.ChannelList([idx,idx+numberOfObjects]), self.adder.audioPort("in%d" % idx), visr.ChannelList([0,1]))                
                
                
            self.audioConnection( self.adder.audioPort("out"), self.binauralOutput)
            self.parameterConnection(self.dynamicBinauraController.parameterPort("delayOutput"),self.delayVector.parameterPort("delayInput") )