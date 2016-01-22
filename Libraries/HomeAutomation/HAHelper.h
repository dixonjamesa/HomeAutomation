#define WAKE_RETRY_TIMEOUT 5000
#define SUBS_RETRY_TIMEOUT 600

// Commmon status buffer
char StatusMessage[96];

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
		unsigned long updateTimer = 0;
		uint16_t NodeID; 
	public:
		HomeAutoNetwork(RF24Network *net):
		TheNetwork(net)
		{
		}
		
		void Begin(const uint16_t node_id)
		{
			NodeID = node_id;
			Serial.print("Hello world"); 
			Serial.print("...");
			sendAwake(MSG_AWAKE, node_id);
		}
		
		virtual void OnMessage(uint16_t from_node, message_data *_message) {}
		virtual void OnUnknown(uint16_t from_node, message_data *_message) {}
		virtual void OnResetNeeded() {}

		// Register that we want to transmit data on a channel
		// will be sent when abs(*value) changes by > delta
		void RegisterChannel(byte t, byte code, byte *value, float delta, const char *c, bool _restart)
		{
		  registerCommon(t, code, value, delta, 1, c, _restart);
		}
		void RegisterChannel(byte t, byte code, bool *value, float delta, const char *c, bool _restart)
		{
		  registerCommon(t, code, value, delta, 1, c, _restart);
		}
		void RegisterChannel(byte t, byte code, float *value, float delta, const char *c, bool _restart)
		{
		  registerCommon(t, code, value, delta, 4, c, _restart);
		}
		void RegisterChannel(byte t, byte code, int *value, float delta, const char *c, bool _restart)
		{
		  registerCommon(t, code, value, delta, 2, c, _restart);
		}
		void RegisterChannel(byte t, byte code, long *value, float delta, const char *c, bool _restart)
		{
		  registerCommon(t, code, value, delta, 4, c, _restart);
		}
		void RegisterChannel(byte t, byte code, char *value, const char *c, bool _restart)
		{
		  registerCommon(t, code, value, 0, strlen(value), c, _restart);
		}

		void SubscribeChannel(byte t, byte i, const char *c)
		{
		  RF24NetworkHeader header(0);
		  Serial.print("Subscribing to channel:");
		  header.type = MSG_SUBSCRIBE;
		  regsub(&header, t, i, c);
		}
		
		// Perform an update step - check for messages, and changes on registered data variables...
		void Update(unsigned long _time)
		{
			// first check for incoming messages...
			CheckForMessages();
			// now look to see if any of the variables we're watching have changed...
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
						memcpy(item->data.data, &nowvalue, 4); // copy the new value into the message data
						sendDataMessage(item, &cachedvalue, 4);
					}
				}
				break;
				case DT_BOOL:
				case DT_BYTE:
				case DT_TOGGLE:
				{
					byte cachedvalue;
					byte nowvalue;
					memcpy(&nowvalue, item->watch, 1);
					memcpy(&cachedvalue, item->data.data, 1);
					if( fabs(cachedvalue-nowvalue) > item->delta )
					{
						memcpy(item->data.data, &nowvalue, 1); // copy the new value into the message data
						sendDataMessage(item, &cachedvalue, 1);
					}
				}
				break;
				case DT_INT16:
				{
					int cachedvalue;
					int nowvalue;
					memcpy(&nowvalue, item->watch, 2);
					memcpy(&cachedvalue, item->data.data, 2);
					if( fabs(cachedvalue-nowvalue) > item->delta )
					{
						memcpy(item->data.data, &nowvalue, 2); // copy the new value into the message data
						sendDataMessage(item, &cachedvalue, 2);
					}
				}
				break;
				case DT_INT32:
				{
					long cachedvalue;
					long nowvalue;
					memcpy(&nowvalue, item->watch, 4);
					memcpy(&cachedvalue, item->data.data, 4);
					if( fabs(cachedvalue-nowvalue) > item->delta )
					{
						memcpy(item->data.data, &nowvalue, 4); // copy the new value into the message data
						sendDataMessage(item, &cachedvalue, 4);
					}
				}
				break;
				case DT_TEXT:
				{
					char *cachedvalue = item->data.data;
					char *nowvalue = (char *)item->watch;
					if( strcmp(cachedvalue, nowvalue) )
					{
						strcpy(item->data.data, nowvalue); // copy the new value into the message data
						sendDataMessage(item, &cachedvalue, strlen(nowvalue)+1);
					}
				}
				break;
				default:
					Serial.print("Unsupported data type registered: ");
					Serial.println(item->data.type);
				break;
				}
			}
			updateTimer += _time;
			if( updateTimer > 30*1000 ) // 30 secs
			{
				Serial.print("sending up check.");
				sendAwake(MSG_AWAKEACK, NodeID);
				updateTimer = 0;
			}
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
			  int sizeread = TheNetwork->read(header, &message, sizeof(message));
			  OnUnknown(header.from_node, &message);
			}
			else if( header.type == MSG_IDENTIFY)
			{
				TheNetwork->read(header, &message, sizeof(message));
				// result of a failed AWAKEACK - we need to tell the sensor to re-register its stuff
				sendAwake(MSG_AWAKE, NodeID);
				OnResetNeeded();
			}
			else if (header.type == MSG_DATA) 
			{
			  TheNetwork->read(header, &message, sizeof(message));
			  OnMessage(header.from_node, &message);
			} 
			else if (header.type == MSG_PING)
			{
				// just ignore
				TheNetwork->read(header, &message, sizeof(message));
			}
			else 
			{
			  // This is not a type we recognize
			  TheNetwork->read(header, &message, sizeof(message));
			  Serial.print("Unknown message ");
			  Serial.print(header.type);
			  Serial.print(" received from node ");
			  Serial.println(header.from_node);
			}
		  }
		}
		
		int SendMessage(message_data *_message, int _datasize)
		{
			RF24NetworkHeader writeHeader(0);
			writeHeader.type = MSG_DATA;
			return TheNetwork->write(writeHeader, _message, _datasize+2);
		}
	private:
		void registerCommon(byte t, byte code, void *value, float delta, int size, const char *c, bool _restart)
		{
		  // register with the controller anyway - it has its own redundancy checking
		  RF24NetworkHeader header(0);
		  Serial.print("Registering channel:");
		  header.type = MSG_REGISTER;
		  regsub(&header, t, code, c);
		  
		  // first check to make sure the code is unique...
		  for(int wid=0;wid<WatchedItems;wid++)
		  {
			if( WatchingItems[wid]->data.code == code )
			{
				// bail
				if( !_restart )
				{
					Serial.print("ERROR: Already registered code ");
					Serial.println( code );
				}
				return;
			}
		  }
		  if( _restart )
		  {
		   // if we're restarting, then we should already have the watcher set up...
		   // so we shouldn't get here, so let's warn the user...
			Serial.print("WARNING: Unexpected: Restart triggered, but code not already present: ");
			Serial.println(code);
		  }
		  // add the watch
		  WatchedItems++;
		  HANWatcher **newwatches = new HANWatcher*[WatchedItems];
		  for(int wid=0;wid<WatchedItems;wid++) newwatches[wid] = WatchingItems[wid];
		  if( WatchedItems > 1 ) delete WatchingItems;
		  WatchingItems = newwatches;
		  HANWatcher *w = new HANWatcher();
		  w->data.type = t;
		  w->data.code = code;
		  w->delta = delta;
		  w->watch = value;
		  memcpy(w->data.data, value, size);
		  WatchingItems[WatchedItems-1] = w;
		}
		void regsub(RF24NetworkHeader *header, byte t, byte code, const char *c)
		{
		  message_subscribe mess;
		  mess.type = t;
		  mess.code = code;
		  strcpy(mess.channel, c);
		  Serial.print((char *)mess.channel); 
		  Serial.print(" using code ");
		  Serial.print(code);
		  Serial.print("...");
		  while(!TheNetwork->write(*header, &mess, 2+1+strlen(c))) 
		  {
			Serial.print("."); 
			delay(SUBS_RETRY_TIMEOUT);
			TheNetwork->update();
		  }
			Serial.print("OK.\n"); 
		}
		
		// Send a MSG_DATA to the controller, of specified size, reverting to original value if not sent
		bool sendDataMessage(HANWatcher *w, void *originalvalue, int size)
		{
			Serial.print("Value changed on code ");
			Serial.print(w->data.code);
			Serial.print("... transmitting...");
			if(SendMessage(&w->data, size)) 
			{
				Serial.println("OK");
			}
			else
			{
				// copy old value back, so we try again...
				memcpy(w->data.data, originalvalue, size);
				Serial.println("FAIL");
			}
		}
		void sendAwake(byte _code, const uint16_t node_id)
		{
			char channel_root[32];
			sprintf(channel_root,"home/node%o",node_id);
			message_data mess;
			mess.code = 0;
			mess.type = DT_TEXT;
			strcpy(mess.data, channel_root);
			RF24NetworkHeader writeHeader(0);
			writeHeader.type = _code;
			while(!TheNetwork->write(writeHeader, &mess, 2+1+strlen(channel_root))) 
			{
				Serial.print("."); 
				delay(WAKE_RETRY_TIMEOUT);
				TheNetwork->update();
			}
			Serial.print("OK.\n"); 
		}
};
