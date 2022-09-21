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
// XhInterface.h
// Created on: Feb 04, 2013
// Author: g.r.mant

#ifndef XHINTERFACE_H_
#define XHINTERFACE_H_

#include "lima/HwInterface.h"

namespace lima {
	namespace Xh {

	class Interface;
	class DetInfoCtrlObj;
    class SyncCtrlObj;
    class RoiCtrlObj;
	class BinCtrlObj;

	class Camera;


	/*******************************************************************
	 * \class Interface
	 * \brief Xh hardware interface
	 *******************************************************************/

	class Interface: public HwInterface {
		DEB_CLASS_NAMESPC(DebModCamera, "Interface", "Xh");

		public:
			Interface(Camera& cam);
			virtual ~Interface();
			virtual void getCapList(CapList&) const;
			virtual void reset(ResetLevel reset_level);
			virtual void prepareAcq();
			virtual void startAcq();
			virtual void stopAcq();
			virtual void getStatus(StatusType& status);
			virtual int getNbHwAcquiredFrames();

		private:
			Camera& m_cam;
			CapList m_cap_list;
			DetInfoCtrlObj* m_det_info;
			HwBufferCtrlObj*  m_bufferCtrlObj;
			SyncCtrlObj* m_sync;
			RoiCtrlObj* m_roi;
			BinCtrlObj* m_bin;
	};

	} // namespace Xh
} // namespace lima

#endif /* XHINTERFACE_H_ */
