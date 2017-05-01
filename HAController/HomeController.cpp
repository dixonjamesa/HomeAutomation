#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <mosquittopp.h>
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <list>
#include "../Libraries/HomeAutomation/HACommon.h"
#include "MessageMap.h"
#include "HomeController.h"

FILE *logfile;
char strbuffer[1024];
char tbuffer[512];
// write to logfile, if enabled...
void WriteLog(const char *str, ...)
{
	if( logfile)
	{
	    std::time_t t = time(0);   // get time now
		struct tm * now = localtime( & t );
		va_list args;
		va_start(args, str);
		fprintf(logfile, "%d-%d-%d %2d:%2d:%2d: ", now->tm_year+1900,now->tm_mon, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
		vfprintf(logfile, str, args);
		
		// and to console as well:
		printf("%d-%d-%d %2d:%2d:%2d: ", now->tm_year+1900,now->tm_mon, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);
		vprintf(str, args);
	}
}


MessageMap *MyMessageMap;
SensorList *MySensors;
MyMosquitto *mosquittoBroker;

// CE Pin, CSN Pin, SPI Speed
RF24 radio(RPI_BPLUS_GPIO_J8_15, RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);
RF24Network network(radio);

// Time between checking for packets (in ms)
const unsigned long interval = 100;
// Timer used to determine when to check nodes for still being present in network
unsigned long timespan=0; 

// add a node to our list
void SensorList::AddSensor(int nodeid)
{
	SensorNodeData *snd;
		
	for( std::list<SensorNodeData *>::const_iterator iterator = NodeList.begin(), end = NodeList.end();
			iterator != end; iterator++)
	{
		snd = *iterator;
		if(snd->nodeid == nodeid )
		{
			// node is already in the list, so we should remove all the mapped messages,
			// as we expect the node to send them again, since it's just sent it's AWAKE message
			MyMessageMap->RemoveAll(nodeid);
			WriteLog( "Adding node %o, but already present in list, so cleared all previously mapped messages\n", nodeid);
			return;
		}
	}
	// add the new node
	snd = new SensorNodeData();
	snd->nodeid = nodeid;
	snd->strikes = 0;
	NodeList.push_front(snd);
	WriteLog( "Added new sensor node %o.\n", nodeid);
}

// remove a sensor from our list
void SensorList::RemoveSensor(int nodeid)
{
	SensorNodeData *snd;

	WriteLog( "Removing sensor node %o...", snd->nodeid);

	for( std::list<SensorNodeData *>::iterator iterator = NodeList.begin(), end = NodeList.end();
			iterator != end; iterator++)
	{
		snd = *iterator;
		if(snd->nodeid == nodeid )
		{
			MyMessageMap->RemoveAll(nodeid);
			NodeList.erase(iterator);
			delete snd;
			WriteLog( " Success.\n");
			return;
		}
	}
	// guess it was never there?
	WriteLog( " Not found.\n");
}

// see if a sensor is in the list. If it is, we reset the strike count
bool SensorList::ConfirmSensor(int nodeid)
{
	SensorNodeData *snd;
	for( std::list<SensorNodeData *>::const_iterator iterator = NodeList.begin(), end = NodeList.end();
			iterator != end; iterator++)
	{
		snd = *iterator;
		if(snd->nodeid == nodeid )
		{
			WriteLog( "Sensor node %o says hi (AWAKEACK). Current strike count of %d reset to 0\n", snd->nodeid, snd->strikes);
			snd->strikes = 0;
			return true;
		}
	}
	return false;

}

// mark a strike against a node
// if it's struck out, then it gets removed from our list of nodes, and result is true
// otherwise result is false
bool SensorList::StrikeNode(int nodeid)
{
	SensorNodeData *snd;

	// once we have this many, we assume the node is gone for some reason
	int maxstrikes = 10;

	WriteLog("Sensor node %o gets a strike...", nodeid);
	
	// first find the corresponding node in our list of nodes
	for( std::list<SensorNodeData *>::iterator iterator = NodeList.begin(), end = NodeList.end();
			iterator != end; iterator++)
	{
		snd = *iterator;
		if(snd->nodeid == nodeid )
		{ // OK, this is the correct node
			// so increment the strike count, and see how many that makes:
			if( ++snd->strikes >= maxstrikes )
			{
				// looks like this node is AWOL
				WriteLog( " unresponsive - %d strikes. Removing\n", snd->strikes);
				MyMessageMap->RemoveAll(snd->nodeid);
				NodeList.erase(iterator);
				delete snd;
				return true; // struck out
			}
			// OK. Just a warning for now.
			WriteLog(" Now has %d strikes/%d\n", snd->strikes, maxstrikes);
			return false; // still has lives left
		}
	}
#if 1
	WriteLog(" Node not registered anyway.\n");
	return true; // struck out
#else
	// the node wasn't already in our list
	WriteLog(" Node not registered, sending MSG_IDENTIFY...");
	// send an IDENTIFY message to tell this node we don't know who it is
	int retries = 4;
	RF24NetworkHeader txheader(nodeid);
	txheader.type = MSG_IDENTIFY;
	while ( retries-- > 0 && !network.write(txheader, NULL, 0))
	{
		delay(50);
	}
	if( retries <= 0 )
	{
		WriteLog(" Not delivered.");
	}
	else
	{
		WriteLog(" Delivered OK.");
	}
	return false;
#endif
}

// go round all the sensors seeing if they respond...
void SensorList::CheckSensors()
{
	//WriteLog("Sensor Check...\n");
	SensorNodeData *snd;
	for( std::list<SensorNodeData *>::iterator iterator = NodeList.begin(), end = NodeList.end();
			iterator != end; /* deliberately nothing */)
	{
		snd = *iterator;
		// Send message on RF24 network
		RF24NetworkHeader header(snd->nodeid);
		header.type = MSG_PING;
		if (!network.write(header, NULL, 0) )
		{
			if( ++snd->strikes > 10 )
			{
				// looks like this node is AWOL
				WriteLog( "Sensor node %o unresponsive - 10 strikes. Removing\n", snd->nodeid);
				MyMessageMap->RemoveAll(snd->nodeid);
				iterator = NodeList.erase(iterator);
				delete snd;						
			}
			else
			{
				WriteLog( "lost sensor node %o, strike %d\n", snd->nodeid, snd->strikes);
				iterator++;
			}
		}
		else
		{
			if( snd->strikes > 0 )
			{
				WriteLog( "recovered sensor node %o\n", snd->nodeid);
				snd->strikes = 0;
			}
			iterator++;
		}
	}
}

void MyMosquitto::on_connect (int rc) { WriteLog( "Connected to Mosquitto\n"); }

void MyMosquitto::on_disconnect () { WriteLog( "Disconnected\n"); }


void MyMosquitto::on_message(const struct mosquitto_message* mosqmessage) 
{
	// Message received on a channel we subscribe to
	sprintf(strbuffer, "Message on %s: %s, ", mosqmessage->topic, (char *)mosqmessage->payload);
	
	std::list<mapitem *> matchlist;
	int matches = MyMessageMap->MatchAll(mosqmessage->topic, &matchlist, false);

	if( matches == 0 )
	{
		sprintf(tbuffer, "could not identify channel from: %s\n", mosqmessage->topic);
		strcat(strbuffer, tbuffer);
		WriteLog(strbuffer);
		return;
	}
	sprintf(tbuffer, "matches %d items; ", matches);
	strcat(strbuffer, tbuffer);
	
	for(std::list<mapitem *>::const_iterator iterator = matchlist.begin(), end = matchlist.end();
		iterator != end; iterator++)
	{
		mapitem *item = *iterator;
		sprintf(tbuffer, "sending to node %o...", item->nodeid);
		strcat(strbuffer, tbuffer);
		// Create message to send via RF24
		message_data datamessage;
		int datasize = 2; // header is 2 bytes
		switch( item->type )
		{
			case DT_BOOL:
			{
				bool state = true;
				if (!strcmp((char*)mosqmessage->payload, "OFF"))
				{
					state = false;
				}
				else if (strcmp((char*)mosqmessage->payload, "ON"))
				{	// warn if data wasn't strictly 0 or 1
					sprintf(tbuffer, "Unknown bool state: %s\n", (char *)mosqmessage->payload);
					strcat(strbuffer, tbuffer);
					WriteLog( strbuffer);
					return;
				}
				datamessage = (message_data){ item->code, DT_BOOL, state};
				datasize += 1;
			}
			break;
			case DT_BYTE:
			{
				datamessage = (message_data){ item->code, DT_BYTE, 0};
				datamessage.data[0] = *((char *)mosqmessage->payload);
				datasize += 1;
			}
			break;
			case DT_TOGGLE:
			{
				datamessage = (message_data){ item->code, DT_TOGGLE, 0};
				// no additional data to send
			}
			break;
			case DT_INT16:
			{
				int value = atoi((char*)mosqmessage->payload);
				datamessage = (message_data){ item->code, DT_INT16, 0};
				memcpy( datamessage.data, &value, 2);
				datasize += 2;
			}
			break;
			case DT_INT32:
			{
				long value = atol((char*)mosqmessage->payload);
				datamessage = (message_data){ item->code, DT_INT32, 0};
				memcpy( datamessage.data, &value, 4);
				datasize += 4;					
			}
			break;
			case DT_FLOAT:
			{
				float value = atof((char*)mosqmessage->payload);
				datamessage = (message_data){ item->code, DT_FLOAT, 0};
				memcpy( datamessage.data, &value, 4);
				datasize += 4;					
			}
			break;
			case DT_TEXT:
			{
				datamessage = (message_data){ item->code, DT_INT32, 0};
				strcpy( datamessage.data, (char*)mosqmessage->payload);
				datamessage.data[63] = 0; // ensure it's 0 terminated
				datasize += strlen(datamessage.data)+1;
			}
			break;
			default:
				sprintf(tbuffer, "Unsupported data type: %i\n", item->type);
				strcat(strbuffer, tbuffer);
				WriteLog( strbuffer);
				return;
		}
		// Send message on RF24 network
		RF24NetworkHeader header(item->nodeid);
		header.type = MSG_DATA;
		if (network.write(header, &datamessage, datasize))
		{
			strcat(strbuffer, "OK\n");
		}
		else
		{
			strcat(strbuffer, "FAIL (queued for retry)\n");
			
			// NEED TO KEEP TRYING!!
			// put a copy of the message onto a queue to try again later...
			qMessage *dcopy = new qMessage();
			dcopy->nodeid = item->nodeid;
			dcopy->datasize = datasize;
			memcpy( &dcopy->message, &datamessage, sizeof(datamessage));
			queuedMessages.push_front( dcopy );
		}
	}
	WriteLog(strbuffer);
}
void MyMosquitto::ProcessQueue()
{
	for( std::list<qMessage *>::iterator iterator = queuedMessages.begin(), end = queuedMessages.end();
			iterator != end; /*deliberately nothing*/ )
	{
		qMessage *mess = *iterator;
		RF24NetworkHeader header(mess->nodeid);
		header.type = MSG_DATA;
		if (network.write(header, &mess->message, mess->datasize))
		{
			// all good - remove the queued message
			WriteLog( "Queued message sent to node %o\n", mess->nodeid);
			iterator = queuedMessages.erase(iterator);
			delete mess;
		}
		else
		{
			// couldn't send, so just leave message in the queue...
			WriteLog( "Queue send fail to node %o\n", mess->nodeid);
			if( MySensors->StrikeNode(mess->nodeid) )
			{
				WriteLog("Node gone, removing queued message\n");
				iterator = queuedMessages.erase(iterator);
				delete mess;
			}
			else
			{
				iterator++;
			}
		}
	}
}

int main(int argc, char** argv)
{
	// Initialize all radio related modules
	radio.begin();
	radio.setRetries(11,15);
	delay(5);
	network.begin(120, 0); // we are the master, node 0
	network.txTimeout = 500;
	
	MyMessageMap = new MessageMap();
	MySensors = new SensorList();
	mosquittoBroker = new MyMosquitto();

	// set input to non-blocking:
	fcntl (0, F_SETFL, O_NONBLOCK);
	
	// Print some radio details (for debug purposes)
	radio.printDetails();

	network.update();

	mosquittoBroker->connect("127.0.0.1");
	printf("Starting log\n");
	logfile = fopen("HALog", "w");
	if (logfile)
	{
		//----- FILE EXISTS -----
		printf("Logging started\n");
		WriteLog("Logging started\n");
	}
	else
	{
		//----- FILE NOT FOUND -----
		printf("failed to start logging\n");
	}
	WriteLog( "Listening...\n");
	while (true)
	{
		// Get the latest network info
		network.update();
		mosquittoBroker->ProcessQueue();
		
		// Enter this loop if there is data available to be read,
		// and continue it as long as there is more data to read
		while ( network.available() )
		{
			*strbuffer = 0;
			//sprintf(strbuffer, "MESSAGE:");				
			RF24NetworkHeader header;
			// Have a peek at the data to see the header type
			network.peek(header);
			switch( header.type )
			{
				case MSG_AWAKE:
				{
					message_data receipt;
					network.read(header, &receipt, sizeof(receipt));
					sprintf(tbuffer, "AWAKE from node %o, channel root %s\n", header.from_node, receipt.data);
					strcat(strbuffer, tbuffer);
					// add this sensor (removes any messages stored previously for this node...)
					MySensors->AddSensor(header.from_node);
					char buffer[128];
					sprintf (buffer, "mosquitto_pub -t %s/wake -m 1", receipt.data);
							strcat(strbuffer,"PUBLISH:");
							strcat(strbuffer, buffer);
							strcat(strbuffer,"\n");
					system(buffer);
				}
				break;
				case MSG_AWAKEACK:
					message_data receipt;
					network.read(header, &receipt, sizeof(receipt));
					if( MySensors->ConfirmSensor(header.from_node) )
					{
						// all fine
					}
					else
					{
						sprintf(tbuffer, "AWAKEACK from node %o, ", header.from_node);
						strcat(strbuffer, tbuffer);
						int retries = 4;
						// not present, so tell the node...
						RF24NetworkHeader txheader(header.from_node);
						txheader.type = MSG_IDENTIFY;
						while ( retries-- > 0 && !network.write(txheader, NULL, 0))
						{
							// let's just wait until next awake is sent.
							delay(5);
						}
						if( retries <= 0 )
						{
							strcat(strbuffer,"not registered, MSG_IDENTIFY response failed\n");
						}
						else
						{
							strcat(strbuffer,"not registered, Sent MSG_IDENTIFY in reply\n");
						}
					}
				break;
				case MSG_REGISTER:
				{
					char channel[128];
					message_subscribe message;
					network.read(header, &message, sizeof(message));
					sprintf(channel, "%s", message.channel);
					mapitem *item = MyMessageMap->Match(header.from_node, message.code, true);
					if( item != NULL )
					{
						// Produces a lot of spew, due to non-guaranteed network delivery (ie messages get sent multiple times)
						//sprintf(tbuffer, "Node %o trying to re-register code %d for channel %s, but already on channel %s with code %d; IGNORED\n", header.from_node, message.code, channel, item->channel, item->code);
						//strcat(strbuffer, tbuffer);
					}
					else
					{
						MyMessageMap->AddMap(channel,header.from_node, message.type, message.code, true);
						sprintf(tbuffer, "Node %o registered channel %s with code %d\n", header.from_node, channel, message.code);
						strcat(strbuffer, tbuffer);
					}
				}
				break;
				case MSG_SUBSCRIBE:
				{
					char channel[128];
					message_subscribe message;
					network.read(header, &message, sizeof(message));
					sprintf(channel, "%s", message.channel);
					mapitem *item = MyMessageMap->Match(header.from_node, message.code, false);
					if( item != NULL )
					{
						// Produces a lot of spew, due to non-guaranteed network delivery (ie messages get sent multiple times)
						//sprintf(tbuffer, "Node %o trying to re-subscribe code %d for channel %s, but already on channel %s with code %d; IGNORED\n", header.from_node, message.code, channel, item->channel, item->code);
						//strcat(strbuffer, tbuffer);
					}
					else
					{
						MyMessageMap->AddMap(channel,header.from_node, message.type, message.code, false);
						sprintf(tbuffer, "Node %o subscribed to channel %s using code %d\n", header.from_node, channel, message.code);				
						strcat(strbuffer, tbuffer);
					}
				}
				break;
				case MSG_DATA:
				{
					message_data message;
					int datasize = network.read(header, &message, sizeof(message));
					sprintf(tbuffer, "Node %o sent code %d (%d bytes) ", header.from_node, message.code, datasize);
					strcat(strbuffer, tbuffer);
					mapitem *item = MyMessageMap->Match(header.from_node, message.code, true);
					if( item == NULL )
					{
						sprintf(tbuffer, "\nFailed to match item... sending UNKNOWN back to node %o...", header.from_node);
						strcat(strbuffer, tbuffer);
						// send MSG_UNKNOWN back to this node, with the same message data...
						RF24NetworkHeader txheader(header.from_node);
						txheader.type = MSG_UNKNOWN;
						if (network.write(txheader, &message, datasize))
						{
							strcat(strbuffer, "OK\n");
						}
						else
						{
							strcat(strbuffer, "FAIL\n");
						}
						
					}
					else
					{
						char buffer [128];
						switch(message.type)
						{
							case DT_BOOL:
							{
								bool result = message.data[0];
								sprintf(tbuffer, " = %s (BOOL %d)\n", item->channel, result);
								strcat(strbuffer, tbuffer);
								sprintf (buffer, "mosquitto_pub -t %s -m \"%s\"", item->channel, result?"ON":"OFF");
								system(buffer);
							}
							break;
							case DT_TOGGLE:
							{
								sprintf(tbuffer, " = %s (TOGGLE)\n", item->channel);
								strcat(strbuffer, tbuffer);
								sprintf (buffer, "mosquitto_pub -t %s -m \"TOGGLE\"", item->channel);
								system(buffer);
							}
							break;
							case DT_FLOAT:
							{
								float result;
								memcpy(&result, message.data, 4);
								sprintf(tbuffer, " = %s (FLOAT %f)\n", item->channel, result);
								strcat(strbuffer, tbuffer);
								sprintf (buffer, "mosquitto_pub -t %s -m \"%f\"", item->channel, result);
								system(buffer);
							}
							break;
							case DT_BYTE:
							{
								char result;
								result = *message.data;
								sprintf(tbuffer, " = %s (BYTE %d)\n", item->channel, result);
								strcat(strbuffer, tbuffer);
								sprintf (buffer, "mosquitto_pub -t %s -m \"%d\"", item->channel, result);
								system(buffer);
							}
							break;
							case DT_INT16:
							{
								short result;
								memcpy(&result, message.data, 2);
								sprintf(tbuffer, " = %s (INT16 %d)\n", item->channel, result);
								strcat(strbuffer, tbuffer);
								sprintf (buffer, "mosquitto_pub -t %s -m \"%d\"", item->channel, result);
								system(buffer);
							}
							break;
							case DT_INT32:
							{
								long result;
								memcpy(&result, message.data, 4);
								sprintf(tbuffer, " = %s (INT32 %ld)\n", item->channel, result);
								strcat(strbuffer, tbuffer);
								sprintf (buffer, "mosquitto_pub -t %s -m \"%ld\"", item->channel, result);
								system(buffer);
							}
							break;
							case DT_TEXT:
								sprintf(tbuffer, " = %s (TEXT %s)\n", item->channel, (char *)message.data);
								strcat(strbuffer, tbuffer);
								sprintf (buffer, "mosquitto_pub -t %s -m \"%s\"", item->channel, (char *)message.data);
								system(buffer);
								break;
							default:
								sprintf(tbuffer, "(unrecognised data type, %d)", message.type );
								strcat(strbuffer, tbuffer);
								break;
						}
					}
				}
				break;
				default:
				{
					// This is not a type we recognize	
					message_data message;
					network.read(header, &message, sizeof(message));
					sprintf(tbuffer, "Unknown message %d received from node %i\n", header.type,  header.from_node);				
					strcat(strbuffer, tbuffer);
				}
				break;
			}
			if( *strbuffer != 0 ) WriteLog(strbuffer);
		}

		// Check for messages on our subscribed channels
		mosquittoBroker->loop();

		delay(interval);
		if(logfile) fflush(logfile);
		
		// Periodically see if all the sensors we are aware of are still up and responding...
		timespan += interval;
		if( timespan > 60000 )
		{
			WriteLog("Checking All Sensors...\n");
			MySensors->CheckSensors();
			timespan = 0;
		}
		int v;
		read(0,&v, 1);
		if( v == 109 )
		{
			// press M then Enter...
			// So list out all the messages mapped:
			MyMessageMap->DumpAll();			
		}
	}

	fclose(logfile);

	// last thing we do before we end things
	return 0;
}
