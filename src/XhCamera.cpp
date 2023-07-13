//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2011
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
//
// XhCamera.cpp
// Created on: Feb 04, 2013
// Author: g.r.mant

#include <sstream>
#include <iostream>
#include <iterator>
#include <string>
#include <math.h>
#include <unistd.h>
#include <climits>
#include <iomanip>
#include <bit>
#include <cstdint>
//#include <concepts>

#include "XhCamera.h"
#include "lima/Exceptions.h"
#include "lima/Debug.h"

using namespace lima;
using namespace lima::Xh;
using namespace std;


//---------------------------
//- utility thread
//---------------------------
class Camera::AcqThread: public Thread {
DEB_CLASS_NAMESPC(DebModCamera, "Camera", "AcqThread");
public:
	AcqThread(Camera &aCam);
	virtual ~AcqThread();
	bool m_thread_started;

protected:
	virtual void threadFunction();

private:
	Camera& m_cam;
};

//---------------------------
// @brief  Ctor
//---------------------------

Camera::Camera(string hostname, int port, string configName, std::vector<std::string> XH_TIMING_SCRIPT, float clock_factor) :
	m_hostname(hostname),
	m_port(port),
	m_configName(configName),
	m_sysName("'xh0'"),
	m_clock_factor(clock_factor),
	m_orbit_delay_rising(true),
	m_orbit_delay_falling(false),
	m_uninterleave(false),
	m_nb_groups(1),
	m_npixels(1024),
	m_image_type(Bpp32),
	m_nb_frames_to_collect(1),
	m_acq_frame_nb(0),
	m_exp_time(1.0),
	m_bufferCtrlObj(),
	m_wait_flag(true),
	m_quit(false)
{
	DEB_CONSTRUCTOR();

//	DebParams::setModuleFlags(DebParams::AllFlags);
//	DebParams::setTypeFlags(DebParams::AllFlags);
//	DebParams::setFormatFlags(DebParams::AllFlags);
	m_xh_timing_scripts = XH_TIMING_SCRIPT;
	m_acq_thread = new AcqThread(*this);
	m_acq_thread->start();

	m_xh = new XhClient();
	setRoi(Roi(0,0,0,0));
	init();
}

Camera::~Camera() {
	DEB_DESTRUCTOR();
	m_xh->disconnectFromServer();
	delete m_xh;
	stopAcqThread();
}

void Camera::init() {
	DEB_MEMBER_FUNCT();
	stringstream cmd1, cmd2, cmd3;
	int dataPort;

	if (m_xh->connectToServer(m_hostname, m_port) < 0) {
		THROW_HW_ERROR(Error) << "[ " << m_xh->getErrorMessage() << " ]";
	}
	if ((dataPort = m_xh->initServerDataPort()) < 0) {
		THROW_HW_ERROR(Error) << "[ " << m_xh->getErrorMessage() << " ]";
	}
	DEB_TRACE() << "da.server assigned dataport " << dataPort;
	if (m_configName.length() != 0) {
		// TODO: uncomment
		// cmd1 << "~" << m_configName;
		// m_xh->sendWait(cmd1.str());
	}
	if (m_uninterleave) {
		cmd2 << "xstrip open " << m_sysName << " un-interleave";
		m_xh->sendWait(cmd2.str(), m_openHandle);
	} else {
		cmd2 << "xstrip open " << m_sysName;
		m_xh->sendWait(cmd2.str(), m_openHandle);
	}
	if (m_openHandle < 0) {
		THROW_HW_ERROR(Error) << "[ " << m_xh->getErrorMessage() << " ]";
	} else {
		DEB_TRACE() << "configured open path as " << m_openHandle;
		cmd3 << "unif-get-nx " << m_openHandle;
		m_xh->sendWait(cmd3.str(), m_npixels);
		DEB_TRACE() << "configured pixels as " << m_npixels;
	}
	
	//call setDefaultTimingParameters to initialize
	setDefaultTimingParameters(m_timingParams);
	//by default, 1 scan
	m_nb_scans = 1;
	// m_clock_mode = 0;
	//timearray[0] = 20*1e-9;
	//timearray[1] = 22*1e-9;
	//timearray[2] = 22*1e-9;
	setTrigMux(9);
	DEB_TRACE() << " m_timingParams.trigMux : " << m_timingParams.trigMux;
	
}

void Camera::reset() {
	DEB_MEMBER_FUNCT();
	m_xh->disconnectFromServer();
	init();
}

void Camera::_prepareAcq() {
	sendCommand("xstrip timing ext-output " + m_sysName + " -1 integration");
	m_acq_thread = new AcqThread(*this);
	int bunch = (int) m_exp_time;
	int cycles_time = bunch;
	int quarter = 0;
	int inttime = m_exp_time;
	if (bunch != m_exp_time) {
		cycles_time = bunch;
		quarter = (int) 10 * (inttime - bunch);
		if (quarter > 3)
			quarter = 3;
	}
	
	setS2Delay(quarter);
	setExpTime(cycles_time);

	if (m_trig_group_mode)
		setTrigGroupMode(m_trig_group_mode);

	if (m_trig_frame_mode)
		setTrigFrameMode(m_trig_frame_mode);

	if (m_trig_scan_mode)
		setTrigScanMode(m_trig_scan_mode);
	
	std::vector<int> lemos;
	lemos.push_back(65535);
	setLemoOut(lemos);
}

void Camera::prepareAcq() {
	DEB_MEMBER_FUNCT();
	double exposure_time;
	getExpTime(exposure_time);
	int mexptime = (int) round(m_exp_time); // in cycles
	_prepareAcq();
	DEB_TRACE() << " nb frames : " << m_nb_frames_to_collect;
	DEB_TRACE() << " nb scans  : " << m_nb_scans;
	DEB_TRACE() << " exp time  : " << m_exp_time;
	m_acq_frame_nb = 0;
	// if (m_trigger_mode != IntTrigMult) {
		if (exposure_time != 0) {
			if (m_nb_groups == 0) {
				THROW_HW_ERROR(Error) << "Number of groups must be bigger than 0";
			}
			float nb_frames_for_group = float(m_nb_frames_to_collect) / float(m_nb_groups);
			if (std::floor(nb_frames_for_group) != nb_frames_for_group) {
				THROW_HW_ERROR(Error) << "Number of frames must be a multiple of number of groups";
			}
			for(int i = 0; i < m_nb_groups; i++) {
				bool last = false;
				if (i == m_nb_groups - 1) // mark last group with 'last' keyword
					last = true;
				
				setTimingGroup(i, nb_frames_for_group, m_nb_scans, mexptime, last, m_timingParams);
			}
		} else {
			int total_frames;
			getTotalFrames(total_frames);
			if (m_nb_frames_to_collect != total_frames)
				THROW_HW_ERROR(Error) << " Trying to collect a different number of frames than is currently configured ";		
		}
	// }
}

void Camera::startAcq() {
	DEB_MEMBER_FUNCT();
	double exposure_time;
	getExpTime(exposure_time);
	int mexptime = (int) round(exposure_time); // in cycles
	if (m_trigger_mode == IntTrigMult) {
		generateSoftwareTrigger();
		// if (m_nb_groups == 0) {
		// 	THROW_HW_ERROR(Error) << "Number of groups must be bigger than 0";
		// }
	
		// float nb_frames_for_group = float(m_nb_frames_to_collect) / float(m_nb_groups);
		// if (std::floor(nb_frames_for_group) != nb_frames_for_group) {
		// 	THROW_HW_ERROR(Error) << "Number of frames must be a multiple of number of groups";
		// }

		// std::cout << "NF FRAMES TO COLLECT: " << nb_frames_for_group <<std::endl;
		// std::cout << "Already acquired: " << m_acq_frame_nb <<std::endl;
		// std::cout << "total nb frames to collect: " << m_nb_frames_to_collect << std::endl;
		// if (m_nb_frames_to_collect % (m_acq_frame_nb + 1) == 0) {
		// 	int current_group_nb = m_acq_frame_nb % m_nb_groups;
		// 	std::cout << "Current group: " << current_group_nb <<std::endl;
		// 	bool last = false;
		// 	// if (((current_group_nb + 1) * nb_frames_for_group) == m_nb_frames_to_collect)
		// 			last = true;
		// 	setTimingGroup(current_group_nb, nb_frames_for_group, m_nb_scans, mexptime, last, m_timingParams);
		// }
	}
		// TODO: make sure its an integer
		// if (m_nb_frames_to_collect % m_acq_frame_nb == 0) {
		// for(int i = 0; i < m_nb_groups; i++) {
		// 	bool last = false;
		// 	if (i == m_nb_groups - 1) // mark last group with 'last' keyword
		// 		last = true;

		// 	// In IntTrigMult for each start acq only one frame is acquired
		// 	setTimingGroup(i, 1, m_nb_scans, mexptime, last, m_timingParams);
		
	stringstream cmd;
	StdBufferCbMgr& buffer_mgr = m_bufferCtrlObj.getBuffer();
	buffer_mgr.setStartTimestamp(Timestamp::now());
	cmd << "xstrip timing start " << m_sysName;
	m_xh->sendWait(cmd.str());
	AutoMutex aLock(m_cond.mutex());
	m_wait_flag = false;
	m_quit = false;
	m_cond.broadcast();
	// Wait that Acq thread start if it's an external trigger
	while (m_trigger_mode == ExtTrigMult && !m_thread_running)
		m_cond.wait();
}

