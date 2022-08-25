#ifndef XHDETINFOCTRLOBJ_H
#define XHDETINFOCTRLOBJ_H

#include "lima/HwInterface.h"

namespace lima {
	namespace Xh {

	class Camera;

	/*******************************************************************
	 * \class DetInfoCtrlObj
	 * \brief Control object providing Xh detector info interface
	 *******************************************************************/

	class DetInfoCtrlObj: public HwDetInfoCtrlObj {
        DEB_CLASS_NAMESPC(DebModCamera, "DetInfoCtrlObj","Xh");

        public:
            DetInfoCtrlObj(Camera& cam);
            virtual ~DetInfoCtrlObj();

            virtual void getMaxImageSize(Size& max_image_size);
            virtual void getDetectorImageSize(Size& det_image_size);

            virtual void getDefImageType(ImageType& def_image_type);
            virtual void getCurrImageType(ImageType& curr_image_type);
            virtual void setCurrImageType(ImageType curr_image_type);

            virtual void getPixelSize(double& x_size, double &y_size);
            virtual void getDetectorType(std::string& det_type);
            virtual void getDetectorModel(std::string& det_model);

            virtual void registerMaxImageSizeCallback(HwMaxImageSizeCallback& cb);
            virtual void unregisterMaxImageSizeCallback(HwMaxImageSizeCallback& cb);

        private:
            Camera& m_cam;
            // RoiCtrlObj* m_roi;
        };
    } // namespace Xh
} // namespace lima

#endif // XHDETINFOCTRLOBJ_H