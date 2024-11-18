/*





*/

#pragma once

#include <stdio.h>
#include <string>
#include <vector>
#include <g40/nv2_util.h>
#include <audio/audio_u.h>


namespace nv2
{
	namespace wav
	{
		//-----------------------------------------------------------------------------
		// 
		//-----------------------------------------------------------------------------
		// do everything in 1 pass.
		static
		nv2::audio::SampleData 
		read(const std::string& filename, int blockSize = (1024 * 1024))
		{

			nv2::audio::SampleData wav_data;

			// these 3 structures make up the wave file header
			WAVE_RIFF_HEADER m_wrh{ 0 };	// 'RIFF' size 'WAVE'
			WAVE_FORMAT_HEADER m_wfx{ 0 };	// content format
			WAVE_DATA_HEADER m_wdh{ 0 };	// 'DATA' file size
			FILE* fp = nullptr;
#if _IS_WINDOWS
			errno_t err = fopen_s(&fp, filename.c_str(), "rb");
			u::throw_if(err != 0, "wav_rdr: could not open file");
#else
			fp = fopen(filename.c_str(), "rb");
			//
			u::throw_if(fp == nullptr, "wav_rdr: could not open file");
#endif
			
			// closer for file handle
			nv2::u::FILECloser fc(fp);

			do
			{
				size_t read = fread(&m_wrh, 1, sizeof(m_wrh), fp);
				if (read != sizeof(m_wrh))
					break;
				read = fread(&m_wfx, 1, sizeof(m_wfx), fp);
				if (read != sizeof(m_wfx))
					break;
				read = fread(&m_wdh, 1, sizeof(m_wdh), fp);
				if (read != sizeof(m_wdh))
					break;

				// sanity checks
				u::throw_if(m_wrh.dwRiff != RIFF_TAG, "Expecting RIFF");
				u::throw_if(m_wrh.dwWave != WAVE_TAG, "Expecting WAVE");
				u::throw_if(m_wrh.dwFormat != FMT__TAG, "Expecting fmt ");
				u::throw_if(m_wrh.dwFormatLength != sizeof(m_wfx), "Bad wave format size");
				u::throw_if(m_wdh.dwData != DATA_TAG, "Expecting data");
				// we only support 16 bit data now.
				if (m_wfx.wBitsPerSample != 16)
					break;
				//
				wav_data.blockSize = blockSize;
				wav_data.channels = m_wfx.nChannels;
				wav_data.sampleRate = m_wfx.dwSampleRate;
				wav_data.samples = m_wdh.dwDataLength / (m_wfx.wBitsPerSample / 8);
				wav_data.buffer.assign(wav_data.samples, 0);

				// float destination buffer
				float* pdf = &wav_data.buffer[0];
				// housekeeping for on the fly conversion
				size_t total = (size_t) m_wdh.dwDataLength;
				size_t chunksize = (4 * 1024);
				std::vector<unsigned char> data(chunksize,0);
				//
				for (;;)
				{
					size_t toRead = (std::min)(total, chunksize);
					// read the block
					size_t ret = fread(&data[0], 1, toRead, fp);
					if (ret == 0)
					{
						// ok. we read nothing
						break;
					}
					if (ret != toRead)
					{
						// error
						break;
					}
					// convert this chunk
					const short* pc = (const short*)&data[0];
					const short* pce = pc + (ret / sizeof(short));
					while (pc < pce)
					{
						*pdf++ = u::convert(*pc++);
					}
					//
					total -= ret;
				}
				//
			} while (0);
			//
			return wav_data;
		}

		//-----------------------------------------------------------------------------
		// interleaved raw sample data. no conversions. for debugging etc.
		struct RawData
		{
			unsigned long samples = 0;
			unsigned long channels = 0;
			unsigned long sampleRate = 0;
			// thus samples * channels in size
			std::vector<unsigned char> buffer;
		};

		//-----------------------------------------------------------------------------
		// do everything in 1 pass. debug version. 
		// return value should match size of buffer
		static
		size_t 
		read(const std::string& filename, RawData& rd)
		{
			size_t totalRead = 0;
			// these 3 structures make up the wave file header
			WAVE_RIFF_HEADER m_wrh;	// 'RIFF' size 'WAVE'
			WAVE_FORMAT_HEADER m_wfx;	// content format
			WAVE_DATA_HEADER m_wdh;	// 'DATA' file size
			FILE* fp = nullptr;
#if _IS_WINDOWS
			errno_t err = fopen_s(&fp, filename.c_str(), "rb");
			u::throw_if(err != 0, "wav_rdr: could not open file");
#else
			fp = fopen(filename.c_str(), "rb");
			//
			u::throw_if(fp == nullptr, "wav_rdr: could not open file");
#endif
			// closer for file handle
			nv2::u::FILECloser fc(fp);

			do
			{
				size_t read = fread(&m_wrh, 1, sizeof(m_wrh), fp);
				if (read != sizeof(m_wrh))
					break;
				read = fread(&m_wfx, 1, sizeof(m_wfx), fp);
				if (read != sizeof(m_wfx))
					break;
				read = fread(&m_wdh, 1, sizeof(m_wdh), fp);
				if (read != sizeof(m_wdh))
					break;
				// sanity checks
				u::throw_if(m_wrh.dwRiff != RIFF_TAG, "Expecting RIFF");
				u::throw_if(m_wrh.dwWave != WAVE_TAG, "Expecting WAVE");
				u::throw_if(m_wrh.dwFormat != FMT__TAG, "Expecting fmt ");
				u::throw_if(m_wrh.dwFormatLength != sizeof(m_wfx), "Bad wave format size");
				u::throw_if(m_wdh.dwData != DATA_TAG, "Expecting data");
				// we only support 16 bit data now.
				if (m_wfx.wBitsPerSample != 16)
					break;
				//
				//
				rd.channels = m_wfx.nChannels;
				rd.sampleRate = m_wfx.dwSampleRate;
				rd.samples = m_wdh.dwDataLength / (m_wfx.wBitsPerSample / 8);
				rd.buffer.assign(m_wdh.dwDataLength, 0);
				//
				size_t total = m_wdh.dwDataLength;
				size_t chunksize = (4 * 1024);
				unsigned char* pd = &rd.buffer[0];
				//
				for (;;)
				{
					size_t toWrite = (std::min)(total, chunksize);
					size_t ret = fread(pd, 1, toWrite, fp);
					if (ret == 0)
					{
						break;
					}
					if (ret != toWrite)
					{
						// error. throw
						break;
					}
					total -= ret;
					pd += ret;
					totalRead += ret;
				}
				//
			} while (0);
			//
			return totalRead;
		}
	}
}

