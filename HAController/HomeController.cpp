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


struct mapitem
{
	char *channel;
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
	void AddMap(const char *c, uint16_t ni, unsigned char tp, unsigned char code)
	{
		mapitem *newitem = new mapitem();
		newitem->channel = new char[strlen(c)+1];
		strcpy(newitem->channel, c);
		newitem->nodeid = ni;
		newitem->type = tp;
		newitem->code = code;
		all.push_front(newitem);
	}
	mapitem *Match(const char *channel)
	{
		for( std::list<mapitem *>::const_iterator iterator = all.begin(), end = all.end();
				iterator != end; iterator++)
		{
			mapitem *mi = *iterator;
			if(!strcmp(mi->channel, channel) )
			{
				return mi;
			}
		}
		return NULL;
	}
	mapitem *Match(uint16_t nodeid, unsigned char code)
	{
		for( std::list<mapitem *>::const_iterator iterator = all.begin(), end = all.end();
				iterator != end; iterator++)
		{
			mapitem *mi = *iterator;
			if(mi->nodeid == nodeid && mi->code == code )
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
	bool ContainsKey(const char *channel)
	{
		return Match(channel)!=NULL;
	}
	uint16_t Node(const char *channel)
	{
		mapitem *mitem = Match(channel);
		if( mitem != NULL ) return mitem->nodeid;
		return 0; // 0 is the master, and not a valid node in this circumstance
	}
};

MessageMap *MyMessageMap;

// CE Pin, CSN Pin, SPI Speed
RF24 radio(RPI_BPLUS_GPIO_J8_15, RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);
RF24Network network(radio);

// Time between checking for packets (in ms)
const unsigned long interval = 1000;

// Mosquitto class
 class MyMosquitto : public mosqpp::mosquittopp 
 {
	public:
		MyMosquitto() : mosqpp::mosquittopp ("PiBrain") { mosqpp::lib_init(); }

		virtual void on_connect (int rc) { printf("Connected to Mosquitto\n"); }

		virtual void on_disconnect () { printf("Disconnected\n"); }

		virtual void on_message(const struct mosquitto_message* mosqmessage) 
		{
			// Message received on a channel we subscribe to
			printf("Message on %s: %s, ", mosqmessage->topic, (char *)mosqmessage->payload);
			mapitem *item = MyMessageMap->Match(mosqmessage->topic);

			if( item == NULL )
			{
				printf("could not identify channel from: %s\n", mosqmessage->topic);
				return;
			}
			printf("sending to node %i...", item->nodeid);
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
						printf("Unknown bool state: %s\n", (char *)mosqmessage->payload);
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
					printf("Unsupported data type: %i\n", item->type);
					return;
			}
			// Send message on RF24 network
			RF24NetworkHeader header(item->nodeid);
			header.type = MSG_DATA;
			if (network.write(header, &datamessage, datasize))
			{
				printf("Message sent\n");
			}
			else
			{
				printf("Could not send message\n");
			}
		}
 };

 MyMosquitto mosq;

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

	mosq.connect("127.0.0.1");

	printf("Listening...\n");
	while (true)
	{
		// Get the latest network info
		network.update();
		//printf(".\n");
		
		// Enter this loop if there is data available to be read,
		// and continue it as long as there is more data to read
		while ( network.available() )
		{
			printf("MESSAGE:");				
			RF24NetworkHeader header;
			// Have a peek at the data to see the header type
			network.peek(header);
			switch( header.type )
			{
				case MSG_AWAKE:
				{
					network.read(header, NULL, 0);
					printf("AWAKE from node %o\n", header.from_node);
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
					mapitem *item = MyMessageMap->Match(header.from_node, message.code);
					if( item != NULL )
					{
						printf("Node %o trying to re-register code %d for channel %s, but already on channel %s; IGNORED", header.from_node, message.code, channel, item->channel);
					}
					else
					{
						MyMessageMap->AddMap(channel,header.from_node, message.type, message.code);
						printf("Node %o registered channel %s\n", header.from_node, channel);				
					}
				}
				break;
				case MSG_SUBSCRIBE:
				{
					char channel[128];
					message_subscribe message;
					network.read(header, &message, sizeof(message));
					sprintf(channel, "home/%s", message.channel);
					MyMessageMap->AddMap(channel,header.from_node, message.type, message.code);
					printf("Node %o subscribed to channel %s\n", header.from_node, channel);				
					mosq.subscribe(0, channel);
				}
				break;
				case MSG_DATA:
				{
					message_data message;
					int datasize = network.read(header, &message, sizeof(message));
					printf("Node %o sent code %d (%d bytes) ", header.from_node, message.code, datasize);
					mapitem *item = MyMessageMap->Match(header.from_node, message.code);
					if( item == NULL )
					{
						printf("\nFailed to match item... sending UNKNOWN back to node %o...", header.from_node);
						// send MSG_UNKNOWN back to this node, with the same message data...
						RF24NetworkHeader txheader(header.from_node);
						txheader.type = MSG_UNKNOWN;
						if (network.write(txheader, &message, datasize))
						{
							printf("OK\n");
						}
						else
						{
							printf("FAIL\n");
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
								printf(" = %s (BOOL %d)\n", item->channel, result);
								sprintf (buffer, "mosquitto_pub -t %s -m \"%d\"", item->channel, result);
								system(buffer);
							}
							break;
							case DT_FLOAT:
							{
								float result;
								memcpy(&result, message.data, 4);
								printf(" = %s (FLOAT %f)\n", item->channel, result);
								sprintf (buffer, "mosquitto_pub -t %s -m \"%f\"", item->channel, result);
								system(buffer);
							}
							break;
							case DT_BYTE:
							{
								char result;
								result = *message.data;
								printf(" = %s (BYTE %c)\n", item->channel, result);
								sprintf (buffer, "mosquitto_pub -t %s -m \"%c\"", item->channel, result);
								system(buffer);
							}
							break;
							case DT_INT16:
							{
								short result;
								memcpy(&result, message.data, 2);
								printf(" = %s (INT16 %d)\n", item->channel, result);
								sprintf (buffer, "mosquitto_pub -t %s -m \"%d\"", item->channel, result);
								system(buffer);
							}
							break;
							case DT_INT32:
							{
								long result;
								memcpy(&result, message.data, 4);
								printf(" = %s (INT32 %ld)\n", item->channel, result);
								sprintf (buffer, "mosquitto_pub -t %s -m \"%ld\"", item->channel, result);
								system(buffer);
							}
							break;
							case DT_TEXT:
								printf(" = %s (TEXT %s)\n", item->channel, (char *)message.data);
								sprintf (buffer, "mosquitto_pub -t %s -m \"%s\"", item->channel, (char *)message.data);
								system(buffer);
								break;
							default:
								printf("(unrecognised data type, %d)", message.type );
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
					printf("Unknown message %d received from node %i\n", header.type,  header.from_node);				
				}
				break;
			}
		}

		// Check for messages on our subscribed channels
		mosq.loop();

		delay(interval);
	}

	// last thing we do before we end things
	return 0;
}
