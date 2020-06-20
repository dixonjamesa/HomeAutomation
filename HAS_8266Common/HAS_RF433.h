/*
 * HAS_RF433.h
 *
 * listen for and assing 433MHz signals to actions
 * (C) 2019 James Dixon
 *
 */
#ifndef _RF433_H_
#define _RF433_H_

void RF433Initialise(int _pin);
void RF433AssignNextReceived(int _id); // assign next received signal to code _id
void RF433Update( int _dt );


#endif // _RF433_H_