void Camera::_stopAcq() {
	DEB_MEMBER_FUNCT();
	AutoMutex aLock(m_cond.mutex());
	if (!m_wait_flag) {
		while (m_thread_running) { // still acquiring
			m_wait_flag = true;
			m_cond.broadcast();
			m_cond.wait();
		}

		aLock.unlock();
	} else {
		DEB_TRACE() << "Called one more time, already stopped or about to be stopped";
	}
}

void Camera::stopAcq() {
	DEB_MEMBER_FUNCT();
	_stopAcq();
}

void Camera::readFrame(void *bptr, int frame_nb, int nframes) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	int num, retval;
	DEB_TRACE() << "reading frame " << frame_nb;
	if (m_uninterleave) {
		cmd << "read 0 0 " << frame_nb << " " << m_npixels/2 << " 2 " << nframes << " from " << m_openHandle;
	} else {
		cmd << "read 0 0 " << frame_nb << " " << m_npixels << " 1 " << nframes <<" from " << m_openHandle;
	}
	if (m_image_type == Bpp16) {
		num = nframes * m_npixels * sizeof(short) ;
		cmd <<  " raw";
	} else {
		num = nframes * m_npixels * sizeof(int32_t);
		cmd << " long";
	}
	AutoMutex aLock(m_cond.mutex());
	m_xh->sendNowait(cmd.str());
	m_xh->getData(bptr, num);
	if (m_xh->waitForResponse(retval) < 0) {
		THROW_HW_ERROR(Error) << "Waiting for response in readFrame";
	}
}

void Camera::setStatus(XhStatus::XhState status) {
	m_status = status;
}

void Camera::getStatus(XhStatus& status) {
	DEB_MEMBER_FUNCT();
	// status.state = m_status;
	stringstream cmd, parser;
	string str;
	unsigned pos, pos2;
	DEB_TRACE() << "xh status read";
	cmd << "xstrip timing read-status " << m_sysName;
	AutoMutex aLock(m_cond.mutex());
	DEB_TRACE() << "xh status send wait";
	m_xh->sendWait(cmd.str(), str);
	// aLock.unlock();
	DEB_TRACE() << "xh status " << str;
	pos = str.find(":");
	string state = str.substr (2, pos-2);
	if (state.compare("Idle") == 0) {
		status.state = XhStatus::Idle;
	} else if (state.compare("Paused at frame") == 0) {
		status.state = XhStatus::PausedAtFrame;
	} else if (state.compare("Paused at group") == 0) {
		status.state = XhStatus::PausedAtGroup;
	} else if (state.compare("Paused at scan") == 0) {
		status.state = XhStatus::PausedAtScan;
	} else {
		status.state = XhStatus::Running;
	}
	pos = str.find("=");
	pos2 = str.find(",", pos);
	std::stringstream ss1(str.substr(pos+1, pos2-pos));
	ss1 >> status.group_num ;
	pos = str.find("=", pos+1);
	pos2 = str.find(",", pos);
	std::stringstream ss2(str.substr(pos+1, pos2-pos));
	ss2 >> status.frame_num;
	pos = str.find("=", pos+1);
	pos2 = str.find(",", pos);
	std::stringstream ss3(str.substr(pos+1, pos2-pos));
	ss3 >> status.scan_num;
	pos = str.find("=", pos+1);
	pos2 = str.find(",", pos);
	std::stringstream ss4(str.substr(pos+1, pos2-pos));
	ss4 >> status.cycle;
	pos = str.find("=", pos+1);
	pos2 = str.find(",", pos);
	std::stringstream ss5(str.substr(pos+1, pos2-pos));
	ss5 >> status.completed_frames;

	DEB_TRACE() << "XhStatus.state is [" << status.state << "]";
	DEB_TRACE() << "XhStatus group " << status.group_num << " frame " << status.frame_num << " scan " << status.scan_num;
	DEB_TRACE() << "XhStatus cycles " << status.cycle << " completed " << status.completed_frames;
}

int Camera::getNbHwAcquiredFrames() {
	DEB_MEMBER_FUNCT();
	return m_acq_frame_nb;
}

void Camera::AcqThread::threadFunction() {
	DEB_MEMBER_FUNCT();
	StdBufferCbMgr& buffer_mgr = m_cam.m_bufferCtrlObj.getBuffer();
	bool continueFlag = true;
	m_thread_started = true;
	AutoMutex aLock(m_cam.m_cond.mutex());

	while (!m_cam.m_quit) {
		while (m_cam.m_wait_flag && !m_cam.m_quit) {
			DEB_TRACE() << "Wait";
			m_cam.m_thread_running = false;
			m_cam.m_cond.broadcast();
			m_cam.m_cond.wait();
		}
		DEB_TRACE() << "AcqThread Running";

		if (m_cam.m_quit)
			return;
		else
			m_cam.m_thread_running = true;

		m_cam.m_cond.broadcast();
		aLock.unlock();

		while (continueFlag && (m_cam.m_nb_frames_to_collect == 0 || m_cam.m_acq_frame_nb < m_cam.m_nb_frames_to_collect)) {
			XhStatus status;
			m_cam.getStatus(status);

			int nframes;
			nframes = m_cam.m_nb_frames_to_collect;
			if (status.state == status.Idle) {
				nframes = m_cam.m_nb_frames_to_collect - m_cam.m_acq_frame_nb;
			} else {
				nframes = status.completed_frames - m_cam.m_acq_frame_nb;
			}

			if (m_cam.m_trigger_mode == IntTrigMult) {
				nframes = 1;
				if (!m_cam.isSoftwareTriggerActive()) {
					m_cam.m_wait_flag = true;
					break;
				}
			}

			// TODO: what do we do with nb groups?
			// nframes *= m_cam.m_nb_groups;

			Bin bin; m_cam.getBin(bin);
			int bin_size_x = bin.getX();
			if (status.state == XhStatus::Idle || (status.completed_frames > m_cam.m_acq_frame_nb) ) {
				int32_t *dptr, *baseptr;
				int npoints = m_cam.m_npixels;
				if (m_cam.m_image_type == Bpp16) {
					npoints /= 2;
				}

				int width = m_cam.m_roi.getSize().getWidth();
				width = width ? width : npoints;
				int start = m_cam.m_roi.getTopLeft().x;

				dptr = (int32_t*)malloc(nframes * npoints * sizeof(int32_t));

				baseptr = dptr;
				// m_cam.setStatus(Camera::XhStatus::Exposure);
				m_cam.readFrame(dptr, m_cam.m_acq_frame_nb, 1);
				// m_cam.setStatus(Camera::XhStatus::Readout);
				for (int i=0; i<nframes; i++) {
					HwFrameInfoType frame_info;
					frame_info.acq_frame_nb = m_cam.m_acq_frame_nb;

					int32_t* bptr = (int32_t*)buffer_mgr.getFrameBufferPtr(m_cam.m_acq_frame_nb);
					std::vector<int32_t> dptr_swapped;
					for (unsigned int j = 0; j < npoints ; j++) {
						dptr_swapped.push_back(
							static_cast<int32_t>(__builtin_bswap32(dptr[j])) / nframes);
					}

					std::vector<int32_t> dptr_binned;
					if (bin_size_x != 1) {
						for (unsigned int ii = start; ii < width + start; ii+= bin_size_x) {
							int32_t sum = 0;
							for (unsigned int j = ii; j < ii + bin_size_x; j++)
								sum += dptr_swapped.at(j);
							dptr_binned.push_back(sum);
						}
						width /= bin_size_x;
						dptr = &dptr_binned[0];
					} else {
						dptr = &dptr_swapped[0];
					}
					memcpy(bptr, dptr + start, width * sizeof(int32_t));
					dptr += npoints;
					continueFlag = buffer_mgr.newFrameReady(frame_info);
					DEB_TRACE() << "acqThread::threadFunction() newframe ready ";
					++m_cam.m_acq_frame_nb;
				}
				
				free(baseptr);
			} else {
				AutoMutex aLock(m_cam.m_cond.mutex());
				continueFlag = !m_cam.m_wait_flag;
				if (m_cam.m_wait_flag) {
					stringstream cmd;
					cmd << "xstrip timing stop " << m_cam.m_sysName;
					m_cam.m_xh->sendWait(cmd.str());
				} else {
					usleep(1000);
				}
			}

		DEB_TRACE() << "acquired " << m_cam.m_acq_frame_nb << " frames, required " << m_cam.m_nb_frames_to_collect << " frames";
		
		}
		
		if (m_cam.m_trigger_mode == IntTrigMult) {
			stringstream cmd;
			cmd << "xstrip timing stop " << m_cam.m_sysName;
			m_cam.m_xh->sendWait(cmd.str());
		}
		aLock.lock();
		m_cam.m_wait_flag = true;
	}
}

