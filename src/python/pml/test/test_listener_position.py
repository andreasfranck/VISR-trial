
import pytest

import pml

import numpy as np
import json

from scipy.spatial.transform import Rotation

def test_defaultInit():
    lp = pml.ListenerPosition()

    pos = lp.position
    assert np.linalg.norm( pos - np.zeros(3, dtype=np.float32) ) < np.finfo(np.float32).eps

    ypr = lp.orientationYPR
    assert np.linalg.norm( ypr - np.zeros(3, dtype=np.float32) ) < np.finfo(np.float32).eps

    assert np.abs( lp.yaw - ypr[0] ) < np.finfo(np.float32).eps
    assert np.abs( lp.pitch - ypr[1] ) < np.finfo(np.float32).eps
    assert np.abs( lp.roll - ypr[2] ) < np.finfo(np.float32).eps

def test_positionInit():
    posInit = np.array( [ 0.3, -0.25, 1.35 ])

    lp = pml.ListenerPosition(posInit)

    pos = lp.position
    assert np.linalg.norm( pos - posInit ) < np.finfo(np.float32).eps

    ypr = lp.orientationYPR
    assert np.linalg.norm( ypr - np.zeros(3, dtype=np.float32) ) < np.finfo(np.float32).eps

    assert np.abs( lp.yaw - ypr[0] ) < np.finfo(np.float32).eps
    assert np.abs( lp.pitch - ypr[1] ) < np.finfo(np.float32).eps
    assert np.abs( lp.roll - ypr[2] ) < np.finfo(np.float32).eps

def test_yprInit():
    posInit = np.array( [ 0.3, -0.25, 1.35 ])
    yprInit = np.deg2rad(np.array([15, -45, -90]))

    lp = pml.ListenerPosition(posInit, yprInit )

    pos = lp.position
    assert np.linalg.norm( pos - posInit ) < np.finfo(np.float32).eps

    tol = 4*np.finfo(np.float32).eps # Reasonable error limit

    ypr = lp.orientationYPR
    assert np.linalg.norm( ypr - yprInit ) <tol

    assert np.abs( lp.yaw - yprInit[0] ) < tol
    assert np.abs( lp.pitch - yprInit[1] ) < tol
    assert np.abs( lp.roll - yprInit[2] ) < tol


def test_quaternionInit():
    posInit = np.array( [ 0.3, -0.25, 1.35 ])
    yprInit = np.deg2rad(np.array([15, -45, -90]))

    initQuat = pml.ypr2Quaternion( yprInit[0], yprInit[1], yprInit[2] )

    lp = pml.ListenerPosition(posInit, initQuat )

    pos = lp.position
    assert np.linalg.norm( pos - posInit ) < np.finfo(np.float32).eps

    tol = 4*np.finfo(np.float32).eps # Reasonable error limit

    ypr = lp.orientationYPR
    assert np.linalg.norm( ypr - yprInit ) < tol

    assert np.abs( lp.yaw - yprInit[0] ) < tol
    assert np.abs( lp.pitch - yprInit[1] ) < tol
    assert np.abs( lp.roll - yprInit[2] ) < tol

def test_translation():
    posInit = np.array( [ 0.3, -0.25, 1.35 ])
    yprInit = np.deg2rad(np.array([30, -15, 35]))

    lp = pml.ListenerPosition(posInit, yprInit )

    tol = 4*np.finfo(np.float32).eps # Reasonable error limit

    translateVec = np.asarray([ -2.75, 1.83, -0.387 ])
    newPos = posInit + translateVec

    lp.translate( translateVec )

    # Check that the orientation is unchanged.
    assert np.linalg.norm(lp.orientationYPR - yprInit) < tol

    assert np.linalg.norm(lp.position-newPos) < tol


def test_rotateOrientation():
    posInit = np.array( [ 0.3, -0.25, 1.35 ])
    yprInit = np.deg2rad(np.array([15, 45, -30]))

    lp = pml.ListenerPosition(posInit, yprInit )

    tol = 4*np.finfo(np.float32).eps # Reasonable error limit

    ypr = lp.orientationYPR
    assert np.linalg.norm( ypr - yprInit ) < tol

    assert np.abs( lp.yaw - yprInit[0] ) < tol
    assert np.abs( lp.pitch - yprInit[1] ) < tol
    assert np.abs( lp.roll - yprInit[2] ) < tol

    rotateYpr = np.deg2rad( np.array([0,0,0]))
    rotateQuat = pml.ypr2Quaternion( rotateYpr )

    initRot= Rotation.from_euler('ZYX', yprInit)
    rotationRef = Rotation.from_euler('ZYX', rotateYpr)
    resRotRef = rotationRef * initRot
    resRotYPR = resRotRef.as_euler('ZYX')

    lp.rotateOrientation( rotateQuat )

    yprNew=lp.orientationYPR
    assert np.linalg.norm( yprNew - resRotYPR ) < tol


