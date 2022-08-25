#ifndef XHSYNCCTRLOBJ_H
#define XHSYNCCTRLOBJ_H

#include "lima/HwInterface.h"

namespace lima {
	namespace Xh {

	class Camera;

        class SyncCtrlObj: public HwSyncCtrlObj {
            DEB_CLASS_NAMESPC(DebModCamera,"SyncCtrlObj","Xh");

            public:
                SyncCtrlObj(Camera& cam); //, BufferCtrlObj& buffer);
                virtual ~SyncCtrlObj();

                virtual bool checkTrigMode(TrigMode trig_mode);
                virtual void setTrigMode(TrigMode trig_mode);
                virtual void getTrigMode(TrigMode& trig_mode);

                virtual void setExpTime(double exp_time);
                virtual void getExpTime(double& exp_time);

                virtual void setLatTime(double lat_time);
                virtual void getLatTime(double& lat_time);

                virtual void setNbHwFrames(int nb_frames);
                virtual void getNbHwFrames(int& nb_frames);

                virtual void getValidRanges(ValidRangesType& valid_ranges);

            private:
                Camera& m_cam;
        };
    } // namespace Xh
} // namespace lima

#endif // XHSYNCCTRLOBJ_H