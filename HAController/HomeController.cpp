#include <RF24/RF24.h>
#include <RF24Network/RF24Network.h>
#include <mosquittopp.h>
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <list>
#include "HACommon.h"

FILE *logfile;
char strbuffer[1024];
char tbuffer[512];

struct mapitem
{
	char *channel;
	bool isreg;
	uint16_t nodeid;
	unsigned char type; // DT_xxx
	unsigned char code; // id to send with message
};

// class to handle storage and retrieval of the message maps
// Basically a very simplistic dictionary
class MessageMap
{
	std::list<mapitem *> all;

	public:
	void AddMap(const char *c, uint16_t ni, unsigned char tp, unsigned char code, bool isreg /* register or subscribe*/)
	{
		mapitem *newitem = new mapitem();
		newitem->channel = new char[strlen(c)+1];
		strcpy(newitem->channel, c);
		newitem->nodeid = ni;
		newitem->type = tp;
		newitem->code = code;
		newitem->isreg = isreg;
		all.push_front(newitem);
	}
	int MatchAll(const char *channel, std::list<mapitem *> *_list, bool isreg)
	{
		int count = 0;
		for( std::list<mapitem *>::const_iterator iterator = all.begin(), end = all.end();
				iterator != end; iterator++)
		{
			mapitem *mi = *iterator;
			if(!strcmp(mi->channel, channel) && mi->isreg == isreg)
			{
				_list->push_front(mi);
				count++;
			}
		}
		return count;

	}
	mapitem *Match(const char *channel, bool isreg)
	{
		for( std::list<mapitem *>::const_iterator iterator = all.begin(), end = all.end();
				iterator != end; iterator++)
		{
			mapitem *mi = *iterator;
			if(!strcmp(mi->channel, channel) && mi->isreg == isreg )
			{
				return mi;
			}
		}
		return NULL;
	}
	mapitem *Match(uint16_t nodeid, unsigned char code, bool isreg)
	{
		for( std::list<mapitem *>::const_iterator iterator = all.begin(), end = all.end();
				iterator != end; iterator++)
		{
			mapitem *mi = *iterator;
			if(mi->nodeid == nodeid && mi->code == code && mi->isreg == isreg )
			{
				return mi;
			}
		}
		return NULL;
	}
	void RemoveAll(uint16_t nodeid)
	{
		for( std::list<mapitem *>::iterator iterator = all.begin(), end = all.end();
				iterator != end; /*deliberately nothing*/ )
		{
			mapitem *mi = *iterator;
			if( mi->nodeid == nodeid )
			{
				iterator = all.erase(iterator);
			}
			else
			{
				iterator++;
			}
		}
	}
};

MessageMap *MyMessageMap;

// CE Pin, CSN Pin, SPI Speed
RF24 radio(RPI_BPLUS_GPIO_J8_15, RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);
RF24Network network(radio);

