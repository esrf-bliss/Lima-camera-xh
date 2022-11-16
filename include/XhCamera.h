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
/*
 * XhCamera.h
 * Created on: Feb 04, 2013
 * Author: g.r.mant
 */
#ifndef XHCAMERA_H_
#define XHCAMERA_H_

#include <stdlib.h>
#include <limits>
#include <stdarg.h>
#include <strings.h>
#include "lima/HwMaxImageSizeCallback.h"
#include "lima/HwBufferMgr.h"
#include "lima/HwInterface.h"
#include "XhInterface.h"
#include <ostream>
#include "lima/Debug.h"
#include "XhClient.h"

using namespace std;

namespace lima {
	namespace Xh {

	const int xPixelSize = 1;
	const int yPixelSize = 1;

	class BufferCtrlObj;

	/*******************************************************************
	 * \class Camera
	 * \brief object controlling the Xh camera
	 *******************************************************************/
		class Camera {
			DEB_CLASS_NAMESPC(DebModCamera, "Camera", "Xh");

			public:
				struct XhStatus {
					public:
						enum XhState {
							Idle,			///< The detector is not acquiring data
							PausedAtGroup,	///< The detector has paused at the beginning of a group
							PausedAtFrame,	///< The detector has paused at the beginning of a frame
							PausedAtScan, 	///< The detector has paused at the beginning of a scan
							Running			///< The detector is acquiring data.
						};
						XhState state;
						int group_num;			///< The current group number being acquired, only valid when not {@link #Idle}
						int frame_num;			///< The current frame number, within a group, being acquired, only valid when not {@link #Idle}
						int scan_num;			///< The current scan number, within a frame, being acquired, only valid when not {@link #Idle}
						int cycle;				///< Not supported yet
						int completed_frames;	///< The number of frames completed, only valid when not {@link #Idle}
				};

				Camera(string hostname, int port, string configName, std::vector<std::string> XH_TIMING_SCRIPT);
				~Camera();

				void init();
				void reset();
				void prepareAcq();
				void startAcq();
				void stopAcq();
				void getStatus(XhStatus& status);
				int getNbHwAcquiredFrames();

				// -- detector info object
				void getImageType(ImageType& type);
				void setImageType(ImageType type);
			
				void getDetectorType(std::string& type);
				void getDetectorModel(std::string& model);
				void getDetectorImageSize(Size& size);
				void getPixelSize(double& sizex, double& sizey);

				// -- Buffer control object
				HwBufferCtrlObj* getBufferCtrlObj();

				//-- Synch control object
				void setTrigMode(TrigMode mode);
				void getTrigMode(TrigMode& mode);

				void setExpTime(double exp_time);
				void getExpTime(double& exp_time);

				void setLatTime(double lat_time);
				void getLatTime(double& lat_time);

				void getExposureTimeRange(double& min_expo, double& max_expo) const;
				void getLatTimeRange(double& min_lat, double& max_lat) const;

				void setNbFrames(int nb_frames);
				void getNbFrames(int& nb_frames);

				bool isAcqRunning() const;

				///////////////////////////
				// -- xh specific functions
				///////////////////////////

				enum HeadVoltageType {
					XhVdd, 		///>
					XhVref, 	///>
					XhVrefc, 	///>
					XhVres1, 	///>
					XhVres2, 	///>
					XhVpupref, 	///>
					XhVclamp, 	///>
					XhVled 		///>
				};

				enum ClockModeType {
					XhInternalClock,		///> internal clock - 20 ns
					XhESRF5468MHz,			///> ESRF 54.68 MHz clcok
					XhESRF1136MHz			///> ESRF Clock settings for RF div 31 = 11.3 MHz
				};

				enum TriggerOutputType {
					XhTrigOut_dc,				///> DC value from software polarity control only
					XhTrigOut_wholeGroup,		///> Asserted for full duration of enabled groups
					XhTrigOut_groupPreDel,		///> Pulse At start of Group before delay or trigger in
					XhTrigOut_groupPostDel,		///> Pulse At start of Group after delay and trigger in
					XhTrigOut_framePreDel,		///> Pulse At start of Frames before delay or trigger in
					XhTrigOut_framePostDel,		///> Pulse At start of Frames after delay and trigger in
					XhTrigOut_scanPreDel,		///> Pulse At start of Scans before delay or trigger in
					XhTrigOut_scanPostDel,		///> Pulse At start of Scans after delay and trigger in
					XhTrigOut_integration,		///> Assert During integration time
					XhTrigOut_aux1,				///> Derive from aux 1 pulse
					XhTrigOut_waitExtTrig,		///> Assert while waiting for external trigger
					XhTrigOut_waitOrbitSync		///> Assert while waiting for Orbit sync
				};

