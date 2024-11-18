/*





*/

#pragma once

#include <cstdio>
#include <string>
#include <vector>
//#include "audio_util.h"
#include <audio/audio_u.h>

namespace nv2
{
	namespace wav
	{
			//-----------------------------------------------------------------------------
			// do everything in 1 pass.
			template <typename T>
			bool write(const std::string& filename, const T& sd)
			{	
				bool ok = false;
				// these 3 structures make up the wave file header
				WAVE_RIFF_HEADER m_wrh;	// 'RIFF' size 'WAVE'
				WAVE_FORMAT_HEADER m_wfx;	// content format
				WAVE_DATA_HEADER m_wdh;	// 'DATA' file size
				
				// for clarity
				size_t hdr_size = sizeof(m_wrh) + sizeof(m_wfx) + sizeof(m_wdh);
				// samples is interleaved count recall
				size_t pcm_size = sd.samples * sizeof(short);

				m_wrh.dwRiff = RIFF_TAG;
				m_wrh.dwFileSize = (unsigned long) (hdr_size + pcm_size - 8);
				m_wrh.dwWave = WAVE_TAG;
				m_wrh.dwFormat = FMT__TAG;
				m_wrh.dwFormatLength = sizeof(WAVE_FORMAT_HEADER);

				m_wfx.dwSampleRate = (unsigned long)sd.sampleRate;
				m_wfx.nChannels = (unsigned short) sd.channels;
				m_wfx.wBitsPerSample = 16;
				m_wfx.wFormat = WAVE_FORMAT_PCM;
				m_wfx.wBlockAlign = (m_wfx.nChannels == 1 ? 2 : 4);	// always 4 for 16 bit samples ?
				m_wfx.dwBytesPerSec = m_wfx.nChannels * m_wfx.dwSampleRate * (m_wfx.wBitsPerSample / 8);

				m_wdh.dwData = DATA_TAG;
				m_wdh.dwDataLength = (unsigned long) pcm_size;

				FILE* fp = nullptr;
				errno_t err = fopen_s(&fp, filename.c_str(), "wb");
				if (!fp)
					return ok;
				do
				{
					size_t ret = fwrite(&m_wrh, 1, sizeof(m_wrh), fp);
					if (ret != sizeof(m_wrh))
						break;
					ret = fwrite(&m_wfx, 1, sizeof(m_wfx), fp);
					if (ret != sizeof(m_wfx))
						break;
					ret = fwrite(&m_wdh, 1, sizeof(m_wdh), fp);
					if (ret != sizeof(m_wdh))
						break;
						
					// set up source pointers for writing
					const float* psf = sd.begin();
					const float* pcf = psf;
					const float* pef = psf + sd.samples;

					// arbitrary but should match host file system
					size_t chunksize = (4 * 1024);
					// conversion buffer
					std::vector<unsigned char> bytes(chunksize, 0);
					// switch units from samples to bytes
					size_t totalBytesToWrite = (size_t)(sd.samples * sizeof(short));
					for (;;)
					{
						// write in chunks to file
						size_t toWrite = (std::min)(totalBytesToWrite, chunksize);
						// float => *short*
						short* pc = (short*) &bytes[0];
						const short* pce = pc + (toWrite / sizeof(short));
						while (pc < pce)
						{
							*pc++ = u::convert(*pcf++);
						}
						// write the converted chunk
						size_t ret = fwrite(&bytes[0], 1, toWrite, fp);
						if (ret == 0)
						{
							//
							ok = true;
							break;
						}
						if (ret != toWrite)
						{
							// error ...
							break;
						}
						totalBytesToWrite -= ret;
					}
				} while (0);
				//
				fclose(fp);
				//
				return ok;
			}
	}
}
