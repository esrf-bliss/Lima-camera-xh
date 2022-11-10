//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2022
// European Synchrotron Radiation Facility
// CS40220 38043 Grenoble Cedex 9
// FRANCE
//
// Contact: lima@esrf.fr
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
#include "lima/HwInterface.h"
#include "lima/CtControl.h"
#include "lima/CtAccumulation.h"
#include "lima/CtAcquisition.h"
#include "lima/CtSaving.h"
#include "lima/CtShutter.h"
#include "lima/Constants.h"

#include "XhCamera.h"
#include "XhInterface.h"
#include "lima/Debug.h"
#include <iostream>
#include <unistd.h>

using namespace std;
using namespace lima;
using namespace lima::Xh;

DEB_GLOBAL(DebModTest);

int main(int argc, char *argv[])
{
	DEB_GLOBAL_FUNCT();
	DebParams::setModuleFlags(DebParams::AllFlags);
	DebParams::setTypeFlags(DebParams::AllFlags);
	DebParams::setFormatFlags(DebParams::AllFlags);

	Camera *m_camera;
	Interface *m_interface;
	CtControl* m_control;

	//xh configuration properties
	string hostname = "gmvig1"; //"rnice31";
	string configName = "config";
	int port = 1972;

	try {

		m_camera = new Camera(hostname, port, configName);
		m_interface = new Interface(*m_camera);
		m_control = new CtControl(m_interface);

		// Setup user timing controls
		int nframes;
		Camera::XhTimingParameters timingParams;
		m_camera->setDefaultTimingParameters(timingParams);
		timingParams.trigControl = (Camera::TriggerControlType) (Camera::XhTrigIn_noTrigger);
		m_camera->setTimingGroup(0, 10, 5, 50000000, 0, timingParams);
		m_camera->setTimingGroup(1, 4, 5, 50000000, 1, timingParams);
		m_camera->setupClock(Camera::XhESRF5468MHz);
		m_camera->setExtTrigOutput(0, Camera::XhTrigOut_dc);

		// setup fileformat and data saving info
		CtSaving* saving = m_control->saving();
		saving->setDirectory("./data");
		saving->setFormat(CtSaving::EDF);
	 	saving->setPrefix("id24_");
		saving->setSuffix(".edf");
		saving->setSavingMode(CtSaving::AutoFrame);
		saving->setOverwritePolicy(CtSaving::Append);

		// do acquisition
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
			m_camera->readFrame(buff, i, 1);
			cout << buff[0] << " " << buff[1] << " " << buff[2] << " " << buff[1023] << endl;
		}
//		m_camera->uninterleave(true);
//		for (int i=0; i<nframes; i++) {
//			m_camera->readFrame(buff, i);
//			cout << buff[0] << " " << buff[1] << " " << buff[2] << " " << buff[1023] << endl;
//		}

	} catch (Exception& ex) {
		DEB_ERROR() << "LIMA Exception: " << ex;
	} catch (...) {
		DEB_ERROR() << "Unkown exception!";
	}
	return 0;
}
