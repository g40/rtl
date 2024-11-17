/*





*/

#pragma once

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

namespace nv2
{
	//-----------------------------------------------------------------------------
	namespace wav
	{
		//-----------------------------------------------------------------------------
		// static const int WAVE_FORMAT_PCM = 1;
		// documented in DDK MSVAD sample
		// wave file header.
		static const uint32_t RIFF_TAG = 0x46464952;
		static const uint32_t WAVE_TAG = 0x45564157;
		static const uint32_t FMT__TAG = 0x20746D66;
		static const uint32_t DATA_TAG = 0x61746164;
			
		//-----------------------------------------------------------------------------
		#define WAVE_FORMAT_PCM 1

		//-----------------------------------------------------------------------------
#pragma pack(push,1)

		//-----------------------------------------------------------------------------
		// documented in DDK MSVAD sample
		// wave file header.
		typedef struct _WAVE_RIFF_HEADER
		{
			uint32_t dwRiff;			// 'RIFF'
			uint32_t dwFileSize;		// total file size excluding initial 8 bytes
			uint32_t dwWave;			// 'WAVE'
			uint32_t dwFormat;			// 'fmt ' 
			uint32_t dwFormatLength;	// size of format 
											// to follow. i.e.
											// WAVE_FORMAT_HEADER (0x10)
		} WAVE_RIFF_HEADER;

		// there is also an extensible header which is larger
		// and contains some flags for multi-channel (5.1/7.1)
		// speaker assignments. So check dwFormatLength above
		// to make sure it matches sizeof(WAVE_FORMAT_HEADER)
		// https://learn.microsoft.com/en-gb/windows/win32/api/mmreg/ns-mmreg-waveformatextensible
		typedef struct WAVEFORMATHEADER_TAG
		{
			uint16_t wFormat;		// WAVE_FORMAT_PCM
			uint16_t nChannels;	// 1 for mono, 2 for stereo
			uint32_t dwSampleRate;	// 22050,32000,44100,48000
			uint32_t dwBytesPerSec;	// 88200 for 22K05
			uint16_t wBlockAlign;	// always 4 ?
			uint16_t wBitsPerSample;	// 16
		} WAVE_FORMAT_HEADER;

		typedef struct _WAVE_DATA_HEADER
		{
			uint32_t dwData;		// 'data' | DATA_TAG
			uint32_t dwDataLength;	// number of bytes in PCM stream
		} WAVE_DATA_HEADER;

#pragma pack(pop)
	}
		
	namespace u
	{
		//-----------------------------------------------------------------------------
		inline void throw_if(bool failed, const std::string& msg)
		{
			if (failed)
				throw std::runtime_error(msg.c_str());
		}

		//-----------------------------------------------------------------------------
		static constexpr float ratio = (1.0f / 32767.0f);
		//-----------------------------------------------------------------------------
		// short to float
		inline float convert(const short& arg)
		{
			float s = float(arg) * ratio;
			return s;
		}
		//-----------------------------------------------------------------------------
		// float to short
		inline short convert(const float& arg)
		{
			return static_cast<short>(arg * 32767.0f);
		}
	}

	namespace audio
	{
		//-----------------------------------------------------------------------------
		// interleaved sample data.
		struct SampleData
		{
			// is this necessary?
			uint32_t blockSize = 0;
			uint32_t samples = 0;
			uint32_t channels = 0;
			uint32_t sampleRate = 0;
			// thus samples * channels in size
			std::vector<float> buffer;
			//
			const float* begin() const
			{
				return buffer.data();
			}
			//
			const float* end() const
			{
				return begin() + samples;
			}
		};

		//-----------------------------------------------------------------------------
		struct SampleIterator
		{
		private:
			const float* m_ps = nullptr;
			const float* m_pc = nullptr;
			const float* m_pe = nullptr;
			//
			uint32_t blockSize = 0;
			// a single contiguous data block
			// blockSize * channels
			std::shared_ptr<float> ptr;
			// vetor used to manage pointers into 
			// de-interleaving block
			std::vector<float*> buffer;
		public:
			//
			SampleIterator(const SampleData& arg)
			{
				blockSize = arg.blockSize;
				m_ps = arg.begin();
				m_pe = arg.end();
				m_pc = m_ps;
				ptr = std::shared_ptr<float>(new float[blockSize * arg.channels]);
				float* p = ptr.get();
				for (size_t s = 0; s < arg.channels; s++)
				{
					buffer.push_back(p);
					p += blockSize;
				}
				// copy the first block
				// copy();
			}
			//
			~SampleIterator() {}
			//
			void copy()
			{
				// can be optimized for common configurations!
				for (size_t sample = 0;
					sample < size() && more();
					sample++)
				{
					for (size_t channel = 0; channel < buffer.size(); channel++)
					{
						buffer[channel][sample] = *m_pc++;
					}
				}
			}
			//
			bool more() const
			{
				return (m_pc < m_pe);
			}
			//
			const float* const* data() const { return &buffer[0]; }
			//
			size_t size() const { return blockSize; }
		};

