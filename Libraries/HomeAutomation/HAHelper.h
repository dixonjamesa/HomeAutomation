#define WAKE_RETRY_TIMEOUT 5000
#define SUBS_RETRY_TIMEOUT 600

// Commmon status buffer
char StatusMessage[48];

struct HANWatcher
{
	float delta;
	void *watch;
	message_data msg;
};

struct MessageQ
{
	message_subscribe msg;
	int size;
	int type;
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
		bool awakeOK = false;
		int queueSize = 0;
		#define MAXQUEUESIZE 8
		MessageQ messageQueue[MAXQUEUESIZE];
	public:
		HomeAutoNetwork(RF24Network *net):
		TheNetwork(net)
		{
		}
		
		bool QueueEmpty()
		{
			return queueSize == 0;
		}
		
		void Begin(const uint16_t node_id)
		{
			NodeID = node_id;
			TestAwake(0);
		}
		
		// Perform an update step - check for messages, and changes on registered data variables...
		void Update(unsigned long _time)
		{
			TestAwake( _time );
			if( awakeOK && ProcessQueue() )
			{
				// first check for incoming messages...
				CheckForMessages();
				// now look to see if any of the variables we're watching have changed...
				CheckForUpdates();
			}
			else
			{
				Serial.println(F("Waiting on queue..."));
			}
		}
		
