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
#define MSG_STATUS 9 // send a status message to the server (which will be logged)
#define MSG_RESEND 10 // re-send all your channel REGISTER and SUBSCRIBE settings please

// guaranteed versions:
#define MSGG_REGISTER 66
#define MSGG_SUBSCRIBE 67
#define MSGG_DATA 68 
#define MSGG_PING 69
#define MSGG_AWAKEACK 70
#define MSGG_UNKNOWN 71
#define MSGG_AWAKE 72
#define MSGG_IDENTIFY 73
#define MSGG_STATUS 74
#define MSGG_RESEND 75 


// Typical message order:
// Client sends AWAKE
//	- Server registers new node, deleting any previous information if there is a duplicate
// Client sends REGISTER and/or SUBSCRIBE to inform server what it is going to pass or what it wants
// Client sends DATA messages when REGISTERed values change
// Server sends DATA messages when SUBSCRIBEd values change
// At regular (30s?) intervals, client sends AWAKEACK
//	- If server doesn't recognise client (eg it's rebooted), it sends IDENTIFY
//	- Client should then re-send an AWAKE and all its REGISTER and SUBSCRIBE
// If server receives a DATA message for an unREGISTERed channel, it will send back an UNKNOWN
// Every so often (60secs) server sends a PING to all known clients. If the message isn't delivered 10 times, the client is forgotten
// Client sends STATUS to server to record its current status


// different data types within a MSG_DATA payload:
#define DT_BOOL 1
#define DT_FLOAT 2
#define DT_BYTE 3
#define DT_INT16 4
#define DT_INT32 5
#define DT_TEXT 6
#define DT_TOGGLE 7

#define MAXDATALENGTH 24

struct message_subscribe
{
	unsigned char type; // DT_xxx
	unsigned char code; // id to send with message
	char channel[MAXDATALENGTH]; // name
};
// Structure of our messages
struct message_data {
	unsigned char code;
	unsigned char type; // DT_xxx
	char data[MAXDATALENGTH];
};

#endif