Camera::AcqThread::AcqThread(Camera& cam) :
		m_cam(cam) {
	pthread_attr_setscope(&m_thread_attr, PTHREAD_SCOPE_PROCESS);
}

Camera::AcqThread::~AcqThread() {
	AutoMutex aLock(m_cam.m_cond.mutex());
	m_cam.m_quit = true;
	m_cam.m_cond.broadcast();
	aLock.unlock();
	join();
}

void Camera::getImageType(ImageType& type) {
	DEB_MEMBER_FUNCT();
	type = m_image_type;
}

void Camera::setImageType(ImageType type) {
	//this is what xh generates but may need Bpp32 for accummulate
	DEB_MEMBER_FUNCT();
	m_image_type = type;
}


void Camera::getDetectorType(std::string& type) {
	DEB_MEMBER_FUNCT();
	type = "xh";
}

void Camera::getDetectorModel(std::string& model) {
	DEB_MEMBER_FUNCT();
	stringstream ss;
	model = ss.str();
}

void Camera::getDetectorImageSize(Size& size) {
	DEB_MEMBER_FUNCT();
	size = Size(m_npixels, 1);
}

void Camera::getPixelSize(double& sizex, double& sizey) {
	DEB_MEMBER_FUNCT();
	sizex = xPixelSize;
	sizey = yPixelSize;
}

HwBufferCtrlObj* Camera::getBufferCtrlObj() {
	return &m_bufferCtrlObj;
}

void Camera::setTrigMode(TrigMode mode) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::setTrigMode - " << DEB_VAR1(mode);
	DEB_PARAM() << DEB_VAR1(mode);
	switch (mode) {
	case IntTrig:
	case IntTrigMult:
	case ExtTrigMult:
		m_trigger_mode = mode;
		break;
	case ExtTrigSingle:
	case ExtGate:
	case ExtStartStop:
	case ExtTrigReadout:
	default:
		THROW_HW_ERROR(Error) << "Cannot change the Trigger Mode of the camera, this mode is not managed !";
		break;
	}
}

void Camera::getTrigMode(TrigMode& mode) {
	DEB_MEMBER_FUNCT();
	mode = m_trigger_mode;
	DEB_RETURN() << DEB_VAR1(mode);
}

void Camera::getExpTime(double& exp_time) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::getExpTime";
	// convert to seconds
	// double timearray[] = {20*1e-9,22*1e-9,22*1e-9};
	// exp_time = m_exp_time * timearray[m_clock_mode];
	if (m_time_mode) { // if true then bunches are used
		exp_time = m_exp_time * m_clock_factor;
	} else {
		exp_time = m_exp_time;
	}
	m_exp_time = exp_time;
	DEB_RETURN() << DEB_VAR1(exp_time);
}

void Camera::setExpTime(double exp_time) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::setExpTime - " << DEB_VAR1(exp_time);
    // we receive seconds, 
	// double timearray[] = {20*1e-9,22*1e-9,22*1e-9};
	// m_exp_time = exp_time / timearray[m_clock_mode];
	if (m_time_mode) { // if true then bunches are used
		m_exp_time = exp_time / m_clock_factor;
	} else {
		m_exp_time = exp_time;
	}
	DEB_TRACE() << "Camera::setExpTime ------------------------------>"  << DEB_VAR1(m_exp_time) ;
}

void Camera::getNbGroups(int& nb_groups) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::getNbGroups";
	nb_groups = m_nb_groups;
	DEB_RETURN() << DEB_VAR1(nb_groups);
}

void Camera::setNbGroups(int nb_groups) {
	const int MAX_NUMBER_OF_GROUPS = 1024;
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::setNbGroups - " << DEB_VAR1(nb_groups);
	if (nb_groups >= MAX_NUMBER_OF_GROUPS) {
		THROW_HW_ERROR(Error) << "Number of groups cannot be bigger than " << MAX_NUMBER_OF_GROUPS;
	}
	m_nb_groups = nb_groups;
	DEB_RETURN() << DEB_VAR1(m_nb_groups) ;
}

void Camera::setLatTime(double lat_time) {
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(lat_time);

	if (lat_time != 0.)
		THROW_HW_ERROR(Error) << "Latency not managed";
}

void Camera::getLatTime(double& lat_time) {
	DEB_MEMBER_FUNCT();
	lat_time = 0;
}

void Camera::getExposureTimeRange(double& min_expo, double& max_expo) const {
	DEB_MEMBER_FUNCT();
	min_expo = 0.;
	max_expo = (double)UINT_MAX * 20e-9; //32bits x 20 ns
	DEB_RETURN() << DEB_VAR2(min_expo, max_expo);
}

void Camera::getLatTimeRange(double& min_lat, double& max_lat) const {
	DEB_MEMBER_FUNCT();
	// --- no info on min latency
	min_lat = 0.;
	// --- do not know how to get the max_lat, fix it as the max exposure time
	max_lat = (double) UINT_MAX * 20e-9;
	DEB_RETURN() << DEB_VAR2(min_lat, max_lat);
}

void Camera::setNbFrames(int nb_frames) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::setNbFrames - " << DEB_VAR1(nb_frames);
	if (m_nb_frames_to_collect < 0) {
		THROW_HW_ERROR(Error) << "Number of frames to acquire has not been set";
	}
	m_nb_frames_to_collect = nb_frames;
}

void Camera::getNbFrames(int& nb_frames) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::getNbFrames";
	DEB_RETURN() << DEB_VAR1(m_nb_frames_to_collect);
	nb_frames = m_nb_frames_to_collect;
}

void Camera::getNbScans(int& nb_scans) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::getNbscans";
	nb_scans = m_nb_scans;
}

void Camera::setNbScans(int nb_scans) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::setNbScans" << DEB_VAR1(nb_scans);
	m_nb_scans = nb_scans;
}

bool Camera::isAcqRunning() const {
	AutoMutex aLock(m_cond.mutex());
	return m_thread_running;
}

/////////////////////////
// xh specific stuff now
/////////////////////////

/*
 * Return a sequence of debug messages from the server for each command
 *
 * @return a vector of debug messages
 */
vector<string> Camera::getDebugMessages() {
	DEB_MEMBER_FUNCT();
	return m_xh->getDebugMessages();
}

/**
 * Software continue from wait before group, frame or scan.
 */
void Camera::continueAcq() {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip timing continue " << m_sysName;
	m_xh->sendWait(cmd.str());
}

/**
 * Set or Clear 16 bit readout mode
 *
 * @param[in] mode false=> 32 bit mode, true => 16 bit mode
 */
void Camera::set16BitReadout(bool mode) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	if (mode) {
		cmd << "xstrip mode16bit " << m_sysName << " 1";
		m_image_type = Bpp16;
	} else {
		cmd << "xstrip mode16bit " << m_sysName << " 0";
		m_image_type = Bpp32;
	}
	m_xh->sendWait(cmd.str());
}

/**
 * Set Dead pixels which are ignored in averaging mode.
 *
 * @param[in] first First dead pixel of sequence
 * @param[in] num Number of dead pixels
 * @param[in] reset Reset all to OK before setting these (default=> Add these as dead)
 */
void Camera::setDeadPixels(int first, int num, bool reset) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip set-dead-pixels " << m_sysName << " " << first << " " << num;
	if (reset)
		cmd << " reset";
	m_xh->sendWait(cmd.str());
}

/**
 * Set offset DACs on inteface card
 *
 * @param[in] first First DAC of sequence
 * @param[in] num Number of DACS in sequence
 * @param[in] direct Do not remap order
 */
void Camera::setOffsets(int first, int num, int value, bool direct) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip offsets set " << m_sysName << " " << first << " " << num << " " << value;
	if (direct)
		cmd << " direct";
	m_xh->sendWait(cmd.str());
}

/**
 * Set control DAC on HV power supply
 *
 * @param[in] value Value (Volts)
 * @param[in] noslew No slew rate limit during this update
 * @param[in] sign Override system and write to positive (+) or negative (-) supply, default=0
 * @param[in] direct Cast value to int (voltage to DAC code)
 */
