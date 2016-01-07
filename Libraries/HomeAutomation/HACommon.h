#define MSG_REGISTER 1
#define MSG_SUBSCRIBE 2
#define MSG_DATA 3
#define MSG_PING 4
#define MSG_PINGACK 5
#define MSG_UNKNOWN 6
#define MSG_AWAKE 7

#define DT_BOOL 1
#define DT_FLOAT 2
#define DT_BYTE 3
#define DT_INT16 4
#define DT_INT32 5
#define DT_TEXT 6


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
