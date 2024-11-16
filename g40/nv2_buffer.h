/*

	Visit https://github.com/g40

	Copyright (c) Jerry Evans, 1999-2024

	All rights reserved.

	The MIT License (MIT)

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.


*/

#pragma once

#include <algorithm>

namespace nv2
{
	//-----------------------------------------------------------------------------
	// semi-smart buffer for heap allocation
	template<typename T>
	class Buffer
	{
		using ptr_t = std::unique_ptr<T[]>;
		// data
		ptr_t m_pb;
		// count of elements *not* bytes
		size_t m_elements = 0;
	public:
		//
		explicit Buffer(size_t elements) :
			m_elements(elements)
		{
			m_pb = std::make_unique<T[]>(m_elements);
			memset(data(), 0, sizeof(T) * m_elements);
		}
		//
		T* data() noexcept {
			return m_pb.get();
		}
		const T* data() const {
			return m_pb.get();
		}
		//
		size_t size() const noexcept {
			return m_elements;
		}
		//
		T* begin() noexcept {
			return m_pb.get();
		}
		const T* begin() const {
			return m_pb.get();
		}
		T* end() noexcept {
			return begin() + size();
		}
		const T* end() const {
			return begin() + size();
		}
		//
		// const version
		template <typename P>
		const P* as() const {
			const P* ret = reinterpret_cast<P*>(m_pb.get());
			return ret;
		}
		// non-const
		template <typename P>
		P* as() {
			P* ret = reinterpret_cast<P*>(m_pb.get());
			return ret;
		}
	};
}