void Camera::setHvDac(double value, bool noslew, int sign, bool direct){
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip hv set-dac " << m_sysName << " " << value;
	if (noslew)
		cmd << " noslew";
	if (direct)
		cmd << " direct";
	if (sign < 0)
		cmd << " neg";
	if (sign > 0)
		cmd << " pos";
	m_xh->sendWait(cmd.str());
}

/**
 * Get ADC value from HV power supply
 *
 * @param[out] value Returned Adc value
 * @param[in] hvmon HV monitor
 * @param[in] v12 12V monitor
 * @param[in] v5 5V2 monitor
 * @param[in] sign Override system and read from positive (+) or negative(-) supply, default=0
 * @param[in] direct Cast value to int (default converts voltage to DAC code)
 */
void Camera::getHvAdc(double& value, bool hvmon, bool v12, bool v5, int sign, bool direct) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip hv get-adc " << m_sysName;
	if (direct)
		cmd << " direct";
	if (sign < 0)
		cmd << " neg";
	if (sign > 0)
		cmd << " pos";
	if (hvmon)
		cmd << " hv";
	if (v12)
		cmd << " v12";
	if (v5)
		cmd << " v5";
	m_xh->sendWait(cmd.str(), value);
}

/**
 * Enable/disable HV supply
 *
 * @param[in] enable true = turn on supply, false = switch off
 * @param[in] overtemp Over temperature protection
 * @param[in] force Forced turn on
 */
void Camera::enableHv(bool enable, bool overtemp, bool force) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip hv " << m_sysName;
	if (enable) {
		if (overtemp)
			cmd << " auto";
		if (force)
			cmd << " forced-on";
	} else {
		cmd << " off";
	}
	m_xh->sendWait(cmd.str());
}

/**
 * List available capactance values for set-xchip-caps
 *
 * @param[out] capValues The capacitance values
 * @param[out] num The number of values
 * @param[out] alt_cd Specifies whether there is alternative CD
 */
void Camera::listAvailableCaps(std::vector<int>& capValues, bool& alt_cd) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	// example of capStr = "2 5 7 10 12 15 17 20 22 25 27 30 32 35 37 40 alternate-cd=1"
	string capStr;
	cmd << "xstrip head list-caps " << m_sysName;
	m_xh->sendWait(cmd.str(), capStr);
	int pos, pos2;
	int curpos = 0;
	pos = capStr.find("=");
	alt_cd = atoi(capStr.substr(pos+1).c_str());
	pos2 = capStr.find("alt");
	while (curpos < pos2) {
		pos = capStr.find(" ", curpos);		// position of " " in capStr
		capValues.push_back(atoi(capStr.substr(curpos, pos-curpos).c_str()));
		curpos = pos+1;
	}
}

/**
 * Set DACs on head
 *
 * @param[in] value The voltage (Volts)
 * @param[in] voltageType Specifies which adc {@see #HeadVoltageType}
 * @param[in] head Specify single head 0 or 1,(default both = -1)
 * @param[in] direct Cast value to int (default converts voltage to DAC code = false)
 */
void Camera::setHeadDac(double value, HeadVoltageType voltageType, int head, bool direct) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip head set-dac " << m_sysName <<  " " << value;
	switch (voltageType) {
	case XhVdd:
		cmd << " vdd";
		break;
	case XhVref:
		cmd << " vref";
		break;
	case XhVrefc:
		cmd << " vrefc";
		break;
	case XhVres1:
		cmd << " vres1";
		break;
	case XhVres2:
		cmd << " vres2";
		break;
	case XhVpupref:
		cmd << " vpupref";
		break;
	case XhVclamp:
		cmd << " vclamp";
		break;
	case XhVled:
		cmd << " vled";
		break;
	}
	if (head != -1)
		cmd << " head " << head;
	if (direct)
		cmd << " direct";
	m_xh->sendWait(cmd.str());
}

/**
 *	Get ADC value from head
 *
 *	@param[out] value The returned adc value
 *	@param[in] head The head number
 *	@param[in] voltageType Specifies which adc {@see #HeadVoltageType}
 */
void Camera::getHeadAdc(double& value, int head, HeadVoltageType voltageType) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip head get-adc " << m_sysName <<  " " << head;
	switch (voltageType) {
	case XhVdd:
		cmd << " vdd";
		break;
	case XhVref:
		cmd << " vref";
		break;
	case XhVrefc:
		cmd << " vrefc";
		break;
	case XhVres1:
		cmd << " vres1";
		break;
	case XhVres2:
		cmd << " vres2";
		break;
	case XhVpupref:
		cmd << " vpupref";
		break;
	case XhVclamp:
		cmd << " vclamp";
		break;
	case XhVled:
		cmd << " vled";
		break;
	}
	m_xh->sendWait(cmd.str(), value);
}

/**
 * Set LED enable signal only in Parallel Expander on head
 *
 * @param[in] 0 => LED off, 1 => LED On (appropriate inversion applied in library)
 * @param[in] Specify single head 0 or 1,(default both = -1)
 */
void Camera::setCalEn(bool onOff, int head) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip head set-cal-en " << m_sysName << " " << onOff;
	if (head != -1)
		cmd << " head " << head;
	m_xh->sendWait(cmd.str());
}

/**
 * Set Caps on XChips (any version) for values {@see Camera::listAvailableCaps}
 *
 * @param[in] Capacitance for common or AB caps
 * @param[in] Capactiance for CD caps
 * @param[in] Specify single head 0 or 1,(default both = -1)
 */
void Camera::setHeadCaps(int capsAB, int capsCD, int head) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip head set-xchip-caps " << m_sysName <<  " " << capsAB << " " << capsCD;
	if (head != -1)
		cmd << " head " << head;
	m_xh->sendWait(cmd.str());
}

/**
 * Sets high voltage on
 *
 */
void Camera::setHighVoltageOn() {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip hv enable " << m_sysName <<  " auto";
	m_xh->sendWait(cmd.str());
	cmd << "xstrip hv set-dac " << m_sysName <<  " -90";
	m_xh->sendWait(cmd.str());
}

/**
 * Sets high voltage of
 *
 */
void Camera::setHighVoltageOff() {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip hv enable " << m_sysName <<  " off";
	m_xh->sendWait(cmd.str());
}

/**
 * Set default timing parameters
 *
 * @param[out] timingParams  {@see Camera::XhTimingParameters}
 */
void Camera::setDefaultTimingParameters(XhTimingParameters& timingParams) {
	timingParams.trigControl=XhTrigIn_noTrigger;
	timingParams.trigMux=-1;
	timingParams.orbitMux = -1;
	timingParams.lemoOut = std::vector<int>();
	timingParams.correctRounding = false;
	timingParams.groupDelay = 0;
	timingParams.frameDelay = 0;
	timingParams.scanPeriod = 0;
	timingParams.auxDelay = 0;
	timingParams.auxWidth = 1;
	timingParams.longS12 = 0;
	timingParams.frameTime = 0;
	timingParams.shiftDown = 0;
	timingParams.cyclesStart = 1;
	timingParams.cyclesEnd = false;
	timingParams.s1Delay = 0;
	timingParams.s2Delay = 0;
	timingParams.xclkDelay = 0;
	timingParams.rstRDelay = 0;
	timingParams.rstFDelay = 0;
	timingParams.allowExcess = false;
	
}


std::string Camera::join(std::vector<int>::const_iterator begin, std::vector<int>::const_iterator last, const std::string& delimeter) {
	std::ostringstream result;
	if (begin != last) {
		result << *begin;
		while (++begin != last)
			result << delimeter << *begin;
	}

	return result.str();
}
/**
 * Setup a single timing group.
 *
 * @param[in] groupNum Group number (0..n-1)
 * @param[in] nframes Number of frames
 * @param[in] nscans Number of scans per frame (0 to calc maximum possible)
 * @param[in] intTime Integration time (20 ns cycles)
 * @param[in] last true => Last group of experiment => write all to memory
 * @param[in] timingParams Additional timing parameters {@see Camera::XhTimingParameters}
 */