def test_rotation():
    posInit = np.array( [ 0.3, -0.25, 1.35 ])
    # posInit = np.array( [ 1.0, 0, 0 ])
    yprInit = np.deg2rad(np.array([15, 45, -30]))
    # yprInit = np.deg2rad(np.array([0, 0, 45]))

    lp = pml.ListenerPosition(posInit, yprInit )

    tol = 10.0*np.finfo(np.float32).eps # Reasonable error limit

    ypr = lp.orientationYPR
    assert np.linalg.norm( ypr - yprInit ) < tol

    assert np.abs( lp.yaw - yprInit[0] ) < tol
    assert np.abs( lp.pitch - yprInit[1] ) < tol
    assert np.abs( lp.roll - yprInit[2] ) < tol

    rotateYpr = np.deg2rad( np.array([-90, 30, -15]))
    rotateQuat = pml.ypr2Quaternion( rotateYpr )

    initRot= Rotation.from_euler('ZYX', yprInit)
    rotationRef = Rotation.from_euler('ZYX', rotateYpr)
    resRotRef = rotationRef * initRot
    resRotRefYPR = resRotRef.as_euler('ZYX')

    resPosition = rotationRef.apply( posInit )

    lp.rotate( rotateQuat )

    posNew = lp.position
    assert np.linalg.norm(posNew - resPosition ) < tol


    yprNew=lp.orientationYPR
    assert np.linalg.norm( yprNew - resRotRefYPR ) < tol

def test_transformation():
    posInit = np.array( [ 0.3, -0.25, 1.35 ])
    yprInit = np.deg2rad(np.array([15, 45, -30]))

    lp = pml.ListenerPosition(posInit, yprInit )

    tol = 10*np.finfo(np.float32).eps # Reasonable error limit, needs
    # some fine tuning to account for arithmetic errors.

    ypr = lp.orientationYPR
    assert np.linalg.norm( ypr - yprInit ) < tol

    assert np.abs( lp.yaw - yprInit[0] ) < tol
    assert np.abs( lp.pitch - yprInit[1] ) < tol
    assert np.abs( lp.roll - yprInit[2] ) < tol

    rotateYpr = np.deg2rad( np.array([25,-90,120]))
    rotateQuat = pml.ypr2Quaternion( rotateYpr )

    translateVec = np.asarray([ -2.75, 1.83, -0.387 ])

    initRot= Rotation.from_euler('ZYX', yprInit)
    rotationRef = Rotation.from_euler('ZYX', rotateYpr)
    resRotRef = rotationRef * initRot
    resRotRefYPR = resRotRef.as_euler('ZYX')

    resPosition = rotationRef.apply( posInit ) + translateVec

    lp.transform( rotateQuat, translateVec )

    posNew = lp.position
    assert np.linalg.norm(posNew - resPosition ) < tol


    yprNew=lp.orientationYPR
    assert np.linalg.norm( yprNew - resRotRefYPR ) < tol

def test_parseJson():
    initStrQuat = """{"x":"0.5","y":"-0.25","z":"-0.3",
    "orientation":{"quaternion":{"w":"0.871836424","x":"-0.285320133","y":"0.335270375","z":"0.214679867"}}}"""
    lp = pml.ListenerPosition()
    lp.parseJson( initStrQuat )


def test_fromJson():
    initStr = """{}"""

def test_writeJson():
    posInit=np.array( [ 0.3, -0.25, 1.35 ])
    yprInit=np.deg2rad(np.array([15, 45, -30]))
    lp=pml.ListenerPosition(posInit, yprInit )
    msg= lp.writeJson(ypr=False)

    rep = json.loads( msg )
    posRead = np.asarray([ rep["x"], rep["y"], rep["z"] ], dtype = posInit.dtype )

    assert np.linalg.norm( posRead - posInit ) < 1e-6

    assert "orientation" in rep.keys()
    assert "quaternion" in rep["orientation"].keys()
    quat = rep["orientation"]["quaternion"]

    quatRead =





# Allow running the tests as a script (as opposed to through pytest)
if __name__ == "__main__":
    test_defaultInit()
    test_positionInit()
    test_yprInit()
    test_quaternionInit()
    test_translation()
    test_rotateOrientation()
    test_rotation()
    test_transformation()
    test_fromJson()
    test_parseJson()
    test_writeJson()
