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

- **getAttrStringValueList**: Returns a list of authorized values, if any.
- **reset**
- **setHeadCaps** <- DevVarULongArray: Common interface for setting the feedback capacitors. Expects three parameters:
    - Capacitance for common or AB caps.
    - Capacitance for CD caps.
    - Specify single head (0 or 1); default is both (-1).
- **sendCommand** <- DevString: Sends a string command to the device without expecting output.
- **sendCommandNumber** <- DevString: Sends a string command to the device and expects numeric output.
- **sendCommandString** <- DevString: Sends a string command to the device and expects string output.
- **setHighVoltageOn**: Sets high voltage; no parameters required. It sends two commands:
    ```
    "xstrip hv enable " + sysName + " auto"
    "xstrip hv set-dac " + sysName + " -90"
    ```
- **setHighVoltageOff**: Disables high voltage; no parameters required. Sends:
    ```
    "xstrip hv enable " + sysName + " off"
    ```
- **setHeadDac** <- DevFloat: Sets DAC to control HV power supply voltage. Expect the following parameters:
    - Value: The voltage (Volts).
    - VoltageType: Specifies which ADC.
    - Head: Specify single head (0 or 1); default is both (-1).
    - Direct: Cast value to int; default converts voltage to DAC code (false).
- **getAvailableCaps**: Lists available capacitors.
- **getAvailableTriggerModes**: Lists available trigger modes.
- **setXhTimingScript** <- DevString: Sets the timing script, expecting the following parameter:
    - Script: Script file name.
- **getTemperature**: Gets temperature and returns an array of 4 channels' temperatures.
- **powerDown**: Sends cooldown_xh script.
- **coolDown**: Sends head_powerdown script.
- **configXh** <- DevString: Sets configuration file.
- **setTrigGroupMode**: Sets group trigger. Expects one integer parameter:
    - 0: no_trigger.
    - 1: group trigger.
    - 2: group orbit.
- **setTrigScanMode**: Sets scan trigger. Expects one integer parameter:
    - 0: no_trigger.
    - 1: scan trigger.
    - 2: scan orbit.
- **setTrigFrameMode**: Sets frame trigger. Expects one integer parameter:
    - 0: no_trigger.
    - 1: frame trigger.
    - 2: frame orbit.
- **shutDown**: Shuts down the detector in a controlled manner. Expects one parameter:
    - Script: The name of the shutdown command file.
- **setVoltage/getVoltage**: Sets or gets voltage.
- **setCapa/getCapa**

### Attributes

- **clockmode**: Sets up clock mode to one of: 'XhInternalClock': 0, 'XhESRF5468Mhz': 1, 'XhESRF1136Mhz': 2.
- **nbscans**: Defines the number of scans. One number can be set and applied to every setup group.
- **nbgroups**: Defines the number of groups. One number can be set and applied to every setup group.
- **maxframes**: Gets the maximum number of frames.
- **trig_mux**
- **orbit_trigger**
- **orbit_delay**
- **orbit_delay_falling**
- **orbit_delay_rising**
- **lemo_out**
- **correct_rounding**
- **group_delay**
- **frame_delay**
- **scan_period**
- **aux_delay**
- **aux_width**
- **custom_trigger_mode**: Sets custom trigger mode. Available values are "no_trigger", "group_trigger", "frame_trigger", "scan_trigger", "group_orbit", "frame_orbit", "scan_orbit", "falling_trigger".
- **trig_group_mode**: Sets triggers based on the number provided as attribute:
    - 0: "no_trigger".
    - 1: "group_trigger".
    - 2: "group_orbit" and sets orbit trig to 3.
- **trig_frame_mode**: Sets triggers based on the number provided as attribute:
    - 0: "no_trigger".
    - 1: "frame_trigger".
    - 2: "frame_orbit" and sets orbit trig to 3.
- **trig_scan_mode**: Sets triggers based on the number provided as attribute:
    - 0: "no_trigger".
    - 1: "scan_trigger".
    - 2: "scan_orbit" and sets orbit trig to 3.
- **timing_script**: Sets the timing script.
- **bias**: Gets bias.
- **capa**: Sets or reads capacitor settings.
- **timemode**: Sets or disables time mode. If time mode is set, exposure time is multiplied by the clock factor: exp_time * clock_factor.

### Software Trigger Generation

For InternalTriggerMulti, the software trigger is set by the code. It sets `m_software_trigger` to true and acquires as many frames as set. Once the last one is grabbed, it disables the software trigger by setting `m_software_trigger` to false.

### Frame Acquisition

Setting the timing group is executed in `prepareAcq()`. We need to set up the timing group before acquisition. If we plan more than one group, the last of them needs the "last" keyword at the end.

> It could also be set in `startAcq()` for every trigger, but this would make sense only in the case of having one group. In that case, we can have just one group with id = 0 and "last" at the end, and set it for the first trigger only.

Lima must have a frame number set before acquisition. So, if we need 25 frames, we ask like "loopscan(25, exposure_time, xh_lima)".

If we have more than one group, the number of frames multiplies. Every group will have the number of frames set in the setup timing group. So, having 5 groups with 5 frames each results in 25 frames in total.

Once Lima knows the number of frames collected, if you would like to have more than one group, then the number of frames set will be divided by the number of groups.

> So, if you have 5 groups and set "loopscan(25, exposure, xh_lima)", it will result in having 5 frames acquired in every group, but still 25 in total.


