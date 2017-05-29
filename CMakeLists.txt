# CMakeLists.txt for Xh camera

SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--add-needed")


SET(xh_srcs src/XhCamera.cpp  src/XhInterface.cpp
	 src/XhDetInfoCtrlObj.cpp src/XhSyncCtrlObj.cpp
	 src/XhClient.cpp)
	 
FILE(STRINGS "VERSION" xh_vers)
ADD_LIBRARY(limaxh SHARED ${xh_srcs})

TARGET_INCLUDE_DIRECTORIES(limaxh PUBLIC "${CMAKE_SOUURCE_DIR}/camera/common/espia/include")
TARGET_INCLUDE_DIRECTORIES(limaxh PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

TARGET_LINK_LIBRARIES(limaxh limacore)

SET_TARGET_PROPERTIES(limaxh PROPERTIES VERSION ${xh_vers} SOVERSION "1")

INSTALL(TARGETS limaxh LIBRARY DESTINATION lib)

IF(COMPILE_SIP)
    SET(NAME "xh")
    SET(INCLUDES)
    FILE(GLOB sipfiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/sip" "${CMAKE_CURRENT_SOURCE_DIR}/sip/*.sip")
    FOREACH(sipfile ${sipfiles})
        SET(INCLUDES 
        "${INCLUDES} 
        %Include ${sipfile}")
    ENDFOREACH()

    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/sip/limamodules.sip.in sip/limaxh.sip)
    SET(SIP_CONCAT_PARTS 1)
    SET(SIP_INCLUDES ${SIP_INCLUDES}
        "${CMAKE_SOURCE_DIR}/third-party/Processlib/sip"
        "${CMAKE_BINARY_DIR}/sip/core"
        "${CMAKE_SOURCE_DIR}/third-party/Processlib/tasks/sip"
        "${CMAKE_SOURCE_DIR}/common/sip"
        "${CMAKE_SOURCE_DIR}/hardware/sip"
        "${CMAKE_SOURCE_DIR}/control/sip"
        "${CMAKE_SOURCE_DIR}/control/software_operation/sip"
        "${CMAKE_CURRENT_SOURCE_DIR}/sip")
    set(SIP_DISABLE_FEATURES WITH_CONFIG)
    set(SIP_CHECK_EXC ON)
    ADD_SIP_PYTHON_MODULE(limaxh ${CMAKE_CURRENT_BINARY_DIR}/sip/limaxh.sip)
    TARGET_INCLUDE_DIRECTORIES(python_module_limaxh PRIVATE
                               ${PYTHON_INCLUDE_DIRS}
                               "${CMAKE_SOURCE_DIR}/sip"
                               "${CMAKE_SOURCE_DIR}/sip/core")
    TARGET_LINK_LIBRARIES(python_module_limaxh limaxh)

    INSTALL(FILES python/__init__.py DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/Lima/Xh")
ENDIF()