void Camera::setTimingGroup(int groupNum, int nframes, int nscans, int intTime, bool last, const XhTimingParameters& timingParams) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip timing setup-group " << m_sysName << " " << groupNum << " " << nframes << " " << nscans << " "
			<< intTime;

	switch (timingParams.trigControl & (Camera::XhTrigIn_groupTrigger | Camera::XhTrigIn_frameTrigger | Camera::XhTrigIn_scanTrigger)) {
	case Camera::XhTrigIn_groupTrigger:
		cmd << " ext-trig-group";
		break;
	case Camera::XhTrigIn_frameTrigger:
		cmd << " ext-trig-frame";
		break;
	case Camera::XhTrigIn_groupTrigger | Camera::XhTrigIn_frameTrigger:
		cmd << " ext-trig-group ext-trig-frame";
		break;
	case Camera::XhTrigIn_scanTrigger:
		cmd << " ext-trig-scan";
		break;
	case Camera::XhTrigIn_groupTrigger | Camera::XhTrigIn_scanTrigger:
		cmd << " ext-trig-group ext-trig-frame ext-trig-scan";
		break;
	}
	switch (timingParams.trigControl & (Camera::XhTrigIn_groupOrbit | Camera::XhTrigIn_frameOrbit | Camera::XhTrigIn_scanOrbit)) {
	case Camera::XhTrigIn_groupOrbit:
		cmd << " orbit-group";
		break;
	case Camera::XhTrigIn_groupOrbit | Camera::XhTrigIn_frameOrbit:
		cmd << " orbit-group orbit-frame";
		break;
	case Camera::XhTrigIn_groupOrbit | Camera::XhTrigIn_frameOrbit | Camera::XhTrigIn_scanOrbit:
		cmd << " orbit-group orbit-frame orbit-scan";
		break;
	case Camera::XhTrigIn_frameOrbit:
		cmd << " orbit-frame";
		break;
	case Camera::XhTrigIn_scanOrbit:
		cmd << " orbit-scan";
		break;
	}
	if (timingParams.trigControl & Camera::XhTrigIn_fallingTrigger)
		cmd << " trig-falling";

	if (timingParams.trigMux != -1 && (timingParams.trigControl & (Camera::XhTrigIn_groupTrigger | Camera::XhTrigIn_frameTrigger | Camera::XhTrigIn_scanTrigger)))
		cmd << " trig-mux " << timingParams.trigMux;
	if (timingParams.orbitMux != -1)
		cmd << " orbit-mux " << timingParams.orbitMux;
	if (!timingParams.lemoOut.empty()) {
		std::string lemosCmd = Camera::join(timingParams.lemoOut.begin(), timingParams.lemoOut.end());
		cmd << " lemo-out " << lemosCmd;
	}
	if (timingParams.correctRounding)
		cmd << " correct-rounding";
	if (timingParams.groupDelay != -1)
		cmd << " group-delay " << timingParams.groupDelay;
	if (timingParams.frameDelay != 0)
		cmd << " frame-delay " << timingParams.frameDelay;
	if (timingParams.scanPeriod != 0)
		cmd << " scan-period " << timingParams.scanPeriod;
	if (timingParams.auxDelay != 0)
		cmd << " aux-delay " << timingParams.auxDelay;
	if (timingParams.auxWidth != 1)
		cmd << " aux-width " << timingParams.auxWidth;
	if (timingParams.longS12 != 0)
		cmd << " long-s1 " << timingParams.longS12;
	if (timingParams.frameTime != 0)
		cmd << " frame-time " << timingParams.frameTime;
	if (timingParams.shiftDown != 0)
		cmd << " shift-down " << timingParams.shiftDown;
	if (timingParams.cyclesStart != 1)
		cmd << " cycles-start " << timingParams.cyclesStart;
	if (timingParams.cyclesEnd)
		cmd << " cycles-end";
	if (timingParams.s1Delay != 0)
		cmd << " s1-delay " << timingParams.s1Delay;
	if (timingParams.s2Delay != 0)
		cmd << " s2-delay " << timingParams.s2Delay;
	if (timingParams.xclkDelay != 0)
		cmd << " xclk-delay " << timingParams.xclkDelay;
	if (timingParams.rstRDelay != 0)
		cmd << " rst-r-delay " << timingParams.rstRDelay;
	if (timingParams.rstFDelay != 0)
		cmd << " rst-f-delay " << timingParams.rstFDelay;
	if (timingParams.allowExcess)
		cmd << " allow-excess";

	if (last)
		cmd << " last";

	int num_frames;
	m_xh->sendWait(cmd.str(), num_frames);

	// TODO: needed?
	if (timingParams.trigControl != Camera::XhTrigIn_noTrigger) {
		setTrigMode(ExtTrigMult);
	}
	if (timingParams.trigMux == 9) {
		setTrigMode(IntTrigMult);
	}
	if (last) {
		if (m_trigger_mode != IntTrigMult) 
			m_nb_frames_to_collect = num_frames;
	}
	DEB_TRACE() << "m_nb_frames_to_collect " << m_nb_frames_to_collect;
}

/**
 * Modify timing group setting before writing with last
 *
 * @param[in] group_num The group to modify
 * @param[in] fixed_reset Fixed time from reset negated to S1 asserted
 * @param[in] allowExcess Allow programming of more frames than will fit in DRAM, for manual probing
 * @param[in] last If true mark this group as the last group to be configured in the detector, writes all groups afterwards.
 */
void Camera::modifyTimingGroup(int group_num, int fixed_reset, bool allowExcess, bool last){
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip timing modify-group " << m_sysName <<  " " << group_num;
	if (last)
		cmd << " last";
	if (allowExcess)
		cmd << " allow-excess";
	if (fixed_reset != -1)
		cmd << " fixed-rst-s1 " << fixed_reset;
	m_xh->sendWait(cmd.str());
}

/**
 * Setup One external trigger output.
 *
 * @param[in] trigNum Output number 0..7 or -1 for all
 * @param[in] trigout Trigger output type {@link #TriggerOutputType}
 * @param[in] width Pulse Width 1.. 255 cycles of 50 MHz
 * @param[in] invert Invert output so it idles high
 */
void Camera::setExtTrigOutput(int trigNum, TriggerOutputType trigout, int width, bool invert) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip timing ext-output " << m_sysName <<  " " << trigNum;
	switch (trigout) {
	case XhTrigOut_dc:
		cmd << " dc";
		break;
	case XhTrigOut_wholeGroup:
		cmd << " whole-group";
		break;
	case XhTrigOut_groupPreDel:
		cmd << " group-pre-delay";
		break;
	case XhTrigOut_groupPostDel:
		cmd << " group-post-delay";
		break;
	case XhTrigOut_framePreDel:
		cmd << " frame-pre-delay";
		break;
	case XhTrigOut_framePostDel:
		cmd << " frame-post-delay";
		break;
	case XhTrigOut_scanPreDel:
		cmd << " scan-pre-delay";
		break;
	case XhTrigOut_scanPostDel:
		cmd << " scan-post-delay";
		break;
	case XhTrigOut_integration:
		cmd << " integration";
		break;
	case XhTrigOut_aux1:
		cmd << " aux1";
		break;
	case XhTrigOut_waitExtTrig:
		cmd << " waiting-trig";
		break;
	case XhTrigOut_waitOrbitSync:
		cmd << " waiting orbit";
		break;
	}
	if (width > 0)
		cmd << " width " << width;
	if (invert)
		cmd << " invert";
	m_xh->sendWait(cmd.str());
}

/**
 * Setup LED stretch Time.
 *
 * @param[in] pause_time Waiting for trigger minimum on time in 0.32768 ms cycles
 * @param[in] frame_time Frame change flash time in 0.32768 ms cycles
 * @param[in] int_time Minimum integration time Flash in 0.32768 ms cycles
 * @param[in] wait_for_trig Flash Waiting LED when waiting for Orbit as well as ext trig
 */
void Camera::setLedTiming(int pause_time, int frame_time, int int_time, bool wait_for_trig){
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip timing setup-leds " << m_sysName <<  " " << pause_time << " " << frame_time << " " << int_time;
	if (wait_for_trig)
		cmd << " inc-orbit";
	m_xh->sendWait(cmd.str());
}

/**
 * Get the configured timing data.
 *
 * @param[out] buff A pointer to a the buffer in which the timing data is returned.
 * @param[in] firstParam First parameter number to be output [0...29]
 * @param[in] nParams The number of parameters  [1...30]
 * @param[in] firstGroup The first group to output [0...n-]] where n is the number of groups configured
 * @param[in] nGroups The number of groups to output [1...n] where n is the number of groups configured
 */
void Camera::getTimingInfo(unsigned int* buff, int firstParam, int nParams, int firstGroup, int nGroups) {
	DEB_MEMBER_FUNCT();
	int timingHandle;
	unsigned char* bptr = (unsigned char*) buff;
	stringstream cmd, cmd1, cmd2;
	cmd << "xstrip timing open " << m_sysName;
	m_xh->sendWait(cmd.str(), timingHandle);
	int num = 30 * m_nb_groups * sizeof(long);
	cmd1 << "read " << firstParam << " " << firstGroup << " 0 " << nParams << " " << nGroups << " 1" << " from " << timingHandle << " long";
	m_xh->sendWait(cmd1.str());
	m_xh->getData(bptr, num);
	cmd2 << "close " << timingHandle;
	m_xh->sendWait(cmd2.str());

//	uint32_t *dptr = (uint32_t *)bptr;
}