				enum TriggerControlType {
					XhTrigIn_noTrigger = 0,			///>
					XhTrigIn_groupTrigger = 0x1,	///>
					XhTrigIn_frameTrigger = 0x2,	///>
					XhTrigIn_scanTrigger = 0x4,		///>
					XhTrigIn_groupOrbit = 0x8,		///>
					XhTrigIn_frameOrbit = 0x10,		///>
					XhTrigIn_scanOrbit = 0x20,		///>
					XhTrigIn_fallingTrigger = 0x40	///>
				};

				enum BinningHorizontalType {
					XhBinnType_average=1, XhBinnType_sum=2, XhBinnType_median=3
				};

				typedef map <TriggerControlType, std::string> TriggerNamesMap;

				TriggerNamesMap triggerNames = {
					{XhTrigIn_noTrigger, "no_trigger"},
					{XhTrigIn_groupTrigger, "group_trigger"},
					{XhTrigIn_frameTrigger, "frame_trigger"},
					{XhTrigIn_scanTrigger, "scan_trigger"},
					{XhTrigIn_groupOrbit, "group_orbit"},
					{XhTrigIn_frameOrbit, "frame_orbit"},
					{XhTrigIn_scanOrbit, "scan_orbit"},
					{XhTrigIn_fallingTrigger, "falling_trigger"},
				};

				struct XhTimingParameters {
				public:
					TriggerControlType trigControl;	///> Trigger control {@see #Camera::TriggerControlType}
					int trigMux;					///> Trigger Mux select (Lemo0..7, 8=delayed orbit, 9 = Software)
					int orbitMux;					///> Orbit mux trigger select (0=direct, 1=delays 2..3 future)
					std::vector<int> lemoOut;		///> Signals for 8 Lemo outputs (binary coded 0..255 for simple use)
					bool correctRounding;			///> Adjust group & frame delay to exactly match the frame time
					int groupDelay;					///> Delay to add before group
					int frameDelay;					///> Delay to add at before each frame
					int scanPeriod;					///> Scan period (default packs as close as possible)
					int auxDelay;					///> Aux signal delay
					int auxWidth;					///> Aux Signal Pulse Width
					int longS12;					///> Make Long overlapping S1,S2&XRST for (first) group
					int frameTime;					///> Specify Frame time, calc number of scans per frame
					int shiftDown;					///> Shift down data 0..4 bits to for averaging
					int cyclesStart;				///> Start a block (sub)frames to cycle over and specify num cycles
					bool cyclesEnd;					///> End block of (sub)frames
					int s1Delay;					///> S1 fine delay in 0.25 cycles (0..3)
					int s2Delay;					///> S2 fine delay in 0.25 cycles (0..3)
					int xclkDelay;					///> XClk fine delay in 0.25 cycles (0..3)
					int rstRDelay;					///> Reset rising delay (0..3)
					int rstFDelay;					///> Reset Falling delay
					bool allowExcess;				///> Allow programming of more frame than will fit in DRAM, for manual probing
				};

				vector<string> getDebugMessages();
				void sendCommand(string cmd);
				string sendCommandString(string cmd);
				double sendCommandNumber(string cmd);
				void shutDown(string cmd);
				void _prepareAcq();
				void uninterleave(bool uninterleave);
				void set16BitReadout(bool mode);
				void setDeadPixels(int first, int num, bool reset=false);
				void setXDelay(int value);

				void setHvDac(double value, bool noslew=false, int sign=0, bool direct=false);
				void getHvAdc(double& value, bool hvmon=true, bool v12=false, bool v5=false, int sign=0, bool direct=false);
				void enableHv(bool enable, bool overtemp=false, bool force=false);
				void setCalImage(double imageScale);

				void setHeadDac(double value, HeadVoltageType voltageType, int head = -1, bool direct=false);
				void getHeadAdc(double& value, int head, HeadVoltageType voltageType);
				void setHeadCaps(int capsAB, int capsCD, int head=-1);
				void setCalEn(bool onOff, int head=-1);
				void listAvailableCaps(std::vector<int>& capValues, bool& alt_cd);
				void getAvailableTriggerModes(std::vector<std::string> &trigger_list);

				void setDefaultTimingParameters(XhTimingParameters& timingParams);
				void setTimingGroup(int groupNum, int nframes, int nscans, int intTime, bool last, const XhTimingParameters& timingParams);
				void modifyTimingGroup(int group_num, int fixed_reset=-1, bool last=false, bool allowExcess=false);
				void setTimingOrbit(int delay, bool use_falling_edge=false);
				void getTimingInfo(unsigned int* buff, int firstParam, int nParams, int firstGroup, int nGroups);
				void continueAcq();
				void setLedTiming(int pause_time, int frame_time, int int_time, bool wait_for_trig);
				void setExtTrigOutput(int trigNum, TriggerOutputType trigType, int width=0, bool invert=false);

				void setupClock(ClockModeType clockMode, int pll_gain=0, int extra_div=0, int caps=0, int r3=0, int r4=0, bool stage1=false, bool nocheck=false);

