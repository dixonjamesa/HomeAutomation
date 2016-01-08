#define SUBS_RETRY_TIMEOUT 1000

struct HANWatcher
{
	float delta;
	void *watch;
	message_data data;
};

class HomeAutoNetwork
{
	private:
		RF24Network *TheNetwork = NULL;
		bool Started = false;
		HANWatcher ** WatchingItems;
		int WatchedItems = 0;
		
	public:
		HomeAutoNetwork(RF24Network *net):
		TheNetwork(net)
		{
		  RF24NetworkHeader writeHeader(0);
		  writeHeader.type = MSG_AWAKE;
		  Serial.print("Hello world"); 
		  Serial.print("...");
		  while(!TheNetwork->write(writeHeader, NULL, 0)) 
		  {
			Serial.print("."); 
			delay(SUBS_RETRY_TIMEOUT);
		  }
			Serial.print("OK.\n"); 
		}
				
		virtual void OnMessage(uint16_t from_node, message_data *_message) {}

		// Register that we want to transmit data on a channel
		// will be sent when abs(*value) changes by > delta
		void RegisterChannel(byte t, byte i, float *value, float delta, const char *c)
		{
		  HANWatcher *w;
		  HANWatcher **newwatches;
		  RF24NetworkHeader header(0);
		  Serial.print("Registering channel:");
		  header.type = MSG_REGISTER;
		  regsub(&header, t, i, c);
		  
		  WatchedItems++;
		  newwatches = new HANWatcher*[WatchedItems];
		  for(int wid=0;wid<WatchedItems;wid++) newwatches[wid] = WatchingItems[wid];
		  if( WatchedItems > 1 ) delete WatchingItems;
		  WatchingItems = newwatches;
		  w = new HANWatcher();
		  w->data.type = t;
		  w->data.code = i;
		  w->delta = delta;
		  w->watch = value;
		  memcpy(w->data.data, value, 4);
		  WatchingItems[WatchedItems-1] = w;
		}

		void SubscribeChannel(byte t, byte i, const char *c)
		{
		  RF24NetworkHeader header(0);
		  Serial.print("Subscribing to channel:");
		  header.type = MSG_SUBSCRIBE;
		  regsub(&header, t, i, c);
		}

		// Perform an update step - check for messages, and changes on registered data variables...
		void Update()
		{
			CheckForMessages();
#if false
			for( int i=0; i < WatchedItems ; i++ )
			{
				HANWatcher *item = WatchingItems[i];
				switch( item->data.type )
				{
				case DT_FLOAT:
				{
					float cachedvalue;
					float nowvalue;
					memcpy(&nowvalue, item->watch, 4);
					memcpy(&cachedvalue, item->data.data, 4);
					if( fabs(cachedvalue-nowvalue) > item->delta )
					{
						Serial.print("Value changed... transmitting...");
						RF24NetworkHeader writeHeader(0);
						writeHeader.type = MSG_DATA;
						memcpy(item->data.data, &nowvalue, 4); // copy the new value into the message data
						if(TheNetwork->write(writeHeader, &item->data, 6)) 
						{
							Serial.println("OK");
						}
						else
						{
							// copy old value back, so we try again...
							memcpy(item->data.data, &cachedvalue, 4);
							Serial.println("FAIL");
						}
					}
				}
				break;
				default:
					Serial.print("Unsupported data type registered: ");
					Serial.println(item->data.type);
				break;
				}
			}
#endif		
		}
		
		void CheckForMessages()
		{
		  while (TheNetwork->available()) 
		  {
			RF24NetworkHeader header;
			message_data message;
			TheNetwork->peek(header);
			if(header.type == MSG_UNKNOWN)
			{
			  // we could either silently try re-registering...
			  // or alert the user via a pin...
			  TheNetwork->read(header, &message, sizeof(message));
			  Serial.print("MSG_UNKNOWN RECEIVED: ");
			  Serial.println(message.code);
			}
			else if (header.type == MSG_DATA) 
			{
			  TheNetwork->read(header, &message, sizeof(message));
			  OnMessage(header.from_node, &message);
			} 
			else 
			{
			  // This is not a type we recognize
			  TheNetwork->read(header, &message, sizeof(message));
			  Serial.print("Unknown message received from node ");
			  Serial.println(header.from_node);
			}
		  }
		}
		
		int SendMessage(message_data *_message, int _datasize)
		{
			// The network header initialized for this node
			RF24NetworkHeader writeHeader(0);
			writeHeader.type = MSG_DATA;
			return TheNetwork->write(writeHeader, _message, _datasize+2);
		}
	private:
		void regsub(RF24NetworkHeader *header, byte t, byte i, const char *c)
		{
		  message_subscribe mess;
		  mess.type = t;
		  mess.code = i;
		  strcpy(mess.channel, c);
		  Serial.print((char *)mess.channel); 
		  Serial.print("...");
		  while(!TheNetwork->write(*header, &mess, sizeof(mess))) 
		  {
			Serial.print("."); 
			delay(SUBS_RETRY_TIMEOUT);
		  }
			Serial.print("OK.\n"); 
		}
};
