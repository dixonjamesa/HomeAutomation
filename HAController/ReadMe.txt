Message Order
===================
Client (Arduino) sends MSG_AWAKE
	Controller adds sensor (if already present, all messages are unmapped)
Client sends MSG_SUBSCRIBE to be informed of changes on a specific mqtt channel
Client sends MSG_REGISTER to submit changes to a specific mqtt channel
Client regularly sends MSG_AWAKEACKs 
	If not recognised, controller returns MSG_IDENTIFY
MSG_DATA is used for sending actual data for channels
	Controller returns MSG_UNKNOWN if it can't find a mapping for the data payload





TODO
=====
More relaible registering? We are still occasionally missing these
Tidy up all sensors to use new simpler HAHelper
Fix garage door button
Fix hallway sensor
Auto-start HAController on Pi (Service?)
Remote message dump from Pi (web?)
