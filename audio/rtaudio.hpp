/*

	The PACH project: https://github.com/g40/pach

	Copyright (c) 2011-2023, Jerry Evans

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are
	met:

	1. Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

	3. The name of the author may not be used to endorse or promote products
	derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
	IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
	INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.

*/

#pragma once

#include <rtaudio/RtAudio.h>

//-----------------------------------------------------------------------------
class RtAudioEnumerator
{
public:
	//
	typedef unsigned int DeviceId;
	typedef std::map<DeviceId, RtAudio::DeviceInfo>::iterator iterator;
	typedef std::map<DeviceId, RtAudio::DeviceInfo>::const_iterator const_iterator;

private:
	//
	std::map<DeviceId, RtAudio::DeviceInfo> m_mapper;
	std::map<std::string, DeviceId> m_idMapper;
	//
	DeviceId m_ipId { 0 };
	DeviceId m_opId{ 0 };
public:
	//
	RtAudioEnumerator() {}
	// add options ... ?
	size_t enumerate()
	{
		//
		m_mapper.clear();
		m_idMapper.clear();
		//
		RtAudio rta;
		//
		std::vector<DeviceId> deviceIds = rta.getDeviceIds();
		if (deviceIds.empty()) {
			return 0;
		}
		//
		for (auto id : deviceIds)
		{
			RtAudio::DeviceInfo di = rta.getDeviceInfo(id);
			m_mapper[id] = di;
			// reverse map name to ID
			m_idMapper[di.name] = id;
			// std::cout << c::sprintf("0x%x %s %d %d %s %s\n", id, di.name.c_str(), di.inputChannels, di.outputChannels, di.isDefaultInput ? "true" : "false", di.isDefaultOutput ? "true" : "false");
			if (di.isDefaultInput)
				m_ipId = id;
			if (di.isDefaultOutput)
				m_opId = id;
		}
		//
		return m_mapper.size();
	}
	//
	iterator find(DeviceId key) { return m_mapper.find(key); }
	iterator begin() { return m_mapper.begin(); }
	iterator end() { return m_mapper.end(); }
	const_iterator find(DeviceId key) const { return m_mapper.find(key); }
	const_iterator begin() const { return m_mapper.begin();  }
	const_iterator end() const { return m_mapper.end(); }
	DeviceId ipId() const { return m_ipId; }
	DeviceId opId() const { return m_opId; }
	DeviceId key(const std::string& name)
	{
		DeviceId id = 0xFFFF;
		std::map<std::string, DeviceId>::iterator it = m_idMapper.find(name);
		if (it != m_idMapper.end())
			id = it->second;
		return id;
	}
};

//-----------------------------------------------------------------------------
// abstract interface to allow for composition
// of various processing units
template <typename T = float>
class IDuplexProcessor
{
public:
	virtual ~IDuplexProcessor() {}
	virtual int process(T* inputBuffer,
						T* outputBuffer, 	
						unsigned int samples,
						unsigned int ipChannels,
						unsigned int opChannels,
						unsigned int sampleRate,
						double streamTime, 
						RtAudioStreamStatus status) = 0;
};

