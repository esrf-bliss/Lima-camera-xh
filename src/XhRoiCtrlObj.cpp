#include <sstream>
#include "XhRoiCtrlObj.h"
#include "XhCamera.h"

using namespace lima;
using namespace lima::Xh;

RoiCtrlObj::RoiCtrlObj(Camera& cam) :
  m_cam(cam)
    {
        DEB_CONSTRUCTOR();
    }

RoiCtrlObj::~RoiCtrlObj() {
    DEB_DESTRUCTOR();
}

void RoiCtrlObj::checkRoi(const Roi& set_roi, Roi& hw_roi)
{
  DEB_MEMBER_FUNCT();
  std::cout << "SET ROI " << std::endl;
  m_cam.checkRoi(set_roi, hw_roi);
}

void RoiCtrlObj::setRoi(const Roi& roi)
{
  DEB_MEMBER_FUNCT();
  std::cout << "SET ROI" << roi.getTopLeft().x << std::endl;
  Roi real_roi;
  checkRoi(roi, real_roi);
  m_cam.setRoi(real_roi);
}

void RoiCtrlObj::getRoi(Roi& roi)
{
  DEB_MEMBER_FUNCT();
  std::cout << "GET ROI " << std::endl;
  m_cam.getRoi(roi);
}
