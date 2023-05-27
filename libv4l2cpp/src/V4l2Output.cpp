/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** V4l2Output.cpp
** 
** V4L2 wrapper 
**
** -------------------------------------------------------------------------*/

#include "V4l2Output.h"

// -----------------------------------------
//    create video output interface
// -----------------------------------------
V4l2Output* V4l2Output::create(const V4L2DeviceParameters & param)
{
	return NULL;
}

// -----------------------------------------
//    constructor
// -----------------------------------------
V4l2Output::V4l2Output(V4l2Device* device) : V4l2Access(device)
{
}

// -----------------------------------------
//    destructor
// -----------------------------------------
V4l2Output::~V4l2Output() 
{
}

// -----------------------------------------
//    check writability
// -----------------------------------------
bool V4l2Output::isWritable(timeval* tv)
{
	return false;
}

// -----------------------------------------
//    write to V4l2Device
// -----------------------------------------
size_t V4l2Output::write(char* buffer, size_t bufferSize)
{
	return 0;
}


bool V4l2Output::startPartialWrite()
{
	return false;
}

size_t V4l2Output::writePartial(char* buffer, size_t bufferSize)
{
	return 0;
}

bool V4l2Output::endPartialWrite()
{
	return false;
}