/**
 * Setting ADC Clock chips for internal or external clock source.
 *
 * @param[in] clockMode selected from {@link #ClockModeType}
 * @param[in] pll_gain Set PLL Charge Pump gain 0..3
 * @param[in] extra_div Set extra divide in R and N (default 1)
 * @param[in] caps Low Pass Filter Capacitance 0..1
 * @param[in] r3 Low Pass Filter R3 0..4
 * @param[in] r4 Low Pass Filter R4 0..4
 * @param[in] stage1
 * @param[in] nocheck
 */
void Camera::setupClock(ClockModeType clockMode, int pll_gain, int extra_div, int caps, int r3, int r4, bool stage1, bool nocheck) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip clock setup " << m_sysName;
	if (clockMode == XhESRF5468MHz)
		cmd << " esrf";
	if (clockMode == XhESRF1136MHz)
		cmd << " esrf11";
	m_clock_mode = clockMode;
	if (stage1)
		cmd << " stage1";
	if (nocheck)
		cmd << " no-check";
	if (pll_gain > 0)
		cmd << " pll-gain " << pll_gain;
	if (extra_div > 0)
		cmd << " extra-div " << extra_div;
	if (caps > 0)
		cmd << " caps " << caps;
	if (r3 > 0)
		cmd << " r3 " << r3;
	if (r4 > 0)
		cmd << " r4 " << r4;
	m_xh->sendWait(cmd.str());
}

/**
 * Get the current setpoint from one of the 4 head sensors.
 *
 * @param[in] channel 0->3=temperature sensors,
 * @param[out] value The current setpoint in degrees C.
 */
void Camera::getSetpoint(int channel, double& value) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip tc get " << m_sysName <<  " ch " << channel << " setpoint";
	m_xh->sendWait(cmd.str(), value);
}

/**
 * Get the current temperature from one of the 4 head sensors.
 *
 * channel 0->3=temperature sensors,
 * @param[out] temperatures The output temperature vector in degrees C for all sensors
 */
void Camera::getTemperature(std::vector<double> &temperatures) {
	DEB_MEMBER_FUNCT();
	int NUMER_OF_TEMP_CHANNELS = 4;
	for (int channel = 0; channel < NUMER_OF_TEMP_CHANNELS; channel++) {
		double value;
		stringstream cmd;
		cmd << "xstrip tc get " << m_sysName <<  " ch " << channel << " t";
		m_xh->sendWait(cmd.str(), value);
		temperatures.push_back(value);
	}
}

/**
 * Set a calibration image using Xchip Vres.
 *
 * @param[in] imageScale The mage scale as %
 */
void Camera::setCalImage(double imageScale) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip head set-cal-image " << m_sysName <<  " " << imageScale;
	m_xh->sendWait(cmd.str());
}

/**
 * Set Delay between XCHIP Rst, S1, S2, ShiftIn & XClk and data
 *
 * @param[in] delay The delay value (0..63, -1 => default)
 */
void Camera::setXDelay(int delay) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip set-x-delay " << m_sysName <<  " " << delay;
	m_xh->sendWait(cmd.str());
}

/**
 * Sync ADC Clocks
 */
void Camera::syncClock() {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip test sync-clock " << m_sysName;
	m_xh->sendWait(cmd.str());
}

/**
 * Send a command directly to the server. Not recommend for general use.
 *
 * @param[in] cmd A server command
 */
void Camera::sendCommand(string cmd) {
	DEB_MEMBER_FUNCT();
	m_xh->sendWait(cmd);
}

double Camera::sendCommandNumber(string cmd) {
	DEB_MEMBER_FUNCT();
	double value;
	m_xh->sendWait(cmd, value);
	return value;
}

string Camera::sendCommandString(string cmd) {
	DEB_MEMBER_FUNCT();
	std::string value;
	m_xh->sendWait(cmd, value);
	return value;
}

void Camera::setRoi(const Roi& roi_to_set) {
	DEB_MEMBER_FUNCT();
	int w = roi_to_set.getSize().getWidth();
	int h = roi_to_set.getSize().getHeight();
	int x = roi_to_set.getTopLeft().x;
	int y = roi_to_set.getTopLeft().y;

	Roi roi = Roi(x, 0, w, 0);
	m_roi = roi;
}

void Camera::getRoi(Roi& roi) {
	DEB_MEMBER_FUNCT();
	roi = m_roi;
}

void Camera::checkRoi(const Roi& set_roi, Roi& hw_roi) {
	DEB_MEMBER_FUNCT();
	hw_roi = set_roi;
	DEB_RETURN() << DEB_VAR1(hw_roi);
}

/**
 * Get the maximum number of frames programable with the current XH configuration.
 *
 * @param[out] nframes return the number of h/w configured time frames
 */
void Camera::getMaxFrames(string& nframes) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "%xstrip_num_tf";
	m_xh->sendWait(cmd.str(), nframes);
}

/**
 * Get the number of h/w configured time frames.
 *
 * @param[out] nframes return the number of h/w configured time frames
 */
void Camera::getTotalFrames(int& nframes) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "%xstrip_num_tf";
	m_xh->sendWait(cmd.str(), nframes);
}


void Camera::getAvailableTriggerModes(std::vector<std::string> &trigger_list) {
	DEB_MEMBER_FUNCT();
	std::vector<std::string> triggers;

    for(TriggerNamesMap::iterator i = triggerNames.begin();
	i != triggerNames.end();++i)
      triggers.push_back(i->second);

	trigger_list = triggers;

	DEB_RETURN() << DEB_VAR1(triggers);
}

void Camera::setCustomTriggerMode(std::string trig_mode) {
	DEB_MEMBER_FUNCT();
	bool found = false;
	TriggerControlType customTrigger;
	for(TriggerNamesMap::iterator i = triggerNames.begin(); i != triggerNames.end();++i)
      if (i->second == trig_mode) {
		  found = true;
		  customTrigger = i->first;
		  break;
	  }

	if (!found) {
		THROW_HW_ERROR(Error) << "Trigger mode not found";
	}

	m_timingParams.trigControl = customTrigger;
}

void Camera::getCustomTriggerMode(std::string& trig_mode) {
	DEB_MEMBER_FUNCT();
	trig_mode = triggerNames[m_timingParams.trigControl];
	DEB_RETURN() << DEB_VAR1(trig_mode);
}

/**
 * Sets the input to wait for
 *
 * 9 - Software trigger
 * 8 - main trigger, delayed orbit
 * 0...7 - Lemo
 * 
 * @param[out] trigMux return the trigger input line number
 */
void Camera::setTrigMux(int trigMux) {
	DEB_MEMBER_FUNCT();
	// if (trigMux == 9)
	// 	if (m_timingParams.trigControl & (Camera::XhTrigIn_groupTrigger | Camera::XhTrigIn_frameTrigger | Camera::XhTrigIn_scanTrigger)) {
	// 		m_timingParams.trigMux = trigMux;
	// 	} else {
	// 		THROW_HW_ERROR(Error) << "Cannot set trigger mux for current trigger mode";
	// 	}
	// if (8 >= trigMux >= 0) {
		m_timingParams.trigMux = trigMux;
	// }
	

	DEB_TRACE() << "Set trig mux to" << trigMux;
}

void Camera::getTrigMux(int &trigMux) {
	DEB_MEMBER_FUNCT();
	trigMux = m_timingParams.trigMux;
}

/**
 * Sets trigger orbit
 *
 * 0=direct
 * 1=delayed
 * 2=LVDS direct
 * 3=LVDS delayed
 * LEMO7 is reserved for the orbit trigger
 * 
 * @param[out] orbitMux return the orbit trigger
 */
void Camera::setOrbitTrig(int orbitMux) {
	DEB_MEMBER_FUNCT();
	if (3 >= orbitMux || orbitMux <= 0) {
		m_timingParams.orbitMux = orbitMux;
	} else {
		THROW_HW_ERROR(Error) << "Invalid orbit mux value";
	}
}

void Camera::getOrbitTrig(int &orbitMux) {
	DEB_MEMBER_FUNCT();

	orbitMux = m_timingParams.orbitMux;
}

/**
 * Sets lemo out
 *
 * LEMO7 is reserved for the orbit trigger
 * Lemo can be set as single integer value, or multiple values
 * value is mapped to the lemo output. Mapping is done using binary coding
 * where 2 bits are used for single output. When multiple values are provided
 * binary OR is calculated and corresponding outputs are then activated
 * 
 * @param[in] lemoOut vector of lemoOuts
 */
void Camera::setLemoOut(std::vector<int> lemoOut) {
	DEB_MEMBER_FUNCT();

	m_timingParams.lemoOut = lemoOut;
}


