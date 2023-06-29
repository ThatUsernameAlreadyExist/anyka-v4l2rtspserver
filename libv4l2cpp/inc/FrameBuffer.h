/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** FrameBuffer.h
** 
**
** -------------------------------------------------------------------------*/

#pragma once

#include <vector>
#include <memory>


class FrameRef
{
public:
	FrameRef();

	char* getData() const;
	size_t getDataSize() const;
	void setDataSize(size_t size);
	bool reallocIfNeed(size_t size);
	bool isSet() const;

private:
	class FrameData
	{
	public:
		FrameData();
		~FrameData();

		char *m_buffer;
		size_t m_fullSize;
		size_t m_dataSize;
	};

private:
	std::shared_ptr<FrameData> m_data;

	friend class FrameBuffer;
};


class FrameBuffer
{
public:
	FrameRef getFreeFrame();
	void clear();

private:
	std::vector<FrameRef> m_frames;

};



