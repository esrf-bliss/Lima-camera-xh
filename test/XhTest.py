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

