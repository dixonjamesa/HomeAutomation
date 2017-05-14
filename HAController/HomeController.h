#ifndef __HOMECONTROLLER_H
#define __HOMECONTROLLER_H

#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <mosquittopp.h>
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <list>
#include <string>
#include "../Libraries/HomeAutomation/HACommon.h"
#include "MessageMap.h"

extern void WriteLog(const char *str, ...);

// SensorList class
// track what sensor nodes have woken up
class SensorList
{
	private:
		struct SensorNodeData
		{
			int nodeid;
			int strikes; // keeps a count of how many sequential failed communication attempts there have been
			std::string status;
		};
		
		std::list<SensorNodeData *> NodeList;
	public:
		void AddSensor(int nodeid);
		void RemoveSensor(int nodeid);
		// see if a sensor is in the list. If it is, we reset the strike count
		bool ConfirmSensor(int nodeid);
		bool StrikeNode(int nodeid);
		void SetStatus(int nodeid, std::string _status);
		
		// go round all the sensors seeing if they respond...
		void CheckSensors();
		
		std::string DumpAll();
};

extern SensorList *MySensors;

// used to store queued messages:
struct qMessage
{
	int nodeid;
	int datasize;
	message_data message;
};

// Mosquitto class
class MyMosquitto : public mosqpp::mosquittopp 
{
	private:
	std::list<qMessage *> queuedMessages;

	public:
	MyMosquitto() : mosqpp::mosquittopp ("PiBrain") { mosqpp::lib_init(); connect("127.0.0.1"); }

	virtual void on_connect (int rc);
	virtual void on_disconnect ();
	virtual void on_message(const struct mosquitto_message* mosqmessage);
 	void ProcessQueue();
};

extern MyMosquitto *mosquittoBroker;

#endif
