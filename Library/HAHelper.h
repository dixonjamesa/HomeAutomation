#define SUBS_RETRY_TIMEOUT 1000

class HomeAutoNetwork
{
	private:
		RF24Network *TheNetwork = NULL;
		bool Started = false;
	public:
		HomeAutoNetwork(RF24Network *net):
		TheNetwork(net)
		{}
		
		virtual void OnMessage(uint16_t from_node, message_data *_message);
		
		void RegisterChannel(byte t, byte i, const char *c)
		{
		  RF24NetworkHeader header(0);
		  Serial.print("Registering channel:");
		  header.type = MSG_REGISTER;
		  regsub(&header, t, i, c);
		}

		void SubscribeChannel(byte t, byte i, const char *c)
		{
		  RF24NetworkHeader header(0);
		  Serial.print("Subscribing to channel:");
		  header.type = MSG_SUBSCRIBE;
		  regsub(&header, t, i, c);
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