//-----------------------------------------------------------------------------
// duplex device wrapper
template <typename T = float>
class RtAudioDuplex
{
	//
	bool m_mute = false;
	//
	IDuplexProcessor<T>* m_processor { nullptr };
	//
	std::unique_ptr<RtAudio> m_device;
	//
	unsigned int m_ipChannels { 0 };
	unsigned int m_opChannels{ 0 };
	// track time ?
	double m_time { 0 };
	//
	T m_level = T(1);
	// private QRT callback
	static int callback(void* outputBuffer, void* inputBuffer, unsigned int samples,
		double streamTime, RtAudioStreamStatus status, void* userdata)
	{
		RtAudioDuplex* instance = reinterpret_cast<RtAudioDuplex*>(userdata);
		return instance->process(reinterpret_cast<T*>(outputBuffer),
							reinterpret_cast<T*>(inputBuffer),
							samples,
							instance->ipChannels(),
							instance->opChannels(),
							instance->sampleRate(),
							streamTime,
							status);
	}
	// non-copyable
	RtAudioDuplex(const RtAudioDuplex&) = delete;
	RtAudioDuplex& operator=(const RtAudioDuplex&) = delete;
public:
	//
	RtAudioDuplex() {}
	//
	virtual ~RtAudioDuplex() { Close(); }
	// open a duplex device with the specified parameters
	bool Open(	RtAudioEnumerator::DeviceId ipId, 
				RtAudioEnumerator::DeviceId opId,
				IDuplexProcessor<T>* processor = nullptr,
				int ipChannels = 2,
				int opChannels = 2,
				int sampleRate = 44100, unsigned int samples = 512) 
	{ 
		if (m_device)
			return false; 
		//
		m_ipChannels = m_opChannels = 0;
		m_processor = nullptr;
		//
		m_device = std::unique_ptr<RtAudio>(new RtAudio());
		//
		RtAudio::StreamParameters ipParams;
		ipParams.deviceId = ipId;
		ipParams.nChannels = ipChannels;
		ipParams.firstChannel = 0;
		RtAudio::StreamParameters opParams;
		opParams.deviceId = opId;
		opParams.nChannels = opChannels;
		opParams.firstChannel = 0;
		// we always have interleaved data ... [L0][R0][L1][R1][...][...][Ln][Rn]
		RtAudio::StreamOptions options;
		// options.flags |= RTAUDIO_NONINTERLEAVED
		//
		void* userdata = reinterpret_cast<void*>(this);
		if (m_device->openStream(&opParams, &ipParams, RTAUDIO_FLOAT32, sampleRate, &samples, &callback, userdata, &options))
			return false;
		//
		if (m_device->isStreamOpen() == false)
			return false;			
		//
		m_ipChannels = ipChannels;
		m_opChannels = opChannels;
		m_processor = processor;
		//
		return true;
	}

	// close and clean up
	bool Close()
	{
		//
		Stop();
		//
		m_device = nullptr;
		//
		return true;
	}

	// start streaming
	bool Start() 
	{ 
		if (!m_device)
			return false;
		if (m_device->startStream())
			return false;
		return true;
	}

	// stop streaming
	bool Stop() 
	{ 
		if (!m_device)
			return false;
		if (m_device->isStreamRunning())
			m_device->stopStream();
		return true;
	}

	// close and clean up
	bool Mute(bool arg)
	{
		//
		m_mute = arg;
		//
		m_device = nullptr;
		//
		return true;
	}
	//
	bool isRunning() const
	{
		return (m_device && m_device->isStreamRunning());
	}
	//
	unsigned int sampleRate() const
	{
		if (!m_device)
			return 0;
		return m_device->getStreamSampleRate();
	}
	//
	unsigned int ipChannels() const
	{
		return m_ipChannels;
	}
	//
	unsigned int opChannels() const
	{
		return m_opChannels;
	}
	//
	virtual 
	int process(T* outputBuffer, 
		T* inputBuffer, 
		unsigned int samples,
		unsigned int ipChannels,
		unsigned int opChannels,
		unsigned int sampleRate,
		double streamTime, RtAudioStreamStatus status)
	{
		//
		int ret = 0;
		// Since the number of input and output channels is equal, we can do
		// a simple buffer copy operation here.
		if (status)
		{
			// Stream over/underflow detected.
		}
		//
		if (m_mute)
		{
			// canonical copy for now
			T* ps = inputBuffer;
			T* pd = outputBuffer;
			T* pe = ps + (samples * ipChannels);
			// T* pe = ps + (samples);
			for (T* pc = ps; pc < pe; pc++, pd++)
			{
				*pd = 0.0f;
			}
		}
		else
		{
			if (m_processor)
			{
				ret = m_processor->process(
					outputBuffer,
					inputBuffer,
					samples,
					ipChannels,
					opChannels,
					sampleRate,
					streamTime,
					status);
			}

			// mix in the incoming audio - settings.
			if (true)
			{
				// canonical copy for now
				T* ps = inputBuffer;
				T* pd = outputBuffer;
				T* pe = ps + (samples * ipChannels);
				// T* pe = ps + (samples);
				for (T* pc = ps; pc < pe; pc++, pd++)
				{
					*pd += (*pc * m_level);
					*pd /= 2.0f;
				}
			}
		}
		return ret;
	}
};