// Time between checking for packets (in ms)
const unsigned long interval = 1000;

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
	std::list<qMessage> queuedMessages;

	public:
	MyMosquitto() : mosqpp::mosquittopp ("PiBrain") { mosqpp::lib_init(); }

	virtual void on_connect (int rc) { fprintf(logfile, "Connected to Mosquitto\n"); }

	virtual void on_disconnect () { fprintf(logfile, "Disconnected\n"); }

	virtual void on_message(const struct mosquitto_message* mosqmessage) 
	{
		// Message received on a channel we subscribe to
		sprintf(strbuffer, "Message on %s: %s, ", mosqmessage->topic, (char *)mosqmessage->payload);
		
		std::list<mapitem *> matchlist;
		int matches = MyMessageMap->MatchAll(mosqmessage->topic, &matchlist, false);

		if( matches == 0 )
		{
			sprintf(tbuffer, "could not identify channel from: %s\n", mosqmessage->topic);
			strcat(strbuffer, tbuffer);
			return;
		}
		for(std::list<mapitem *>::const_iterator iterator = all.begin(), end = all.end();
			iterator != end; iterator++)
		{
			mapitem *item = *iterator;
			sprintf(tbuffer, "sending to node %i...", item->nodeid);
			strcat(strbuffer, tbuffer);
			// Create message to send via RF24
			message_data datamessage;
			int datasize = 2; // header is 2 bytes
			switch( item->type )
			{
				case DT_BOOL:
				{
					bool state = true;
					if (!strcmp((char*)mosqmessage->payload, "0"))
					{
						state = false;
					}
					else if (strcmp((char*)mosqmessage->payload, "1"))
					{	// warn if data wasn't strictly 0 or 1
						sprintf(tbuffer, "Unknown bool state: %s\n", (char *)mosqmessage->payload);
						strcat(strbuffer, tbuffer);
						if( logfile) fprintf(logfile, strbuffer);
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
					if( logfile) fprintf(logfile, strbuffer);
					return;
			}
			// Send message on RF24 network
			RF24NetworkHeader header(item->nodeid);
			header.type = MSG_DATA;
			if (network.write(header, &datamessage, datasize))
			{
				strcat(strbuffer, "Message sent\n");
			}
			else
			{
				strcat(strbuffer, "Could not send message\n");
				
				// NEED TO KEEP TRYING!!
				// put a copy of the message onto a queue to try again later...
				qMessage *dcopy = new qMessage();
				dcopy.nodeid = item->nodeid;
				dcopy.datasize = datasize;
				memcpy( &dcopy.message, &datamessage, sizeof(datamessage));
				queuedMessages.push_front( dcopy );
			}
		}
		if( logfile) fprintf(logfile, strbuffer);
	}
 	void ProcessQueue()
	{
		for( std::list<qMessage *>::iterator iterator = queuedMessages.begin(), end = queuedMessages.end();
				iterator != end; /*deliberately nothing*/ )
		{
			qMessage *mess = *iterator;
			RF24NetworkHeader header(mess->nodeid);
			header.type = MSG_DATA;
			if (network.write(header, &mess->message, mess->datasize))
			{
				if(logfile) fprintf(logfile, "Queued message sent\n");
				iterator = queuedMessages.erase(iterator);
			}
			else
			{
				// status quo...
			}
			iterator++;
		}
	}
};

MyMosquitto mosquittoBroker;

int main(int argc, char** argv)
{
	// Initialize all radio related modules
	radio.begin();
	delay(5);
	network.begin(90, 0); // we are the master, node 0
	
	MyMessageMap = new MessageMap();

	// Print some radio details (for debug purposes)
	radio.printDetails();

	network.update();

	mosquittoBroker.connect("127.0.0.1");
	logfile = fopen("HALog", "w");
	if (logfile)
	{
		//----- FILE EXISTS -----
		printf("Logging started");
		fprintf(logfile,"Logging started");
	}
	else
	{
		//----- FILE NOT FOUND -----
		printf("failed to start logging\n");
	}
	if(logfile) fprintf(logfile, "Listening...\n");
	while (true)
	{
		// Get the latest network info
		network.update();
		//printf(".\n");
		
		// Enter this loop if there is data available to be read,
		// and continue it as long as there is more data to read
		while ( network.available() )
		{
			sprintf(strbuffer, "MESSAGE:");				
			RF24NetworkHeader header;
			// Have a peek at the data to see the header type
			network.peek(header);
			switch( header.type )
			{
				case MSG_AWAKE:
				{
					network.read(header, NULL, 0);
					sprintf(tbuffer, "AWAKE from node %o\n", header.from_node);
					strcat(strbuffer, tbuffer);
					// remove any messages stored previously for this node...
					MyMessageMap->RemoveAll(header.from_node);
				}
				break;
				case MSG_REGISTER:
				{
					char channel[128];
					message_subscribe message;
					network.read(header, &message, sizeof(message));
					sprintf(channel, "home/%s", message.channel);
					mapitem *item = MyMessageMap->Match(header.from_node, message.code, true);
					if( item != NULL )
					{
						sprintf(tbuffer, "Node %o trying to re-register code %d for channel %s, but already on channel %s; IGNORED", header.from_node, message.code, channel, item->channel);
						strcat(strbuffer, tbuffer);
					}
					else
					{
						MyMessageMap->AddMap(channel,header.from_node, message.type, message.code, true);
						sprintf(tbuffer, "Node %o registered channel %s\n", header.from_node, channel);				
						strcat(strbuffer, tbuffer);
					}
				}
				break;
				case MSG_SUBSCRIBE:
				{
					char channel[128];
					message_subscribe message;
					network.read(header, &message, sizeof(message));
					sprintf(channel, "home/%s", message.channel);
					MyMessageMap->AddMap(channel,header.from_node, message.type, message.code, false);
					sprintf(tbuffer, "Node %o subscribed to channel %s\n", header.from_node, channel);				
					strcat(strbuffer, tbuffer);
					mosquittoBroker.subscribe(0, channel);
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
								sprintf (buffer, "mosquitto_pub -t %s -m \"%d\"", item->channel, result);
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
								sprintf(tbuffer, " = %s (BYTE %c)\n", item->channel, result);
								strcat(strbuffer, tbuffer);
								sprintf (buffer, "mosquitto_pub -t %s -m \"%c\"", item->channel, result);
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
			if( logfile) fprintf(logfile, strbuffer);
		}

		// Check for messages on our subscribed channels
		mosquittoBroker.loop();

		delay(interval);
	}

	fclose(logfile);

	// last thing we do before we end things
	return 0;
}

