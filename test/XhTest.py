############################################################################
# This file is part of LImA, a Library for Image Acquisition
#
# Copyright (C) : 2009-2022
# European Synchrotron Radiation Facility
# CS40220 38043 Grenoble Cedex 9
# FRANCE
#
# Contact: lima@esrf.fr
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

from Lima import Xh
from Lima import Core

cam=Xh.Camera('youpi', 1972)
cam=Xh.Camera(1972,'youpi')
cam=Xh.Camera('youpi', 1972, 'config')

inter = Xh.Interface(cam)
cont=Core.CtControl(inter)

timeparms = Xh.Camera.XhTimingParameters()
cam.setDefaultTimingParameters(timeparms)
timeparms.trigControl=Xh.Camera.XhTrigIn_noTrigger
cam.setTimingGroup(0,10,5,50000000,0,timeparms)
cam.setTimingGroup(1,4,5,50000000,1,timeparms)
cam.setupClock(Xh.Camera.XhESRF5468MHz)
cam.setExtTrigOutput(0,Xh.Camera.XhTrigOut_dc)

saving=cont.saving()
saving.setDirectory('/home/claustre/XH-SIMULATOR/data')
saving.setFormat(Core.CtSaving.EDF)
saving.setPrefix('test_with_youpi_')
saving.setSuffix('.edf')
saving.setSavingMode(Core.CtSaving.AutoFrame)
saving.setOverwritePolicy(Core.CtSaving.Append)

cam.getNbFrames()
acq=cont.acquisition()
acq.setAcqNbFrames(cam.getNbFrames())
cont.prepareAcq()
cont.startAcq()
image=cont.image()
cont.ReadImage(-1)
imlast=cont.ReadImage(-1)
print imlast
imfirst=cont.ReadImage(0)
print imfirst

