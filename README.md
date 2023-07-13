[![License](https://img.shields.io/github/license/esrf-bliss/lima.svg?style=flat)](https://opensource.org/licenses/GPL-3.0)
[![Gitter](https://img.shields.io/gitter/room/esrf-bliss/lima.svg?style=flat)](https://gitter.im/esrf-bliss/LImA)
[![Conda](https://img.shields.io/conda/dn/esrf-bcu/lima-camera-xh.svg?style=flat)](https://anaconda.org/esrf-bcu)
[![Version](https://img.shields.io/conda/vn/esrf-bcu/lima-camera-xh.svg?style=flat)](https://anaconda.org/esrf-bcu)
[![Platform](https://img.shields.io/conda/pn/esrf-bcu/lima-camera-xh.svg?style=flat)](https://anaconda.org/esrf-bcu)

# LImA Quantum XH Camera Plugin

This is the LImA plugin for Quantum XH detector.

## Install

### Camera python

conda install -c esrf-bcu lima-camera-xh

### Camera tango device server

conda install -c tango-controls -c esrf-bcu lima-camera-xh-tango

# LImA

Lima ( **L** ibrary for **Im** age **A** cquisition) is a project for the unified control of 2D detectors. The aim is to clearly separate hardware specific code from common software configuration and features, like setting standard acquisition parameters (exposure time, external trigger), file saving and image processing.

Lima is a C++ library which can be used with many different cameras. The library also comes with a [Python](http://python.org) binding and provides a [PyTango](http://pytango.readthedocs.io/en/stable/) device server for remote control.

## Documentation

### Commands

- *getAttrStringValueList*
- *reset*
- *setHeadCaps* <- DevVarULongArray - common interface for setting the feedback capacitators
- *sendCommand* <- DevString - sends a string command to the device and do not expect output
- *sendCommandNumber* <- DevString - sends a string command to the device and expect numeric output
- *sendCommandString* <- DevString - sends a string command to the device and  expect string output
- *setHighVoltageOn* - sets high voltage
- *setHighVoltageOff* - disables high voltage
- *setHeadDac* <- DevFloat - sets DAC to control HV power supply voltage
- *getAvailableCaps* - lists available Caps
- *getAvailableTriggerModes* - lists available trigger modes
- *setXhTimingScript* <- DevString - sets timing script
- *getTemperature* - gets temperature and returns an array of 4 channels temp
- *powerDown* - sends cooldown_xh scipr
- *coolDown* - sends head_powerdown script
- *configXh* <- DevString - sets config file

### Attributes
- *clockmode* - setups clock mode to one of: 'XhInternalClock': 0, 'XhESRF5468Mhz': 1, 'XhESRF1136Mhz': 2
- *nbscans* - defines number of scans. One number can be set and applied to every setup group
- *nbgroups* - defines number of groups. One number can be set and applied to every setup group
- *maxframes* - get maximum number of frames
- *trig_mux* - 
- *orbit_trigger*
- *orbit_delay*
- *orbit_delay_falling*
- *orbit_delay_rising*
- *lemo_out*
- *correct_rounding*
- *group_delay*
- *frame_delay*
- *scan_period*
- *aux_delay*
- *aux_width*
- *custom_trigger_mode* - sets custom trigger mode. Available values are "no_trigger", "group_trigger", "frame_trigger", "scan_trigger", "group_orbit", "frame_orbit", "scan_orbit", "falling_trigger", 
- *trig_group_mode* - sets triggers based on number provided as attribute:
        - 0: "no_trigger";
	    - 1: "group_trigger"
        - 2: "group_orbit" and sets orbit trig to 3;
- *trig_frame_mode* - sets triggers based on number provided as attribute:
    - 0: "no_trigger"
    - 1: "frame_trigger
    - 2: "frame_orbit" and orbit trig to 3
- *trig_scan_mode* - sets triggers based on number provided as attribute:
    - 0: "no_trigger"
	- 1: "scan_trigger"
    - 2: "scan_orbit" and orbit trig to 3;
- *timing_script* - set timing script
- *bias* - gets bias
- *capa* - sets or writes capa
- *timemode* - sets or disables timemode. If time mode set exposure time is multplied by clock factor: exp_time * clock_factor
