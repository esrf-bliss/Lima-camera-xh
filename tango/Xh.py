############################################################################
# This file is part of LImA, a Library for Image Acquisition
#
# Copyright (C) : 2009-2013
# European Synchrotron Radiation Facility
# BP 220, Grenoble 38043
# FRANCE
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################
#=============================================================================
#
# file :        Xh.py
#
# description : Python source for the Xh and its commands.
#                The class is derived from Device. It represents the
#                CORBA servant object which will be accessed from the
#                network. All commands which can be executed on the
#                Pilatus are implemented in this file.
#
# project :     TANGO Device Server
#
# copyleft :    European Synchrotron Radiation Facility
#               BP 220, Grenoble 38043
#               FRANCE
#
#=============================================================================
#         (c) - Bliss - ESRF
#=============================================================================
#
import PyTango
from Lima import Core
from Lima import Xh as XhAcq
# import some useful helpers to create direct mapping between tango attributes
# and Lima interfaces.
from Lima.Server import AttrHelper

#------------------------------------------------------------------
#------------------------------------------------------------------
#    class Xh
#------------------------------------------------------------------
#------------------------------------------------------------------

class Xh(PyTango.Device_4Impl):

    Core.DEB_CLASS(Core.DebModApplication, 'LimaCCDs')


#------------------------------------------------------------------
#    Device constructor
#------------------------------------------------------------------
    def __init__(self,*args) :
        PyTango.Device_4Impl.__init__(self,*args)

	
        self.__clockmode = {'XhInternalClock': 0,
			    'XhESRF5468Mhz': 1,
			    'XhESRF1136Mhz': 2}

        self.__Attribute2FunctionBase = {
            'aux_delay': 'AuxDelay',
            'trig_mux': 'TrigMux',
            'orbit_trigger': 'OrbitTrig',
            'correct_rounding': 'CorrectRounding',
            'group_delay': 'GroupDelay',
            'frame_delay': 'FrameDelay',
            'scan_period': 'ScanPeriod',
            'aux_delay': 'AuxDelay',
            'aux_width': 'AuxWidth',
            'nb_groups': 'NbGroups',
            'temperature': 'Temperature'
        }
			    
        self.init_device()

#------------------------------------------------------------------
#    Device destructor
#------------------------------------------------------------------
    def delete_device(self):
        pass

#------------------------------------------------------------------
#    Device initialization
#------------------------------------------------------------------
    @Core.DEB_MEMBER_FUNCT
    def init_device(self):
        self.set_state(PyTango.DevState.ON)
        self.get_device_properties(self.get_device_class())

#------------------------------------------------------------------
#    getAttrStringValueList command:
#
#    Description: return a list of authorized values if any
#    argout: DevVarStringArray   
#------------------------------------------------------------------
    @Core.DEB_MEMBER_FUNCT
    def getAttrStringValueList(self, attr_name):
        return AttrHelper.get_attr_string_value_list(self, attr_name)

#------------------------------------------------------------------
#    reset command:
#
#    Description: make a full reset: 
#    - call of init() method which run the config.cmd file
#      defined in the properties 
#    argout: DevVoid   
#------------------------------------------------------------------
    @Core.DEB_MEMBER_FUNCT
    def reset(self):
        _XhCam.reset()
		
#==================================================================
#
#    setHeadCaps command
#
#==================================================================
    @Core.DEB_MEMBER_FUNCT
    def setHeadCaps(self,argin):
        l = len(argin)
        capsAB = argin[0]
        capsCD = argin[1]
	
        print (l)
        print (capsAB)
        print (capsCD)
	
	
        _XhCam.setHeadCaps(capsAB,capsCD)
		
#==================================================================
#
#    setHeadCaps command
#
#==================================================================
    @Core.DEB_MEMBER_FUNCT
    def getAvailableCaps(self):
        capsList, _ = _XhCam.listAvailableCaps() 
        return capsList

#==================================================================
#
#    setCommand command
#
#==================================================================
    @Core.DEB_MEMBER_FUNCT
    def sendCommand(self,argin):
        cmd = argin

        print (cmd)
	
        _XhCam.sendCommand(cmd)

#==================================================================
#
#    setHighVoltageOn command
#
#==================================================================
    @Core.DEB_MEMBER_FUNCT
    def setHighVoltageOn(self):	
        _XhCam.setHighVoltageOn()


#==================================================================
#
#    setHighVoltageOff command
#
#==================================================================
    @Core.DEB_MEMBER_FUNCT
    def setHighVoltageOff(self):	
        _XhCam.setHighVoltageOff()
		

#==================================================================
#
#    setHeadDac command
#
#==================================================================
    @Core.DEB_MEMBER_FUNCT
    def setHeadDac(self,argin):
        _value = argin[0]
        _type = argin[1]
        _head = argin[2]
        _direct = argin[3]
	
        _XhCam.setHeadDac(_value, _type, _head, _direct)


