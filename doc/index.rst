.. _camera-xh:

XH camera
---------


Introduction
````````````

  "XH is the worlds first 50μm pitch Ge Strip detector which has been designed specifically for Energy Dispersive EXAFS (EDE). Carrying on from the CLRC development of XSTRIP1, a Si based detector system, XH makes use of amorphous germanium (a-Ge) contact technology produced by LBNL2 and readout ASICs developed by CLRC. XH is designed to address the issues of detection efficiency and radiation damage that limit the effectiveness of the original XSTRIP system."

The system is controlled from its own PC or via a TCP/IP connection from a beamline computer system.

The Lima plugin has been tested only at ESRF for a unique XH detector on BM23 and ID24 beamlines.

Prerequisite Linux OS
`````````````````````

The plugin is only working for Linux distribution and been tested on Redhat E4 i386 and debian 6 x86_64.

Installation & Module configuration
```````````````````````````````````

Follow the generic instructions in :ref:`build_installation`. If using CMake directly, add the following flag:

.. code-block:: sh

 -DLIMACAMERA_XH=true

For the Tango server installation, refers to :ref:`tango_installation`.

Initialisation and Capabilities
```````````````````````````````

Implementing a new plugin for new detector is driven by the LIMA framework but the developer has some freedoms to choose which standard and specific features will be made available. This section is supposed to give you the correct information regarding how the camera is exported within the LIMA framework.

Camera initialisation
.....................

TODO

Std capabilities
................

This plugin has been implemented in respect of the mandatory capabilites but with some limitations which
are due to the camera and SDK features.  We only provide here extra information for a better understanding
of the capabilities for Andor cameras.

* HwDetInfo

 TODO

* HwSync

 TODO

Optional capabilities
.....................

In addition to the standard capabilities, we make the choice to implement some optional capabilities which
are supported by the SDK and the I-Kon cameras. A Shutter control, a hardware ROI and a hardware Binning are available.

* HwShutter

 TODO

* HwRoi

 TODO

* HwBin

 TODO

Configuration
`````````````

 TODO

How to use
````````````

This is a python code example for a simple test:

.. code-block:: python

  from Lima import Xh
  from lima import Core

  #                 hostname     port  config name
  cam = Xh.Camera('xh-detector', 1972, 'config_xhx3')
  hwint = Xh.Interface(cam)
  ct = Core.CtControl(hwint)

  acq = ct.acquisition()

  # configure some hw parameters

  # set some low level configuration

  # setting new file parameters and autosaving mode
  saving=ct.saving()

  pars=saving.getParameters()
  pars.directory='/buffer/lcb18012/opisg/test_lima'
  pars.prefix='test1_'
  pars.suffix='.edf'
  pars.fileFormat=Core.CtSaving.EDF
  pars.savingMode=Core.CtSaving.AutoFrame
  saving.setParameters(pars)

  # now ask for 2 sec. exposure and 10 frames
  acq.setAcqExpoTime(2)
  acq.setNbImages(10)

  ct.prepareAcq()
  ct.startAcq()

  # wait for last image (#9) ready
  lastimg = ct.getStatus().ImageCounters.LastImageReady
  while lastimg !=9:
    time.sleep(0.1)
    lastimg = ct.getStatus().ImageCounters.LastImageReady

  # read the first image
  im0 = ct.ReadImage(0)