		virtual void OnMessage(uint16_t from_node, message_data *_message) {}
		virtual void OnUnknown(uint16_t from_node, message_data *_message) {}
		virtual void OnAwake(bool _success) {}
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
		  Serial.print(F("Subscribing to channel:"));
		  header.type = MSG_SUBSCRIBE;
		  regsub(&header, t, i, c);
		}
		
		// force a particular data send, even if it's not changed
		bool ForceSendRegisteredChannel(int _code)
		{
			for( int i=0; i < WatchedItems ; i++ )
			{
				HANWatcher *item = WatchingItems[i];
				if(item->msg.code == _code)
				{
					CheckWatcher(item, true);
					return true;
				}
			}
			Serial.print(F("Force send of unregistered code "));
			Serial.println(_code);
			return false;
		}
		bool ForceSendRegisteredChannel(void * _data)
		{
			for( int i=0; i < WatchedItems ; i++ )
			{
				HANWatcher *item = WatchingItems[i];
				if(item->watch == _data) // same watched pointer?
				{
					CheckWatcher(item, true);
					return true;
				}
			}
			Serial.println(F("Force send of unregistered void*"));
			return false;
		}
		
		// see if any of the variables that have been registered have changed
		// and transmit the data as required
		void CheckForUpdates()
		{
			for( int i=0; i < WatchedItems ; i++ )
			{
				HANWatcher *item = WatchingItems[i];
				if( !CheckWatcher(item, false) )
				{
					Serial.print(F("Failed to check watcher "));
					Serial.println(i);
				}
				
			}
		}			
		
		// check an individual watcher, and send data if necessary		
		bool CheckWatcher(HANWatcher *item, bool _forceSend)
		{
			switch( item->msg.type )
			{
			case DT_FLOAT:
			{
				float cachedvalue;
				float nowvalue;
				memcpy(&nowvalue, item->watch, 4);
				memcpy(&cachedvalue, item->msg.data, 4);
				if( _forceSend || fabs(cachedvalue-nowvalue) > item->delta )
				{
					Serial.print(F("Float value: "));
					Serial.print(nowvalue);
					memcpy(item->msg.data, &nowvalue, 4); // copy the new value into the message data
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
				memcpy(&cachedvalue, item->msg.data, 1);
				if( _forceSend || fabs(cachedvalue-nowvalue) > item->delta )
				{
					Serial.print(F("Byte/bool value: "));
					Serial.print(nowvalue);
					memcpy(item->msg.data, &nowvalue, 1); // copy the new value into the message data
					sendDataMessage(item, &cachedvalue, 1);
				}
			}
			break;
			case DT_INT16:
			{
				int cachedvalue;
				int nowvalue;
				memcpy(&nowvalue, item->watch, 2);
				memcpy(&cachedvalue, item->msg.data, 2);
				if( _forceSend || fabs(cachedvalue-nowvalue) > item->delta )
				{
					Serial.print(F("Int16 value: "));
					Serial.print(nowvalue);
					memcpy(item->msg.data, &nowvalue, 2); // copy the new value into the message data
					sendDataMessage(item, &cachedvalue, 2);
				}
			}
			break;
			case DT_INT32:
			{
				long cachedvalue;
				long nowvalue;
				memcpy(&nowvalue, item->watch, 4);
				memcpy(&cachedvalue, item->msg.data, 4);
				if( _forceSend || fabs(cachedvalue-nowvalue) > item->delta )
				{
					Serial.print(F("Int32 value: "));
					Serial.print(nowvalue);
					memcpy(item->msg.data, &nowvalue, 4); // copy the new value into the message data
					sendDataMessage(item, &cachedvalue, 4);
				}
			}
			break;
			case DT_TEXT:
			{
				char *cachedvalue = item->msg.data;
				char *nowvalue = (char *)item->watch;
				if( _forceSend || strcmp(cachedvalue, nowvalue) )
				{
					Serial.print(F("Text value: "));
					Serial.print(nowvalue);
					strcpy(item->msg.data, nowvalue); // copy the new value into the message data
					sendDataMessage(item, &cachedvalue, strlen(nowvalue)+1);
				}
			}
			break;
			default:
				Serial.print(F("Unsupported data type registered: "));
				Serial.println(item->msg.type);
				return false;
			}
			return true;
		}
		
		// see if there are any RF24 network messages waiting for us, 
		// and handle them as required
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
			  Serial.print(F("Unknown message "));
			  Serial.print(header.type);
			  Serial.print(F(" received from node "));
			  Serial.println(header.from_node);
			}
		  }
		}

		// Return number of bytes required to send message correctly
		int DataSizeOfMessage( message_data *_data )
		{
			switch( _data->type )
			{
				case DT_FLOAT:
				case DT_INT32:
					return 4;
				case DT_BOOL:
				case DT_BYTE:
				case DT_TOGGLE:
					return 1;
				case DT_INT16:
					return 2;
				case DT_TEXT:
					return strlen(_data->data)+1;
			}
			return 0;
		}
		
		// Send a data message
		int SendMessage(message_data *_message, int _datasize)
		{
			RF24NetworkHeader writeHeader(0);
			writeHeader.type = MSG_DATA;
			return TheNetwork->write(writeHeader, _message, _datasize+2);
		}
		
	private:
		// make sure we are registered awake...
		void TestAwake(unsigned int _time)
		{
			if( !awakeOK )
			{
				awakeOK = sendAwake(MSG_AWAKE, NodeID);
				if( awakeOK ) Serial.println(F("HANetwork awake")); else Serial.println(F("Failed to wake...")); 
				return;
			}
			// timer 
			updateTimer += _time;
			if( updateTimer > 30*1000 ) // 30 secs
			{
				Serial.print(F("Update timer now "));
				Serial.print(updateTimer);
				Serial.print(F("..."));
				if( sendAwake(MSG_AWAKEACK, NodeID) )
				{
					// reset for another 30 (otherwise keep trying on next loop)
					updateTimer = 0;
					Serial.println(F("Up check OK."));
					OnAwake(true);
				}
				else
				{
					Serial.println(F("Up check FAIL."));
					OnAwake(false);
				}
			}
		}
		
		void registerCommon(byte t, byte code, void *value, float delta, int size, const char *c, bool _restart)
		{
			if( strlen(c) >= MAXDATALENGTH )
			{
				Serial.print(F("Channel name is too long: "));
				Serial.println(c);
			}
			
		  // register with the controller anyway - it has its own redundancy checking
		  RF24NetworkHeader header(0);
		  Serial.print(F("Registering channel:"));
		  header.type = MSG_REGISTER;
		  regsub(&header, t, code, c);
		  
		  // first check to make sure the code is unique...
		  for(int wid=0;wid<WatchedItems;wid++)
		  {
			if( WatchingItems[wid]->msg.code == code )
			{
				// bail
				if( !_restart )
				{
					Serial.print(F("ERROR: Already registered code "));
					Serial.println( code );
				}
				return;
			}
		  }
		  if( _restart )
		  {
		   // if we're restarting, then we should already have the watcher set up...
		   // so we shouldn't get here, so let's warn the user...
			Serial.print(F("WARNING: Unexpected: Restart triggered, but code not already present: "));
			Serial.println(code);
		  }
		  // add the watch
		  WatchedItems++;
		  HANWatcher **newwatches = new HANWatcher*[WatchedItems];
		  for(int wid=0;wid<WatchedItems;wid++) newwatches[wid] = WatchingItems[wid];
		  if( WatchedItems > 1 ) delete WatchingItems;
		  WatchingItems = newwatches;
		  HANWatcher *w = new HANWatcher();
		  w->msg.type = t;
		  w->msg.code = code;
		  w->delta = delta;
		  w->watch = value;
		  memcpy(w->msg.data, value, size);
		  WatchingItems[WatchedItems-1] = w;
		}
		void regsub(RF24NetworkHeader *header, byte t, byte code, const char *c)
		{
		  message_subscribe mess;
		  mess.type = t;
		  mess.code = code;
		  strcpy(mess.channel, c);
		  Serial.print((char *)mess.channel); 
		  Serial.print(F(" using code "));
		  Serial.print(code);
		  Serial.print(F("..."));
		  if(!TheNetwork->write(*header, &mess, 2+1+strlen(c))) 
		  {
			Serial.print(F("FAIL. ")); 
			// add it to the queue...
			if( queueSize < MAXQUEUESIZE )
			{
				Serial.print(F("Queuing... ")); 
				messageQueue[queueSize].msg = mess;
				messageQueue[queueSize].size = 2+1+strlen(c);
				messageQueue[queueSize].type = header->type;
				queueSize++;
				Serial.println(F("Queued."));
			}
		  }
		  else
		  {
			Serial.println(F("OK.")); 
		  }
		}
		
		// try to send all queued messages
		bool ProcessQueue()
		{
			RF24NetworkHeader header(0);
			while( queueSize > 0 )
			{
				Serial.println(F("Processing queue..."));
				header.type = messageQueue[queueSize-1].type;
				if( !TheNetwork->write( header, &messageQueue[queueSize-1].msg, messageQueue[queueSize-1].size ) )
				{
					// failed, so break out of loop
					return false;
				}
				Serial.print(F("Queued message "));
				Serial.print(queueSize);
				Serial.print(F(" on channel "));
				Serial.print(messageQueue[queueSize-1].msg.channel);
				Serial.print(F(" using code "));
				Serial.print(messageQueue[queueSize-1].msg.code);
				Serial.println(F(" sent OK"));
				queueSize--;
			}
			return true;
		}
		
		// Send a MSG_DATA to the controller, of specified size, reverting to original value if not sent
		bool sendDataMessage(HANWatcher *w, void *originalvalue, int size)
		{
			Serial.print(F(" changed on code "));
			Serial.print(w->msg.code);
			Serial.print(F("... transmitting..."));
			if(SendMessage(&w->msg, size)) 
			{
				Serial.println(F("OK"));
			}
			else
			{
				// copy old value back, so we try again...
				memcpy(w->msg.data, originalvalue, size);
				Serial.println(F("FAIL"));
			}
		}
		bool sendAwake(byte _code, const uint16_t node_id)
		{
			char channel_root[32];
			sprintf(channel_root,"n%o",node_id);
			message_data mess;
			mess.code = 0;
			mess.type = DT_TEXT;
			strcpy(mess.data, channel_root);
			RF24NetworkHeader writeHeader(0);
			writeHeader.type = _code;
			return TheNetwork->write(writeHeader, &mess, 2+1+strlen(channel_root));
		}
};