#==================================================================
#
#    getAvailableTriggerModes command
#
#==================================================================
    @Core.DEB_MEMBER_FUNCT
    def getAvailableTriggerModes(self):	
        return _XhCam.getAvailableTriggerModes()


#==================================================================
#
#    setXhTimingScript command
#
#==================================================================

    @Core.DEB_MEMBER_FUNCT
    def setXhTimingScript(self, argin):
        """ Set timing script
        :param argin: script name
        """
        return _XhCam.setXhTimingScript(argin)


#==================================================================
#
#   getTemperature command
#   argin: channel number
#
#==================================================================

    @Core.DEB_MEMBER_FUNCT
    def getTemperature(self, argin):
        return  _XhCam.getTemperature(int(argin))


#==================================================================
#
#   setTimingOrbit command
#   argin[0]: delay
#   argin[1]: falling edge
#
#==================================================================
    @Core.DEB_MEMBER_FUNCT
    def setTimingOrbit(self, argin):
        delay = int(argin[0])
        falling_edge = bool(argin[1])
        return _XhCam.setTimingOrbit(delay, falling_edge)

#==================================================================
#
#    Xh read/write attribute methods
#
#==================================================================


    def __getattr__(self,name):
        try:
            return AttrHelper.get_attr_4u(self, name, _XhInterface)
        except:
            return AttrHelper.get_attr_4u(self, name, _XhCam)
	
	

#------------------------------------------------------------------
#    write clockmode:
#
#    Description: writes the clockMode
#    argin: DevString   
#------------------------------------------------------------------	

    def write_clockmode(self,attr):
        data = attr.get_write_value()
        print (data)
        clockmode = AttrHelper.getDictValue(self.__clockmode,data)
        print (clockmode)
        _XhCam.setupClock(clockmode)

#------------------------------------------------------------------
#    write nbscans:
#
#    Description: writes the nbscans 
#    argin: DevInt   
#------------------------------------------------------------------	

    def write_nbscans(self,attr):
        data = attr.get_write_value()
        nbscans = data
        print (nbscans)
        _XhCam.setNbScans(nbscans)


#------------------------------------------------------------------
#    read nbscans:
#
#    Description: reads the nbscans 
#    argin: DevInt   
#------------------------------------------------------------------	

    def read_nbscans(self,attr):
        nbscans = _XhCam.getNbScans()
        attr.set_value(nbscans)

#------------------------------------------------------------------
#    read maxframes:
#
#    Description: reads the maxframes 
#    argin: DevInt   
#------------------------------------------------------------------	

    def read_maxframes(self,attr):
        maxframes_s = _XhCam.getMaxFrames()
        maxframes = int(maxframes_s)
        attr.set_value(maxframes)


#------------------------------------------------------------------
#    read lemoout:
#
#    Description: reads the lemoout 
#------------------------------------------------------------------	

    def read_lemo_out(self, attr):
        lemosout = []
        _XhCam.getLemoOut(lemosout)
        attr.set_value(lemosout)

#------------------------------------------------------------------
#    write lemoout:
#
#    Description: writes the lemoout 
#------------------------------------------------------------------	

    def write_lemo_out(self, attr):
        data = attr.get_write_value()
        data = [int(el) for el in data]
        _XhCam.setLemoOut(data)

#------------------------------------------------------------------
#    write custom trigger mode:
#
#    Description: writes the custom trigger mode 
#    argin: DevString   
#------------------------------------------------------------------	

    def write_custom_trigger_mode(self, argin):
        data = argin.get_write_value()
        _XhCam.setCustomTriggerMode(data)

#------------------------------------------------------------------
#------------------------------------------------------------------
#    class XhClass
#------------------------------------------------------------------
#------------------------------------------------------------------

