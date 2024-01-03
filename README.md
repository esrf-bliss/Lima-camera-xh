[![License](https://img.shields.io/github/license/esrf-bliss/lima.svg?style=flat)](https://opensource.org/licenses/GPL-3.0)
[![Gitter](https://img.shields.io/gitter/room/esrf-bliss/lima.svg?style=flat)](https://gitter.im/esrf-bliss/LImA)
[![Conda](https://img.shields.io/conda/dn/esrf-bcu/lima-camera-xh.svg?style=flat)](https://anaconda.org/esrf-bcu)
[![Version](https://img.shields.io/conda/vn/esrf-bcu/lima-camera-xh.svg?style=flat)](https://anaconda.org/esrf-bcu)
[![Platform](https://img.shields.io/conda/pn/esrf-bcu/lima-camera-xh.svg?style=flat)](https://anaconda.org/esrf-bcu)

# LImA Quantum XH Camera Plugin

This is the LImA plugin for Quantum XH detector.

## Install

### Camera Python

`conda install -c esrf-bcu lima-camera-xh`

### Camera Tango Device Server

`conda install -c tango-controls -c esrf-bcu lima-camera-xh-tango`

## Manual Compilation

Build:

`cmake --build . --target install --config Release`

Install:

`cmake .. -DLIMA_ENABLE_PYTHON=1 -DCMAKE_INSTALL_PREFIX=$CONDA_PREFIX -DLIMA_ENABLE_PYTANGO_SERVER=1`

# LImA

Lima ( **L** ibrary for **Im** age **A** cquisition) is a project aimed at the unified control of 2D detectors. The aim is to clearly separate hardware-specific code from common software configuration and features, such as setting standard acquisition parameters (exposure time, external trigger), file saving and image processing.

Lima is a C++ library which can be used with many different cameras. The library also includes a [Python](http://python.org) binding and provides a [PyTango](http://pytango.readthedocs.io/en/stable/) device server for remote control.

## Documentation

### Custom Commands

Commands are sorted alphabetically for ease of reference.

#### Command Sending:

- **sendCommand** <- DevString: Sends a string command to the device without expecting output.
- **sendCommandNumber** <- DevString: Sends a string command to the device and expects a numeric output.
- **sendCommandString** <- DevString: Sends a string command to the device and expects a string output.

#### Other Commands:

- **configXh** <- DevString: Sets configuration file.
- **coolDown** <- Sends head_powerdown script.
- **powerDown** <- Sends cooldown_xh script.
- **shutDown** <- Shuts down the detector in a controlled manner (only for internal camera usage). Parameter:
    - Script: The name of the shutdown command file.

- **getAttrStringValueList** <- Returns a list of authorized values, if any.
- **getAvailableCaps** <- Lists available capacitors.
- **getAvailableTriggerModes** <- Lists available trigger modes.
- **setHeadCaps** <- DevVarULongArray: Common interface for setting the feedback capacitors. Parameters:
    - Capacitance for common or AB caps.
    - Capacitance for CD caps.
    - Specify single head (0 or 1); default is both (-1).
- **setHeadDac** <- DevFloat: Sets DAC to control HV power supply voltage. Parameters:
    - Value: The voltage (Volts).
    - VoltageType: Specifies which ADC.
    - Head: Specify single head (0 or 1); default is both (-1).
    - Direct: Cast value to int; default converts voltage to DAC code (false).
- **setHighVoltageOff** <- Disables high voltage; no parameters required.Sends:
    ```
    xstrip hv enable <<sysName>> off
    ```
- **setHighVoltageOn** <- Sets high voltage; no parameters required. It sends two commands:
    ```
    xstrip hv enable <<sysName>> auto
    xstrip hv set-dac <<sysName>> -90
    ```


### Attributes

- **allow_excess** <- By default set to Fasle and is not used,
- **aux_delay** <- By default set to 0 and is not used,
- **aux_width** <- By default set to 1 and is not used,
- **bias** <- Gets bias.
- **capa** <- Sets or reads capacitor settings.
- **clockmode** <- Sets up clock mode to one of: 'XhInternalClock': 0, 'XhESRF5468Mhz': 1, 'XhESRF1136Mhz': 2.
- **custom_trigger_mode** <- Sets custom trigger mode. Available values are "no_trigger", "group_trigger", "frame_trigger", "scan_trigger", "group_orbit", "frame_orbit", "scan_orbit", "falling_trigger".
- **correct_rounding** <- By default set to False and is not used,
- **cycles_end** <- By default set to False and is not used,
- **cycles_start** <- By default set to 1,
- **frame_delay** <- By default set to 1 and is not used,
- **frame_time** <- By default set to 0 and is not used,
- **group_delay** <- By default set to 0 and is not used,
- **lemo_out** <- Lemo can be set as single integer value, or multiple values, value is mapped to the lemo output. Mapping is done using binary coding, where 2 bits are used for single output. When multiple values are provided binary OR is calculated and corresponding outputs are then activated. LEMO7 is reserved for the orbit trigger.

    Per group enables for the 8 Lemo outputs. These are binary coded 0..65535, using 2 bits per LEMO output. Bits 1..0 for LEMO0, bits 3..2 for LEMO1 … bits 15..13 for LEMO 7. For each output, currently 0=> do not pulse output in this frame, 1 => do pulse output in this frame, 2..3 => reserved for future expansion.

    By default set to `[65535]`.

    > Example:

        xstrip timing setup-group "xh0" 0 200000 3 500 frame-delay 300 lemo-out 65535 aux-delay 250 aux-width 50

        xstrip timing setup-group "xh0" 1 100000 3 500 frame-delay 300 lemo-out 1|4096|16384

    The first group sets lemo-out 65535 which enables currently all 8 outputs during this group. Group 1, lemo-out 1|4096|16384 enable outputs 0, 6 and 7. Group 2 enables outputs 0, 2, 6 and 7 etc.
- **maxframes** <- Gets the maximum number of frames.
- **nbscans** <- Defines the number of scans. One number can be set and applied to every setup group.
- **nbgroups** <- Defines the number of groups. One number can be set and applied to every setup group.
- **orbit_trigger** <- Sets or gets trigger orbit. By default equals to -1 and is ommited.
    - 0: direct
    - 1: delayed
    - 2: LVDS direct
    - 3: LVDS delayed
- **orbit_delay** <- Sets or gets orbit delay. Expects double and executes `xstrip timing setup-orbit <<sysName>> orbit_delay`
- **orbit_delay_falling** <- Sets or gets delay falling. If True, setup-orbit is command is appended with `falling`
- **orbit_delay_rising** <- Sets or gets delay rising. By default is True.
- **RstF_delay** <- By default set to 0 and is not used,
- **RstR_delay** <- By default set to 0 and is not used,
- **scan_period** <- By default set to 0 and is not used,
- **s1_delay** <- By default set to 0 and is not used,
- **s2_delay** <- By default set to 0 and is not used,
- **shift_down** <- By default set to 0 and is not used,
- **temperature** <- Gets temperature and returns an array of 4 channels' temperatures.
- **timemode** <- Sets or disables time mode. If time mode is set, exposure time is multiplied by the clock factor: exp_time * clock_factor.
- **timing_script** <- DevString: Sets the timing script, expecting the following parameter:
    - Script: Script file name.
- **trig_frame_mode** <- Sets triggers based on the number provided as attribute:
    - 0: "no_trigger".
    - 1: "frame_trigger".
    - 2: "frame_orbit" and sets orbit trig to 3.
- **trig_group_mode** <- Sets triggers based on the number provided as attribute:
    - 0: "no_trigger".
    - 1: "group_trigger".
    - 2: "group_orbit" and sets orbit trig to 3.
- **trig_mux** <- Sets or gets the input to wait for.
    - 9 - Software trigger (default)
    - 8 - main trigger, delayed orbit
    - 0...7 - Lemo
- **trig_scan_mode** <- Sets triggers based on the number provided as attribute:
    - 0: "no_trigger".
    - 1: "scan_trigger".
    - 2: "scan_orbit" and sets orbit trig to 3.
- **voltage** <- Sets or gets voltage. Expects double value
- **Xclk_delay** <- By default set to 0 and is not used,

### Software Trigger Generation

For InternalTriggerMulti, the software trigger is set by the software. It sets `m_software_trigger` to true and acquires as many frames as set. Once the last one is grabbed, it disables the software trigger by setting `m_software_trigger` to false.

### Frame Acquisition

Setting the timing group is executed in `prepareAcq()`. We need to set up the timing group before acquisition. If we plan more than one group, the last of them needs the "last" keyword at the end, which is implemented inside `prepareAcq()`.

> It could also be set in `startAcq()` for every trigger, but this would make sense only in the case of having one group. In that case, we can have just one group with id = 0 and "last" at the end, and set it for the first trigger only.

Lima must have a frame number set before acquisition. So, if we need 25 frames, we ask like "loopscan(25, exposure_time, xh_lima)".

If we have more than one group, the number of frames is multiplied. Every group will have the number of frames set in the setup timing group. So, having 5 groups with 5 frames each results in 25 frames in total.

Once Lima knows the number of frames collected, if you would like to have more than one group, then the number of frames set will be divided by the total number of groups.

> So, if you have 5 groups and set "loopscan(25, exposure, xh_lima)", it will result in having 5 frames acquired in every group, but still 25 in total.