/**
 * Gets lemo out value
 * 
 * @param[out] lemoOut return the vector of lemoOuts
 */
void Camera::getLemoOut(std::vector<int>& lemoOut) {
	DEB_MEMBER_FUNCT();
	lemoOut = m_timingParams.lemoOut;
}

/**
 * Sets correct rounding
 * add/no add a rounding error to the frame delay
 *
 * 
 * @param[in] correctRounding
 */

void Camera::setCorrectRounding(bool correctRounding) {
	DEB_MEMBER_FUNCT();

	m_timingParams.correctRounding = correctRounding;
}

/**
 * Gets correct rounding
 * add/no add a rounding error to the frame delay
 *
 * 
 * @param[out] correctRounding
 */
void Camera::getCorrectRounding(bool& correctRounding) {
	DEB_MEMBER_FUNCT();
	correctRounding = m_timingParams.correctRounding;
}


/**
 * Sets group delay
 * delay is added before group
 *
 * 
 * @param[out] groupDelay
 */

void Camera::setGroupDelay(int groupDelay) {
	DEB_MEMBER_FUNCT();

	m_timingParams.groupDelay = groupDelay;
}

void Camera::getGroupDelay(int& groupDelay) {
	DEB_MEMBER_FUNCT();
	groupDelay = m_timingParams.groupDelay;
}

/**
 * Sets group delay
 * delay is added before group
 *
 * 
 * @param[out] frame
 */

void Camera::setFrameDelay(int frame) {
	DEB_MEMBER_FUNCT();

	m_timingParams.frameDelay = frame;
}

void Camera::getFrameDelay(int& frame) {
	DEB_MEMBER_FUNCT();
	frame = m_timingParams.frameDelay;
}

/**
 * Sets scan period
 * specifies scan to scan time in cloc cycles
 *
 * 
 * @param[out] scanPeriod
 */

void Camera::setScanPeriod(int scanPeriod) {
	DEB_MEMBER_FUNCT();

	m_timingParams.scanPeriod = scanPeriod;
}

/**
 * Gets scan period
 * 
 * @param[out] scanPeriod
 */

void Camera::getScanPeriod(int& scanPeriod) {
	DEB_MEMBER_FUNCT();
	scanPeriod = m_timingParams.scanPeriod;
}

/**
 * Sets frame time
 * Specify Frame time, calculate number of scans per frame if number of scans set to 0
 * frame time is provided in cycles
 *
 * 
 * @param[out] frameTime
 */

void Camera::setFrameTime(int frameTime) {
	// Should refer to fremas count ?
	DEB_MEMBER_FUNCT();

	m_timingParams.frameTime = frameTime;
}

void Camera::getFrameTime(int& frameTime) {
	DEB_MEMBER_FUNCT();
	frameTime = m_timingParams.frameTime;
}

/**
 * Sets shift down
 * shift down data 0..4 bits for averaging
 *
 * 
 * @param[out] shiftDown number of bits of right shift to divide total (default 4). 
 */

void Camera::setShiftDown(int shiftDown) {
	DEB_MEMBER_FUNCT();

	m_timingParams.shiftDown = shiftDown;
}

void Camera::getShiftDown(int& shiftDown) {
	DEB_MEMBER_FUNCT();
	shiftDown = m_timingParams.shiftDown;
}

/**
 * Sets cycles start
 *
 * 
 * @param[out] cyclesStart
 */

void Camera::setCyclesStart(int cyclesStart) {
	DEB_MEMBER_FUNCT();

	m_timingParams.cyclesStart = cyclesStart;
}

void Camera::getCyclesStart(int& cyclesStart) {
	DEB_MEMBER_FUNCT();
	cyclesStart = m_timingParams.cyclesStart;
}


/**
 * Sets cycles end
 *
 * 
 * @param[out] cyclesEnd
 */

void Camera::setCyclesEnd(int cyclesEnd) {
	DEB_MEMBER_FUNCT();
	if (minCycles <= cyclesEnd >= maxCycles) {
		m_timingParams.cyclesEnd = cyclesEnd;
	} else {
		THROW_HW_ERROR(Error) << "value must be in range: " << minCycles << " to " << maxCycles;
	}

}

void Camera::getCyclesEnd(int& cyclesEnd) {
	DEB_MEMBER_FUNCT();
	cyclesEnd = m_timingParams.cyclesEnd;
}

/**
 * Sets s2 delay
 *
 * 
 * @param[out] s1Delay
 */

void Camera::setS1Delay(int s1Delay) {
	DEB_MEMBER_FUNCT();

	m_timingParams.s1Delay = s1Delay;
}

void Camera::getS1Delay(int& s1Delay) {
	DEB_MEMBER_FUNCT();
	s1Delay = m_timingParams.s1Delay;
}

/**
 * Sets s2 delay
 *
 * 
 * @param[out] s2Delay
 */

void Camera::setS2Delay(int s2Delay) {
	DEB_MEMBER_FUNCT();

	m_timingParams.s2Delay = s2Delay;
}

void Camera::getS2Delay(int& s2Delay) {
	DEB_MEMBER_FUNCT();
	s2Delay = m_timingParams.s2Delay;
}

/**
 * Sets xclk delay
 *
 * 
 * @param[out] xclkDelay
 */

void Camera::setXclkDelay(int xclkDelay) {
	DEB_MEMBER_FUNCT();

	m_timingParams.xclkDelay = xclkDelay;
}

void Camera::getXclkDelay(int& xclkDelay) {
	DEB_MEMBER_FUNCT();
	xclkDelay = m_timingParams.xclkDelay;
}

/**
 * Sets rst R delay
 *
 * 
 * @param[out] rstRDelay
 */

void Camera::setRstRDelay(int rstRDelay) {
	DEB_MEMBER_FUNCT();

	m_timingParams.rstRDelay = rstRDelay;
}

void Camera::getRstRDelay(int& rstRDelay) {
	DEB_MEMBER_FUNCT();
	rstRDelay = m_timingParams.rstRDelay;
}

/**
 * Sets rst F delay
 *
 * 
 * @param[out] rstFDelay
 */

void Camera::setRstFDelay(int rstFDelay) {
	DEB_MEMBER_FUNCT();

	m_timingParams.rstFDelay = rstFDelay;
}

void Camera::getRstFDelay(int& rstFDelay) {
	DEB_MEMBER_FUNCT();
	rstFDelay = m_timingParams.rstFDelay;
}

/**
 * Allow excess
 * Allow programming of more frame than will fit in DRAM
 *
 * 
 * @param[out] allowExcess
 */

void Camera::setAllowExcess (bool allowExcess) {
	DEB_MEMBER_FUNCT();

	m_timingParams.allowExcess  = allowExcess;
}

void Camera::getAllowExcess (bool& allowExcess) {
	DEB_MEMBER_FUNCT();
	allowExcess  = m_timingParams.allowExcess;
}

/**
 * Read Bias
 *
 * 
 * @param[out] bias
 */

void Camera::getBias(double& bias) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip hv get-adc " << m_sysName << " ibias";
	m_xh->sendWait(cmd.str(), bias);
	DEB_RETURN() << DEB_VAR1(bias);
}

/**
 * Read capa
 *
 * 
 * @param[out] capa
 */

void Camera::getCapa(double& capa) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip head get-caps " << m_sysName << " 0";
	m_xh->sendWait(cmd.str(), capa);
	DEB_RETURN() << DEB_VAR1(capa);
}

/**
 * Write capa
 *
 * 
 * @param[in] capa
 */

void Camera::setCapa(double capa) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip head set-dac " << m_sysName <<  " " << capa;
	m_xh->sendWait(cmd.str());
	m_capa = capa;
}

/**
 * Read time_mode
 *
 * 
 * @param[out] time_mode
 */

void Camera::getTimeMode(bool& time_mode) {
	DEB_MEMBER_FUNCT();
	time_mode = m_time_mode;
	DEB_RETURN() << DEB_VAR1(time_mode);
}

/**
 * Write time_mode
 *
 * 
 * @param[in] time_mode
 */

void Camera::setTimeMode(bool time_mode) {
	DEB_MEMBER_FUNCT();
	m_time_mode = time_mode;
}

/**
 * Read orbit delay
 *
 * 
 * @param[out] orbit delay
 */

void Camera::getOrbitDelay(double& orbit_delay) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip timing xstrip_timing_orbit_read " << m_sysName;
	m_xh->sendWait(cmd.str(), orbit_delay);
	DEB_RETURN() << DEB_VAR1(orbit_delay);
}

void Camera::setOrbitDelay(double orbit_delay) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip timing setup-orbit " << m_sysName <<  " " << orbit_delay;
	if (m_orbit_delay_falling) {
		cmd << " falling";
	}
	m_xh->sendWait(cmd.str());
	m_orbit_delay = orbit_delay;
}

