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
/*
 * XhDetInfoCtrlObj.cpp
 *
 *  Created on: Feb 04, 2013
 *      Author: gm
 */

#include <cstdlib>
#include "XhDetInfoCtrlObj.h"
#include "XhCamera.h"

using namespace lima;
using namespace lima::Xh;

DetInfoCtrlObj::DetInfoCtrlObj(Camera& cam) :
		m_cam(cam) {
	DEB_CONSTRUCTOR();
}

DetInfoCtrlObj::~DetInfoCtrlObj() {
	DEB_DESTRUCTOR();
}

void DetInfoCtrlObj::getMaxImageSize(Size& size) {
	DEB_MEMBER_FUNCT();
	m_cam.getDetectorImageSize(size);
}

void DetInfoCtrlObj::getDetectorImageSize(Size& image_size) {
	DEB_MEMBER_FUNCT();
	getMaxImageSize(image_size);
}

void DetInfoCtrlObj::getDefImageType(ImageType& image_type) {
	DEB_MEMBER_FUNCT();
	m_cam.getImageType(image_type);
}

void DetInfoCtrlObj::getCurrImageType(ImageType& image_type) {
	DEB_MEMBER_FUNCT();
	m_cam.getImageType(image_type);
}

void DetInfoCtrlObj::setCurrImageType(ImageType image_type) {
	DEB_MEMBER_FUNCT();
	m_cam.setImageType(image_type);
}

void DetInfoCtrlObj::getPixelSize(double& xsize, double& ysize) {
	DEB_MEMBER_FUNCT();
	m_cam.getPixelSize(xsize, ysize);
}

void DetInfoCtrlObj::getDetectorType(std::string& type) {
	DEB_MEMBER_FUNCT();
	m_cam.getDetectorType(type);
}

void DetInfoCtrlObj::getDetectorModel(std::string& model) {
	DEB_MEMBER_FUNCT();
	m_cam.getDetectorModel(model);
}

void DetInfoCtrlObj::registerMaxImageSizeCallback(HwMaxImageSizeCallback& cb) {
	DEB_MEMBER_FUNCT();
//	m_cam->registerMaxImageSizeCallback(cb);
}

void DetInfoCtrlObj::unregisterMaxImageSizeCallback(HwMaxImageSizeCallback& cb) {
	DEB_MEMBER_FUNCT();
//	m_cam->unregisterMaxImageSizeCallback(cb);
}