				void getSetpoint(int channel, double& value);
				void getTemperature(std::vector<double> &temperatures);

				void setOffsets(int first, int num, int value, bool direct=false);
				void syncClock();

				void setHighVoltageOn();
				void setHighVoltageOff();

				void readFrame(void* ptr, int frame_nb, int nframes);
				
				void setNbScans(int nb_scans);
				void getNbScans(int& nb_scans);
				void getTotalFrames(int& nframes);
				void getMaxFrames(string& nframes);

				void getNbGroups(int& nb_groups);
				void setNbGroups(int nb_groups);

				void setTrigMux(int trigMux);
				void getTrigMux(int& trigMux);

				void setOrbitTrig(int orbitMux);
				void getOrbitTrig(int &orbitMux);

				void setLemoOut(std::vector<int> lemoOut);
				void getLemoOut(std::vector<int>& lemoOut);

				void setCorrectRounding(bool correctRounding);
				void getCorrectRounding(bool& correctRounding);

				void setGroupDelay(int groupDelay);
				void getGroupDelay(int& groupDelay);

				void setFrameDelay(int frame);
				void getFrameDelay(int& frame);

				void setScanPeriod(int scanPeriod);
				void getScanPeriod(int& scanPeriod);

				void setAuxDelay(int auxDelay);
				void getAuxDelay(int& auxDelay) const;

				void setAuxWidth(int auxWidth);
				void getAuxWidth(int& auxWidth);

				void setFrameTime(int frameTime);
				void getFrameTime(int& frameTime);

				void setShiftDown(int shiftDown);
				void getShiftDown(int& shiftDown);

				void setCyclesStart(int cyclesStart);
				void getCyclesStart(int& cyclesStart);

				void setCyclesEnd(int cyclesEnd);
				void getCyclesEnd(int& cyclesEnd);

				void setS1Delay(int s1Delay);
				void getS1Delay(int& s1Delay);

				void setS2Delay(int s2Delay);
				void getS2Delay(int& s2Delay);

				void setXclkDelay(int xclkDelay);
				void getXclkDelay(int& xclkDelay);

				void setRstRDelay(int rstRDelay);
				void getRstRDelay(int& rstRDelay);

				void setRstFDelay(int rstFDelay);
				void getRstFDelay(int& rstFDelay);

				void setAllowExcess(bool allowExcess);
				void getAllowExcess(bool& allowExcess);

				void setCustomTriggerMode(std::string trig_mode);
				void getCustomTriggerMode(std::string& trig_mode);

				void setVoltage(double voltage);
				void getVoltage(double& voltage);

				void getBias(double& bias);

				void getCapa(double& capa);
				void setCapa(double capa);

				void setRoi(const Roi& roi_to_set);
				void getRoi(Roi& roi);
				void checkRoi(const Roi& set_roi, Roi& hw_roi);

				void setXhTimingScript(string script);

				void getSysName(string &sys_name);

				void setBin(const Bin &binning);
				void getBin(Bin &binning);
				void checkBin(Bin &binning);

				void setTrigGroupMode(int trig_mode);
				void getTrigGroupMode(int& trig_mode);

				void setTrigScanMode(int trig_mode);
				void getTrigScanMode(int& trig_mode);

				void setTrigFrameMode(int trig_mode);
				void getTrigFrameMode(int& trig_mode);

				void coolDown();
				void powerDown();
				void configXh();

				string getTimingScript() const;

			private:
				// xh specific
				XhClient *m_xh;
				string m_hostname;
				int m_port;
				string m_configName;
				string m_sysName;
				// float m_bunch_factor;

				int m_uninterleave;
				int m_npixels;
				int m_nb_groups;
				int m_openHandle;

				class AcqThread;

				AcqThread *m_acq_thread;
				TrigMode m_trigger_mode;
				double m_exp_time;
				ImageType m_image_type;
				int m_nb_frames; // nos of frames to acquire
				bool m_thread_running;
				bool m_wait_flag;
				bool m_quit;
				int m_acq_frame_nb; // nos of frames acquired
				mutable Cond m_cond;
				XhTimingParameters m_timingParams;
				int m_nb_scans;
				int m_clock_mode;
				Roi m_roi;
				Bin m_binning;

				int maxCycles = 3;
				int minCycles = 0;

				int m_voltage;
				double m_capa;

				int m_trig_group_mode;
				int m_trig_frame_mode;
				int m_trig_scan_mode;


				std::vector<std::string> m_xh_timing_scripts;
				
				// Buffer control object
				SoftBufferCtrlObj m_bufferCtrlObj;

				//utils
				std::string join(std::vector<int>::const_iterator begin, std::vector<int>::const_iterator last, const std::string &delimeter="|");

			};
	} // namespace Xh
} // namespace lima

#endif /* XHCAMERA_H_ */