class XhClass(PyTango.DeviceClass):

    class_property_list = {}

    device_property_list = {
        'cam_ip_address':
        [PyTango.DevString,
         "Camera ip address",[]],
        'port':
        [PyTango.DevShort,
         "port number",[]],
        'config_name':
        [PyTango.DevString,
         "The default configuration loaded",[]],
        'XH_TIMING_SCRIPT':
        [PyTango.DevVarStringArray,
        "Timing scripts",
        ['config_timing_1turn',
        'config_timing_2turn',
        'config_timing_3turn',
        'config_timing_4turn',
        'config_timing_3turn_no_overlap',
        'config_timing_4turn_no_overlap',
        'config_timing_2turn_4bunch',
        'config_timing_2turn_16bunch',
        'config_timing_2turn_4bunch_no',
        'config_timing_2turn_16bunch_no',
        'config_timing_3turn_4bunch',
        'config_timing_3turn_16bunch',
        'config_timing_4turn_4bunch',
        'config_timing_4turn_16bunch',
        'config_timing_5turn_4bunch',
        'config_timing_5turn_16bunch',
        ]]
        }

    cmd_list = {
        'getAttrStringValueList':
        [[PyTango.DevString, "Attribute name"],
            [PyTango.DevVarStringArray, "Authorized String value list"]],
        'reset':
        [[PyTango.DevVoid, ""],
            [PyTango.DevVoid, ""]],
        'setHeadCaps':
        [[PyTango.DevVarULongArray, "Caps for AB, Caps for CD"],
            [PyTango.DevVoid, ""]],
        'sendCommand':
        [[PyTango.DevString, "da.server command"],
            [PyTango.DevVoid, ""]],
        'setHighVoltageOn':
        [[PyTango.DevVoid, ""],
            [PyTango.DevVoid, ""]],
        'setHighVoltageOff':
        [[PyTango.DevVoid, ""],
            [PyTango.DevVoid, ""]],
        'setHeadDac':
        [[PyTango.DevFloat, "value"], 
            [PyTango.DevVoid, ""]],
        'getAvailableCaps':
        [[PyTango.DevVoid, ""],
            [PyTango.DevVarShortArray, "available caps"]],
        'getAvailableTriggerModes':
        [[PyTango.DevVoid, ""],
            [PyTango.DevVarStringArray, "Trigger modes"]],
        'setXhTimingScript':
        [[PyTango.DevString, "Script name"],
            [PyTango.DevVoid, ""]],
        'getTemperature':
        [[PyTango.DevString, "Channel number"],
            [PyTango.DevFloat, ""]],
        'setTimingOrbit': [[
            PyTango.DevVarULongArray, "Array containing two elements, first corresponds to delay (int), second (bool, 0/1) tells if falling edge is used"
        ], [
            PyTango.DevVoid, ""
        ]]
    }
		
    attr_list = {
       	'clockmode':
	[[PyTango.DevString,
	PyTango.SCALAR,
	PyTango.WRITE]],
       	'nbscans':
	[[PyTango.DevLong,
	PyTango.SCALAR,
	PyTango.READ_WRITE]],
        'nb_groups':
	[[PyTango.DevLong,
	PyTango.SCALAR,
	PyTango.READ_WRITE]],
        'maxframes':
	[[PyTango.DevLong,
	PyTango.SCALAR,
	PyTango.READ]],
       'trig_mux':
	[[PyTango.DevLong,
	PyTango.SCALAR,
	PyTango.READ_WRITE]],
        'orbit_trigger':
    [[PyTango.DevLong,
    PyTango.SCALAR,
    PyTango.READ_WRITE]],
        'lemo_out':
    [[PyTango.DevLong,
    PyTango.SPECTRUM,
    PyTango.READ_WRITE, 1024]],
        'correct_rounding':
    [[PyTango.DevBoolean,
    PyTango.SCALAR,
    PyTango.READ_WRITE]],
        'group_delay':
    [[PyTango.DevLong,
    PyTango.SCALAR,
    PyTango.READ_WRITE]],
        'frame_delay':
    [[PyTango.DevLong,
    PyTango.SCALAR,
    PyTango.READ_WRITE]],
        'scan_period':
    [[PyTango.DevLong,
    PyTango.SCALAR,
    PyTango.READ_WRITE]],
        'aux_delay':
    [[PyTango.DevLong,
    PyTango.SCALAR,
    PyTango.READ_WRITE]],
        'aux_width':
    [[PyTango.DevLong,
    PyTango.SCALAR,
    PyTango.READ_WRITE]],
        'custom_trigger_mode':
    [[PyTango.DevString,
    PyTango.SCALAR,
    PyTango.WRITE]]
    }

    def __init__(self,name) :
        PyTango.DeviceClass.__init__(self,name)
        self.set_type(name)


#------------------------------------------------------------------
#------------------------------------------------------------------
#    Plugins
#------------------------------------------------------------------
#------------------------------------------------------------------

_XhCam = None
_XhInterface = None

def get_control(cam_ip_address = "0", port = 1972, config_name = 'config', XH_TIMING_SCRIPT = [], **keys) :
    global _XhCam
    global _XhInterface
    
    if _XhCam is None:
        print (cam_ip_address)
        print (port)
        print (config_name)
        #	Core.DebParams.setTypeFlags(Core.DebParams.AllFlags)
        # XH_TIMING_SCRIPT must be converted to list of strings. By default the type is StdStringArray which is not consumed in cpp
        # when argument type is std::vector<std::string>
        _XhCam = XhAcq.Camera(cam_ip_address, int(port), config_name, list(XH_TIMING_SCRIPT))
        _XhInterface = XhAcq.Interface(_XhCam)
    return Core.CtControl(_XhInterface)

def get_tango_specific_class_n_device():
    return XhClass,Xh