/**
 * Read orbit delay falling
 *
 * 
 * @param[out] orbit delay type
 */

void Camera::getOrbitDelayFalling(double& orbit_delay_falling) {
	DEB_MEMBER_FUNCT();
	orbit_delay_falling = m_orbit_delay_falling;
	DEB_RETURN() << DEB_VAR1(orbit_delay_falling);
}

void Camera::setOrbitDelayFalling(double orbit_delay_falling) {
	DEB_MEMBER_FUNCT();
	m_orbit_delay_falling = orbit_delay_falling;
	m_orbit_delay_rising = !orbit_delay_falling;
	DEB_RETURN() << DEB_VAR1(orbit_delay_falling);
}


/**
 * Read orbit delay rising
 *
 * 
 * @param[out] orbit delay type
 */

void Camera::getOrbitDelayRising(double& orbit_delay_rising) {
	DEB_MEMBER_FUNCT();
	orbit_delay_rising = m_orbit_delay_rising;
	DEB_RETURN() << DEB_VAR1(orbit_delay_rising);
}

void Camera::setOrbitDelayRising(double orbit_delay_rising) {
	DEB_MEMBER_FUNCT();
	m_orbit_delay_rising = orbit_delay_rising;
	m_orbit_delay_falling = !orbit_delay_rising;
	DEB_RETURN() << DEB_VAR1(orbit_delay_rising);
}

/**
 * Set voltage
 *
 * 
 * @param[out] allowExcess
 */

void Camera::setVoltage(double voltage) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip hv set-dac " << m_sysName << " " << voltage;
	m_xh->sendWait(cmd.str());
	m_voltage = voltage;
}

void Camera::getVoltage(double& voltage) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip hv get-adc " << m_sysName;
	m_xh->sendWait(cmd.str(), voltage);
	// voltage = m_voltage;
	DEB_RETURN() << DEB_VAR1(voltage);
}

/**
 * Sets aux delay
 *
 * 
 * @param[out] auxDelay
 */

void Camera::setAuxDelay(int auxDelay) {
	// TODO: Don’t use in delayed mux reset from aux modes.
	DEB_MEMBER_FUNCT();

	m_timingParams.auxDelay = auxDelay;
}

void Camera::getAuxDelay(int& auxDelay) const {
	DEB_MEMBER_FUNCT();
	auxDelay = m_timingParams.auxDelay;
}

/**
 * Sets aux width
 *
 * 
 * @param[out] auxWidth
 */

void Camera::setAuxWidth(int auxWidth) {
	// TODO: Don’t use in delayed mux reset from aux modes. 
	DEB_MEMBER_FUNCT();

	m_timingParams.auxWidth = auxWidth;
}

void Camera::getAuxWidth(int& auxWidth) {
	DEB_MEMBER_FUNCT();
	auxWidth = m_timingParams.auxWidth;
}

void Camera::setBin(const Bin &binning) {
	m_binning = binning;
}

void Camera::getBin(Bin &binning) {
	binning = m_binning;
}

void Camera::checkBin(Bin &binning) {
	int bin_x = binning.getX();
	int bin_y = binning.getY();

	// Max vertical dimension is 1 since we receive a signal from detector
	if (bin_y > 1) {
		bin_y = 1;
	}

	binning = Bin(bin_x, bin_y);
}

void Camera::setTrigGroupMode(int trig_mode) { 
	DEB_MEMBER_FUNCT();
	m_trig_group_mode = trig_mode;
	if (m_trig_group_mode == 0) {
		setCustomTriggerMode("no_trigger");
	} else if (m_trig_group_mode == 1) {
		setCustomTriggerMode("group_trigger");
	} else if (m_trig_group_mode == 2) {
		setCustomTriggerMode("group_orbit");
		setOrbitTrig(3);
	} else {
		THROW_HW_ERROR(Error) << "Trig_group_mode must be between 0-2";
	}
}

void Camera::getTrigGroupMode(int& trig_mode) { 
	trig_mode = m_trig_group_mode;
}

void Camera::setTrigScanMode(int trig_mode) { 
	DEB_MEMBER_FUNCT();
	m_trig_scan_mode = trig_mode;
	if (m_trig_scan_mode == 0) {
		setCustomTriggerMode("no_trigger");
	} else if (m_trig_scan_mode == 1) {
		setCustomTriggerMode("scan_trigger");
	} else if (m_trig_scan_mode == 2) {
		setCustomTriggerMode("scan_orbit");
		setOrbitTrig(3);
	} else {
		THROW_HW_ERROR(Error) << "Trig_scan_mode must be between 0-2";
	}
}

void Camera::getTrigScanMode(int& trig_mode) { 
	trig_mode = m_trig_scan_mode;
}

void Camera::setTrigFrameMode(int trig_mode) {
	DEB_MEMBER_FUNCT(); 
	m_trig_frame_mode = trig_mode;
	if (m_trig_frame_mode == 0) {
		setCustomTriggerMode("no_trigger");
	} else if (m_trig_frame_mode == 1) {
		setCustomTriggerMode("frame_trigger");
	} else if (m_trig_frame_mode == 2) {
		setCustomTriggerMode("frame_orbit");
		setOrbitTrig(3);
	} else {
		THROW_HW_ERROR(Error) << "Trig_frame_mode must be between 0-2";
	}
}

void Camera::getTrigFrameMode(int& trig_mode) { 
	trig_mode = m_trig_frame_mode;
}

/**
 * Executes script with the name provided as param
 *
 * 
 * @param[in] script - script file name
 */

void Camera::setXhTimingScript(string script) {
	DEB_MEMBER_FUNCT();

	if (std::find(m_xh_timing_scripts.begin(), m_xh_timing_scripts.end(), script) != m_xh_timing_scripts.end()) {
		m_xh->sendWait("~" + script);
	} else {
		THROW_HW_ERROR(Error) << "Provided script not found";
	}
}

string Camera::getTimingScript() const {
	DEB_MEMBER_FUNCT();
	string timingScript;
	m_xh->sendWait("%xstrip_timing_file", timingScript);
	return timingScript;
}
/**
 * Gets device name
 *
 * @param[out] sys_name
 */

void Camera::getSysName(string &sys_name) {
	sys_name = m_sysName;
};

/**
 * Shutdown the detector in a controlled manner
 *
 * @param[in] script The name of the shutdown command file
 */
void Camera::shutDown(string script) {
	DEB_MEMBER_FUNCT();
	m_xh->sendWait(script);
}

void Camera::coolDown() {
	DEB_MEMBER_FUNCT();
	m_xh->sendWait("~head_powerdown");
}

void Camera::powerDown() {
	DEB_MEMBER_FUNCT();
	m_xh->sendWait("~cooldown_xh");
}

void Camera::configXh(string config_name) {
	DEB_MEMBER_FUNCT();
	string config_name_to_execute = config_name.empty() ? m_configName : config_name;
	std::cout << config_name_to_execute << std::endl;
	m_xh->sendWait("~" + config_name_to_execute);
}

/**
 * Change the format of the output data. In default mode the data is returned interleaved
 * which is as how the xh heads are wired. In un-interleaved mode each head returns its data
 * separately, as a consequence of this the number of pixels is halved, but the number of heads
 * appear to give the output data a second dimension.
 *
 * @param[in] uninterleave If set to true the data is returned un-interleaved
 */
void Camera::uninterleave(bool uninterleave) {
	DEB_MEMBER_FUNCT();
	stringstream cmd1, cmd2, cmd3;
	if (m_uninterleave != uninterleave) {
		m_uninterleave = uninterleave;
		cmd1 << "close " << m_openHandle;
		m_xh->sendWait(cmd1.str());
		if (m_uninterleave) {
			cmd2 << "xstrip open " << m_sysName << " un-interleave";
			m_xh->sendWait(cmd2.str(), m_openHandle);
		} else {
			cmd2 << "xstrip open " << m_sysName;
			m_xh->sendWait(cmd2.str(), m_openHandle);
		}
		if (m_openHandle < 0) {
			THROW_HW_ERROR(Error) << "[ " << m_xh->getErrorMessage() << " ]";
		} else {
			DEB_TRACE() << "configured open path as " << m_openHandle;
		}
	}
}

void Camera::generateSoftwareTrigger() {
	DEB_MEMBER_FUNCT();
	m_software_trigger = true;
}

bool Camera::isSoftwareTriggerActive() {
	DEB_MEMBER_FUNCT();
	if (m_software_trigger) {
		m_software_trigger = false;
		return true;
	}
	return false;
}

void Camera::stopAcqThread() {
	if (m_acq_thread->hasStarted() && !m_acq_thread->hasFinished()) {
		m_quit = true;
	}
	delete m_acq_thread;
	m_acq_thread = NULL;
}
