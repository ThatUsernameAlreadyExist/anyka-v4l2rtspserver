/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** FrameBuffer.cpp
** 
**
** -------------------------------------------------------------------------*/
#include "FrameBuffer.h"
#include "logger.h"


FrameRef::FrameData::FrameData()
	: m_buffer(nullptr)
	, m_fullSize(0)
	, m_dataSize(0)
{
}


FrameRef::FrameData::~FrameData()
{
	if (m_buffer != nullptr)
	{
		free(m_buffer);
	}
}


FrameRef::FrameRef()
	: m_data(std::make_shared<FrameData>())
{
}


char* FrameRef::getData() const
{
	return m_data->m_buffer;
}


size_t FrameRef::getDataSize() const
{
	return m_data->m_dataSize;
}


void FrameRef::setDataSize(size_t size)
{
	m_data->m_dataSize = size;
}


bool FrameRef::reallocIfNeed(size_t size)
{
	if (size > m_data->m_fullSize)
	{
		m_data->m_buffer = (char*) realloc(m_data->m_buffer, size);

		if (m_data->m_buffer != nullptr)
		{
 			m_data->m_fullSize = size;
		}
		else
		{
 			m_data->m_fullSize = 0;
			m_data->m_dataSize = 0;
		}
	}

	return m_data->m_fullSize >= size;
}


bool FrameRef::isSet() const
{
	return m_data->m_dataSize > 0;
}


FrameRef FrameBuffer::getFreeFrame()
{
	FrameRef *retPtr = nullptr;

	for (auto &it : m_frames)
	{
		if (it.m_data.use_count() == 1)
		{
			it.setDataSize(0);
			retPtr = &it;
			break;
		}
	}

	if (retPtr == nullptr)
	{
		m_frames.push_back(FrameRef());
		retPtr = &m_frames.back();

		LOG(NOTICE)<<"Increase frame buffer size to "<<m_frames.size()<<"\n";
	}

	return *retPtr;
}


void FrameBuffer::clear()
{
	m_frames.clear();
}



