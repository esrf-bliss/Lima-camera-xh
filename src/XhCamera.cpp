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
#include <string>
#include <math.h>
#include <iomanip>
#include "XhCamera.h"
#include "Exceptions.h"
#include "Debug.h"

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

protected:
	virtual void threadFunction();

private:
	Camera& m_cam;
};

//---------------------------
// @brief  Ctor
//---------------------------

Camera::Camera(string hostname, int port, string configName) : m_hostname(hostname), m_port(port), m_configName(configName),
		m_sysName("'xh0'"), m_uninterleave(false), m_npixels(1024), m_image_type(Bpp32), m_nb_frames(0), m_acq_frame_nb(-1), m_bufferCtrlObj(){
	DEB_CONSTRUCTOR();

//	DebParams::setModuleFlags(DebParams::AllFlags);
//	DebParams::setTypeFlags(DebParams::AllFlags);
//	DebParams::setFormatFlags(DebParams::AllFlags);
	m_acq_thread = new AcqThread(*this);
	m_acq_thread->start();
	m_xh = new XhClient();
}

Camera::~Camera() {
	DEB_DESTRUCTOR();
	m_xh->disconnectFromServer();
	delete m_xh;
	delete m_acq_thread;
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
		cmd1 << "~" << m_configName;
		m_xh->sendWait(cmd1.str());
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
}

void Camera::reset() {
	DEB_MEMBER_FUNCT();
	m_xh->disconnectFromServer();
	init();
}

void Camera::prepareAcq() {
	DEB_MEMBER_FUNCT();
}

void Camera::startAcq() {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	m_acq_frame_nb = 0;
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

void Camera::stopAcq() {
	DEB_MEMBER_FUNCT();
	AutoMutex aLock(m_cond.mutex());
	m_wait_flag = true;
	while (m_thread_running)
		m_cond.wait();
}

void Camera::readFrame(void *bptr, int frame_nb) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	int num;
	DEB_TRACE() << "reading frame " << frame_nb;
	if (m_uninterleave) {
		cmd << "read 0 0 " << frame_nb << " " << m_npixels/2 << " 2 1 from " << m_openHandle;
	} else {
		cmd << "read 0 0 " << frame_nb << " " << m_npixels << " 1 1 from " << m_openHandle;
	}
	if (m_image_type == Bpp16) {
		num = m_npixels * sizeof(short) ;
		cmd <<  " raw";
	} else {
		num = m_npixels * sizeof(long);
		cmd << " long";
	}
	m_xh->sendWait(cmd.str());
	m_xh->getData(bptr, num);

	uint32_t *dptr;
	dptr = (uint32_t *)bptr;
	std::cout << *dptr << " " << *(dptr+1) << " " << *(dptr+2) << std::endl;
}

void Camera::getStatus(XhStatus& status) {
	DEB_MEMBER_FUNCT();
	stringstream cmd, parser;
	string str;
	unsigned pos, pos2;
	cmd << "xstrip timing read-status " << m_sysName;
	m_xh->sendWait(cmd.str(), str);
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
	AutoMutex aLock(m_cam.m_cond.mutex());
	StdBufferCbMgr& buffer_mgr = m_cam.m_bufferCtrlObj.getBuffer();

	while (!m_cam.m_quit) {
		while (m_cam.m_wait_flag && !m_cam.m_quit) {
			DEB_TRACE() << "Wait";
			m_cam.m_thread_running = false;
			m_cam.m_cond.broadcast();
			m_cam.m_cond.wait();
		}
		DEB_TRACE() << "AcqThread Running";
		m_cam.m_thread_running = true;
		if (m_cam.m_quit)
			return;

		m_cam.m_cond.broadcast();
		aLock.unlock();

		bool continueFlag = true;
		while (continueFlag && (!m_cam.m_nb_frames || m_cam.m_acq_frame_nb < m_cam.m_nb_frames)) {
			XhStatus status;
			m_cam.getStatus(status);
			if (status.state == status.Idle || status.completed_frames > m_cam.m_acq_frame_nb) {

				void* bptr = buffer_mgr.getFrameBufferPtr(m_cam.m_acq_frame_nb);
				m_cam.readFrame(bptr, m_cam.m_acq_frame_nb);
				HwFrameInfoType frame_info;
				frame_info.acq_frame_nb = m_cam.m_acq_frame_nb;
				continueFlag = buffer_mgr.newFrameReady(frame_info);
				DEB_TRACE() << "acqThread::threadFunction() newframe ready ";
				++m_cam.m_acq_frame_nb;
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
			DEB_TRACE() << "acquired " << m_cam.m_acq_frame_nb << " frames, required " << m_cam.m_nb_frames << " frames";
		}
		aLock.lock();
		m_cam.m_wait_flag = true;
	}
}

Camera::AcqThread::AcqThread(Camera& cam) :
		m_cam(cam) {
	AutoMutex aLock(m_cam.m_cond.mutex());
	m_cam.m_wait_flag = true;
	m_cam.m_quit = false;
	aLock.unlock();
	pthread_attr_setscope(&m_thread_attr, PTHREAD_SCOPE_PROCESS);
}

Camera::AcqThread::~AcqThread() {
	AutoMutex aLock(m_cam.m_cond.mutex());
	m_cam.m_quit = true;
	m_cam.m_cond.broadcast();
	aLock.unlock();
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
//	ss << "Revision " << xh_get_revision(m_sys);
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
	//	AutoMutex aLock(m_cond.mutex());
	exp_time = m_exp_time;
	DEB_RETURN() << DEB_VAR1(exp_time);
}

void Camera::setExpTime(double exp_time) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::setExpTime - " << DEB_VAR1(exp_time);

	m_exp_time = exp_time;
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

void Camera::setNbFrames(int nb_frames) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::setNbFrames - " << DEB_VAR1(nb_frames);
	if (m_nb_frames < 0) {
		THROW_HW_ERROR(Error) << "Number of frames to acquire has not been set";
	}
	m_nb_frames = nb_frames;
}

