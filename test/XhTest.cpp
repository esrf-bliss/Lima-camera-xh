//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2013
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################
#include "HwInterface.h"
#include "CtControl.h"
#include "CtAccumulation.h"
#include "CtAcquisition.h"
#include "CtSaving.h"
#include "CtShutter.h"
#include "Constants.h"

#include "XhCamera.h"
#include "XhInterface.h"
#include "Debug.h"
//#include "Exception.h"
#include <iostream>

using namespace std;
using namespace lima;
using namespace lima::Xh;

DEB_GLOBAL(DebModTest);

int main(int argc, char *argv[])
{
	DEB_GLOBAL_FUNCT();
//	DebParams::setModuleFlags(DebParams::AllFlags);
//	DebParams::setTypeFlags(DebParams::AllFlags);
//	DebParams::setFormatFlags(DebParams::AllFlags);

	Camera *m_camera;
	Interface *m_interface;
	CtControl* m_control;

	//xh configuration properties
	string hostname = "rnice31";
	string configName = "config";
	int port = 1972;

	try {

		m_camera = new Camera(hostname, port, configName);
		m_interface = new Interface(*m_camera);
		m_control = new CtControl(m_interface);
		m_camera->init();

		int nframes;
		Camera::TriggerControlType trigControl;
		trigControl = (Camera::TriggerControlType) (Camera::XhTrigIn_noTrigger);
		m_camera->setTimingGroup(0, 10, 5, 50000000, 0, trigControl);
		m_camera->setTimingGroup(1, 4, 5, 50000000, 1, trigControl);
		m_camera->setupClock(Camera::XhESRF5468MHz);
		m_camera->setExtTrigOutput(0, Camera::XhTrigOut_dc);

		CtSaving* saving = m_control->saving();
		saving->setDirectory("./data");
		saving->setFormat(CtSaving::EDF);
	 	saving->setPrefix("id24_");
		saving->setSuffix(".edf");
		saving->setSavingMode(CtSaving::AutoFrame);
		saving->setOverwritePolicy(CtSaving::Append);

		m_camera->getNbFrames(nframes);
		m_control->acquisition()->setAcqNbFrames(nframes);
		m_control->prepareAcq();
		m_control->startAcq();
		usleep(5000);
		while(1) {
			usleep(1000);
			Camera::XhStatus status;
			m_camera->getStatus(status);
			if (status.state == status.Idle)
				break;
		}
		uint32_t buff[1024];
		for (int i=0; i<nframes; i++) {
			m_camera->readFrame(buff, i);
			cout << buff[0] << " " << buff[1] << " " << buff[2] << " " << buff[1023] << endl;
		}
//		m_camera->uninterleave(true);
//		for (int i=0; i<nframes; i++) {
//			m_camera->readFrame(buff, i);
//			cout << buff[0] << " " << buff[1] << " " << buff[2] << " " << buff[1023] << endl;
//		}

	} catch (Exception e) {
		DEB_ERROR() << "LIMA Exception: " << e;
	} catch (...) {
		DEB_ERROR() << "Unkown exception!";
	}
	return 0;
}