		//-----------------------------------------------------------------------------
		// de-interleaved sample block
		// C0 S0S1...Sn
		// C1 S0S1...Sn
		struct SampleBlock
		{
		private:
			//
			uint32_t m_available = 0;
			//
			uint32_t m_blockSize = 0;
			// a single contiguous data block
			// blockSize * channels
			std::shared_ptr<float> m_ptr;
			// vector used to manage pointers into 
			// de-interleaving block
			// N channels in size, 1 pointer per channel
			std::vector<float*> m_buffer;
		public:
			//
			SampleBlock(int blockSize, int channels)
			{
				m_blockSize = blockSize;
				m_ptr = std::shared_ptr<float>(new float[blockSize * channels]);
				float* p = m_ptr.get();
				for (int s = 0; s < channels; s++)
				{
					m_buffer.push_back(p);
					p += m_blockSize;
				}
			}
			//
			~SampleBlock() {}
			//
			float* const* data() { return &m_buffer[0]; }
			//
			const float* const* data() const { return &m_buffer[0]; }
			// get a channel buffer pointer
			const float* begin_data(size_t channel) const { return m_buffer[channel]; }
			const float* end_data(size_t channel) const { return (begin_data(channel) + blocksize()); }
			//
			size_t blocksize() const { return m_blockSize; }
			// 
			size_t available() const { return m_available; }
			//
			size_t channels() const { return m_buffer.size(); }
		};

		//-----------------------------------------------------------------------------
		static
			inline
			nv2::audio::SampleData
			interleave(const nv2::audio::SampleBlock& sb,
				nv2::audio::SampleData& opData,
				size_t samples
			)
		{
			// re-interleave
			const float* const* p = sb.data();
			if (sb.channels() == 2)
			{
				const float* psl = sb.begin_data(0);
				const float* pel = psl + samples;
				const float* psr = sb.begin_data(1);
				const float* per = psr + samples;
				while (psl < pel && psr < per)
				{
					opData.buffer.push_back(*psl++);
					opData.samples++;
					opData.buffer.push_back(*psr++);
					opData.samples++;
				}
			}
			else
			{
				for (size_t s = 0; s < sb.available(); s++)
				{
					for (size_t c = 0; c < sb.channels(); c++)
					{
						opData.buffer.push_back(p[c][s]);
						opData.samples++;
					}
				}
			}
			return opData;
		}

		//-----------------------------------------------------------------------------
		static
			inline
			nv2::audio::SampleData make(const nv2::audio::SampleData& arg)
		{
			nv2::audio::SampleData ret;
			ret.channels = arg.channels;
			ret.sampleRate = arg.sampleRate;
			ret.blockSize = arg.blockSize;
			ret.buffer.reserve(ret.blockSize);
			return ret;
		}

		//-----------------------------------------------------------------------------
		// returns a vector of normalized thumbnail values (+/-1.0f)
		// this is a positive peak peaker
		static
		std::vector<float> 
			thumbNail(const SampleData& sd, size_t widthPixels = 100)
		{
			//size_t samples = sd.samples / sd.channels;
			size_t samples = sd.samples;
			size_t blockSize = samples / widthPixels;
			const float* ps = &sd.buffer[0];
			const float* po = ps;
			const float* pi = po;
			const float* pe = ps + (blockSize * widthPixels);
			float peak = 0.0f;
			std::vector<float> peaks;
			for (/*po*/; po < pe; po += blockSize)
			{
				float fv = 0.0f;
				const float* pb = std::min(po + blockSize, pe);
				for (pi = po; pi < pb; pi += sd.channels)
				{
					// max only					
					fv = std::max(fv, std::abs(*pi));
				}
				// track peak value for normalising
				peak = std::max(peak, fv);
				peaks.push_back(fv);
			}
			// normalize all data to +/-1.0f
			float mul = powf(peak, -1);
			for (size_t s = 0; s < peaks.size(); s++)
				peaks[s] *= mul;
			//
			return peaks;
		}
	}
}