void Camera::getNbFrames(int& nb_frames) {
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "Camera::getNbFrames";
	DEB_RETURN() << DEB_VAR1(m_nb_frames);
	nb_frames = m_nb_frames;
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
void Camera::listAvailableCaps(int32_t* capValues, int& num, bool& alt_cd) {
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
	num = 0;
	while (curpos < pos2) {
		pos = capStr.find(" ", curpos);		// position of " " in capStr
		capValues[num] = atoi(capStr.substr(curpos, pos-curpos).c_str());
		curpos = pos+1;
		num++;
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
 * Setup a single timing group.
 *
 * @param[in] groupNum Group number (0..n-1)
 * @param[in] nframes Number of frames
 * @param[in] nscans Number of scans per frame (0 to calc maximum possible)
 * @param[in] intTime Integration time (20 ns cycles)
 * @param[in] last true => Last group of experiment => write all to memory
 * @param[in] trigControl Trigger control {@see #Camera::TriggerControlType}
 * @param[in] trigMux Trigger Mux select (Lemo0..7, 8=delayed orbit, 9 = Software)
 * @param[in] orbitMux Orbit mux trigger select (0=direct, 1=delays 2..3 future)
 * @param[in] lemoOut Signals for 8 Lemo outputs (binary coded 0..255 for simple use)
 * @param[in] correctRounding Adjust group & frame delay to exactly match the frame time
 * @param[in] groupDelay Delay to add before group
 * @param[in] frameDelay Delay to add at before each frame
 * @param[in] scanPeriod Scan period (default packs as close as possible)
 * @param[in] auxDelay Aux signal delay
 * @param[in] auxWidth Aux Signal Pulse Width
 * @param[in] longS12 Make Long overlapping S1,S2&XRST for (first) group
 * @param[in] frameTime Specify Frame time, calc number of scans per frame
 * @param[in] shiftDown Shift down data 0..4 bits to for averaging
 * @param[in] cyclesStart Start a block (sub)frames to cycle over and specify num cycles
 * @param[in] cyclesEnd End block of (sub)frames
 * @param[in] s1Delay S1 fine delay in 0.25 cycles (0..3)
 * @param[in] s2Delay S2 fine delay in 0.25 cycles (0..3)
 * @param[in] xclkDelay XClk fine delay in 0.25 cycles (0..3)
 * @param[in] rstRDelay Reset rising delay (0..3)
 * @param[in] rstFDelay Reset Falling delay
 * @param[in] allowExcess Allow programming of more frame than will fit in DRAM, for manual probing
 */
void Camera::setTimingGroup(int groupNum, int nframes, int nscans, int intTime, bool last,
		Camera::TriggerControlType trigControl, int trigMux, int orbitMux, int lemoOut, bool correctRounding,
		int groupDelay, int frameDelay, int scanPeriod, int auxDelay, int auxWidth, int longS12, int frameTime,
		int shiftDown, int cyclesStart, bool cyclesEnd, int s1Delay, int s2Delay, int xclkDelay, int rstRDelay,
		int rstFDelay, bool allowExcess) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip timing setup-group " << m_sysName << " " << groupNum << " " << nframes << " " << nscans << " "
			<< intTime;
	if (last)
		cmd << " last";
	switch (trigControl & (Camera::XhTrigIn_groupTrigger | Camera::XhTrigIn_frameTrigger | Camera::XhTrigIn_scanTrigger)) {
	case Camera::XhTrigIn_groupTrigger:
		cmd << " ext-trig-group";
		break;
	case Camera::XhTrigIn_groupTrigger | Camera::XhTrigIn_frameTrigger:
		cmd << " ext-trig-group ext-trig-frame";
		break;
	case Camera::XhTrigIn_groupTrigger | Camera::XhTrigIn_frameTrigger | Camera::XhTrigIn_scanTrigger:
		cmd << " ext-trig-group ext-trig-frame ext-trig-scan";
		break;
	case Camera::XhTrigIn_frameTrigger:
		cmd << " ext-trig-frame-only";
		break;
	case Camera::XhTrigIn_scanTrigger:
		cmd << " ext-trig-scan-only";
		break;
	}
	switch (trigControl & (Camera::XhTrigIn_groupOrbit | Camera::XhTrigIn_frameOrbit | Camera::XhTrigIn_scanOrbit)) {
	case Camera::XhTrigIn_groupOrbit:
		cmd << " ext-orbit-group";
		break;
	case Camera::XhTrigIn_groupOrbit | Camera::XhTrigIn_frameOrbit:
		cmd << " orbit-group orbit-frame";
		break;
	case Camera::XhTrigIn_groupOrbit | Camera::XhTrigIn_frameOrbit | Camera::XhTrigIn_scanOrbit:
		cmd << " orbit-group orbit-frame orbit-scan";
		break;
	case Camera::XhTrigIn_frameOrbit:
		cmd << " orbit-frame-only";
		break;
	case Camera::XhTrigIn_scanOrbit:
		cmd << " orbit-scan-only";
		break;
	}
	if (trigControl & Camera::XhTrigIn_fallingTrigger)
		cmd << " trig-falling";

	if (trigMux != -1)
		cmd << " trig-mux " << trigMux;
	if (orbitMux != -1)
		cmd << " orbit-mux " << orbitMux;
	if (lemoOut != 0)
		cmd << " lemo-out " << lemoOut;
	if (correctRounding)
		cmd << " correct-rounding";
	if (groupDelay != 0)
		cmd << " group-delay " << groupDelay;
	if (frameDelay != 0)
		cmd << " frame-delay " << frameDelay;
	if (scanPeriod != 0)
		cmd << " scan-period " << scanPeriod;
	if (auxDelay != 0)
		cmd << " aux-delay " << auxDelay;
	if (auxWidth != 1)
		cmd << " aux-width " << auxWidth;
	if (longS12 != 0)
		cmd << " long-s1 " << longS12;
	if (frameTime != 0)
		cmd << " frame-time " << frameTime;
	if (shiftDown != 0)
		cmd << " shift-down " << shiftDown;
	if (cyclesStart != 1)
		cmd << " cycles-start " << cyclesStart;
	if (cyclesEnd)
		cmd << " cycles-end";
	if (s1Delay != 0)
		cmd << " s1-delay " << s1Delay;
	if (s2Delay != 0)
		cmd << " s2-delay " << s2Delay;
	if (xclkDelay != 0)
		cmd << " xclk-delay " << xclkDelay;
	if (rstRDelay != 0)
		cmd << " rst-r-delay " << rstRDelay;
	if (rstFDelay != 0)
		cmd << " rst-f-delay " << rstFDelay;
	if (allowExcess)
		cmd << " allow-excess";

	int num_frames;
	m_xh->sendWait(cmd.str(), num_frames);

	if (trigControl != Camera::XhTrigIn_noTrigger) {
		setTrigMode(ExtTrigMult);
	}
	if (trigMux == 9) {
		setTrigMode(IntTrigMult);
	}
	if (last) {
		m_nb_frames = num_frames;
	}
	m_nb_groups = groupNum + 1;
	DEB_TRACE() << "m_nb_frames " << m_nb_frames;
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
 * Setup delay and polarity of Beam Orbit trigger.
 *
 * @param[in] delay Delay in 20 ns clock cycles
 * @param[in] use_falling_edge Using falling edge of orbit input
 */
void Camera::setTimingOrbit(int delay, bool use_falling_edge) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip timing setup-orbit " << m_sysName <<  " " << delay;
	if (use_falling_edge) {
		cmd << " falling";
	}
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
void Camera::getTimingInfo(uint32_t* buff, int firstParam, int nParams, int firstGroup, int nGroups) {
	DEB_MEMBER_FUNCT();
	int timingHandle;
	uint8_t* bptr = (uint8_t*) buff;
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
//	std::cout << *dptr << " " << *(dptr+1) << " " << *(dptr+2) << std::endl;
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
 * @param[in] channel 0->3=temperature sensors,
 * @param[out] value The output temperature in degrees C.
 */
void Camera::getTemperature(int channel, double& value) {
	DEB_MEMBER_FUNCT();
	stringstream cmd;
	cmd << "xstrip tc get " << m_sysName <<  " ch " << channel << " t";
	m_xh->sendWait(cmd.str(), value);
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

/**
 * Shutdown the detector in a controlled manner
 *
 * @param[in] script The name of the shutdown command file
 */
void Camera::shutDown(string script) {
	DEB_MEMBER_FUNCT();
	m_xh->sendWait(script);
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

