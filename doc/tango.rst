Xh Tango device
===============

This is the reference documentation of the Xh Tango device.

you can also find some useful information about the camera models/prerequisite/installation/configuration/compilation in the :ref:`Xh camera plugin <camera-xh>` section.

Properties
----------
=============== =============== =============== =========================================================================
Property name	Mandatory	Default value	Description
=============== =============== =============== =========================================================================
cam_ip_address  Yes		N/A		The detector IP address
port            No              1972            The port number
config_name     No              "config"        The default configuration filename
=============== =============== =============== =========================================================================

Attributes
----------
======================= ======= ======================= ======================================================================
Attribute name		RW	Type			Description
======================= ======= ======================= ======================================================================
clockmode               wo      DevString               The clockmode, 	**XhInternalClock**, **XhESRF5468Mhz** or **XhESRF1136Mhz**
nbscans                 wr      DevLong                 the number of scans for accumulation
======================= ======= ======================= ======================================================================

Commands
--------

=======================	================ ======================= ===========================================
Command name		Arg. in		 Arg. out                Description
=======================	================ ======================= ===========================================
Init			DevVoid 	 DevVoid                 Do not use
State			DevVoid		 DevLong                 Return the device state
Status			DevVoid		 DevString               Return the device state as a string
getAttrStringValueList	DevString:	 DevVarStringArray:      Return the authorized string value list for
			Attribute name	 String value list       a given attribute name
reset                   DevVoid          DevVoid                 Perform a hardware reset of the detector
setHeadCaps             DevVarULongArray DevVoid                 Caps for AB, Caps for CD
sendCommand             DevString        DevVoid                 Backdoor command to send direct command to
                                                                 the *da.server* server
=======================	================ ======================= ===========================================

