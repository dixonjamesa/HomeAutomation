/*
 * HAS_Comms.h
 *
 * (C) 2019 James Dixon
 *
 * MQTT Pub/Sub functionality
 * Handles all the MQTT comms to the network
 *
 */
#ifndef _COMMS_H_
#define _COMMS_H_

// temp static buffer for composing payloads
#define MBUFSIZE 512
extern char messageBuffer[MBUFSIZE];

/*
 * Set up all the QTT communication topics
 */
void setupMQTTMessages();
void setupAutoDisco( bool _clear=false );

/*
 * Handle all incoming traffic
 */
void MQTTcallback(char* topic, byte* payload, unsigned int length);


#endif // _COMMS_H_
