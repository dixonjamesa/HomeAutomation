#ifndef __HACOMMON_H
#define __HACOMMON_H

#define MSG_REGISTER 1 // register that we will send messages on a particular channel
#define MSG_SUBSCRIBE 2 // subscribe to messages on a channel
#define MSG_DATA 3 // data message
#define MSG_PING 4 // empty message - network presence test
#define MSG_AWAKEACK 5 // confirm controller still knows about us (will receive MSG_UNKNOWN if not)
#define MSG_UNKNOWN 6 // unknown MSG_DATA received (usually due to failing to send MSG_REGISTER)
#define MSG_AWAKE 7 // sensor node started
#define MSG_IDENTIFY 8 // you sent an AWAKEACK, but I don't recognise you. I've added you, but you really need to re-subscribe now or nothing will happen

#define DT_BOOL 1
#define DT_FLOAT 2
#define DT_BYTE 3
#define DT_INT16 4
#define DT_INT32 5
#define DT_TEXT 6
#define DT_TOGGLE 7


struct message_subscribe
{
	unsigned char type; // DT_xxx
	unsigned char code; // id to send with message
	char channel[64]; // name
};
// Structure of our messages
struct message_data {
	unsigned char code;
	unsigned char type; // DT_xxx
	char data[64];
};

#endif
