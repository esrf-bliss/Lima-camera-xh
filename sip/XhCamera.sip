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

namespace Xh
{
  /*******************************************************************
   * \class Camera
   * \brief object controlling the xh detector via da.server 
   *******************************************************************/
  class Camera
  {
%TypeHeaderCode
#include <XhCamera.h>
#include <string>
%End

  public:
	struct XhStatus {
	public:
		enum XhState {
			Idle, ///< The detector is not acquiringing data
			PausedAtGroup, ///< The detector has paused at the beginning of a group
			PausedAtFrame, ///< The detector has paused at the beginning of a frame
			PausedAtScan, ///< The detector has paused at the beginning of a scan
			Running ///< The detector is acquiring data.
		};
		XhState state;
		int group_num; ///< The current group number being acquired, only valid when not {@link #Idle}
		int frame_num; ///< The current frame number, within a group, being acquired, only valid when not {@link #Idle}
		int scan_num; ///< The current scan number, within a frame, being acquired, only valid when not {@link #Idle}
		int cycle; ///< Not supported yet
		int completed_frames; ///< The number of frames completed, only valid when not {@link #Idle}
	};


	Camera(std::string hostname, int port, std::string configName);
	~Camera();

	void init();
	void reset();
	void prepareAcq();
	void startAcq();
	void stopAcq();
	void getStatus(XhStatus& status /Out/);
	int getNbHwAcquiredFrames();

	// -- detector info object
	void getImageType(ImageType& type /Out/);
	void setImageType(ImageType type);

	void getDetectorType(std::string& type /Out/);
	void getDetectorModel(std::string& model /Out/);
	void getDetectorImageSize(Size& size /Out/);
	void getPixelSize(double& sizex, double& sizey /Out/);

	// -- Buffer control object
	HwBufferCtrlObj* getBufferCtrlObj();

	//-- Synch control object
	void setTrigMode(TrigMode mode);
	void getTrigMode(TrigMode& mode /Out/);

	void setExpTime(double exp_time);
	void getExpTime(double& exp_time /Out/);

	void setLatTime(double lat_time);
	void getLatTime(double& lat_time /Out/);
        
	void getExposureTimeRange(double& min_expo /Out/, double& max_expo /Out/) const;
	void getLatTimeRange(double& min_lat /Out/, double& max_lat /Out/) const;

	void setNbFrames(int nb_frames);
	void getNbFrames(int& nb_frames /Out/);

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
		XhInternalClock,		///> internal clock
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
		XhTrigIn_groupTrigger = 0x1,		///>
		XhTrigIn_frameTrigger = 0x2,		///>
		XhTrigIn_scanTrigger = 0x4,		///>
		XhTrigIn_groupOrbit = 0x8,		///>
		XhTrigIn_frameOrbit = 0x10,		///>
		XhTrigIn_scanOrbit = 0x20,		///>
		XhTrigIn_fallingTrigger = 0x40		///>
	};

	struct XhTimingParameters {
	public:
		Xh::Camera::TriggerControlType trigControl;	///> Trigger control {@see #Camera::TriggerControlType}
		int trigMux;					///> Trigger Mux select (Lemo0..7, 8=delayed orbit, 9 = Software)
		int orbitMux;					///> Orbit mux trigger select (0=direct, 1=delays 2..3 future)
		int lemoOut;					///> Signals for 8 Lemo outputs (binary coded 0..255 for simple use)
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

	//vector<std::string> getDebugMessages();
	void sendCommand(std::string cmd);
	void shutDown(std::string cmd);
	void uninterleave(bool uninterleave);
	void set16BitReadout(bool mode);
	void setDeadPixels(int first, int num, bool reset=false);
	void setXDelay(int value);

	void setHvDac(double value, bool noslew=false, int sign=0, bool direct=false);
	void getHvAdc(double& value, bool hvmon=true, bool v12=false, bool v5=false, int sign=0, bool direct=false);
	void enableHv(bool enable, bool overtemp=false, bool force=false);
	void setCalImage(double imageScale);

	void setHeadDac(double value, HeadVoltageType voltageType, int head = -1, bool direct=false);
	void getHeadAdc(double& value /Out/, int head, HeadVoltageType voltageType);
	void setHeadCaps(int capsAB, int capsCD, int head=-1);
	void setCalEn(bool onOff, int head=-1);
	//mcd
	void listAvailableCaps(int* capValues /Out/, int& num /Out/, bool& alt_cd /Out/);
	
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
	void getTemperature(int channel, double& value /Out/);

	void setOffsets(int first, int num, int value, bool direct=false);
	void syncClock();

	void readFrame(void* ptr, int frame_nb, int nframes);

	void setTrigerMux(int trigMux);
	void getTrigMux(int& trigMux);

	void setOrbitTrig(int orbitMux);
	void getOrbitTrig(int &orbitMux);

	void setNbScans(int nb_scans);
	void getNbScans(int& nb_scans /Out/);
        void getMaxFrames(std::string& nframes /Out/);
	
  private:
	Camera(const Xh::Camera&);
  };

};
