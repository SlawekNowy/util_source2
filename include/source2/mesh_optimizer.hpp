/*
MIT License

Copyright (c) 2021 Silverlan
Copyright (c) 2015 Steam Database

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __SOURCE2_MESH_OPTIMIZER_HPP__
#define __SOURCE2_MESH_OPTIMIZER_HPP__

#include "util_source2_definitions.hpp"
#include <cinttypes>
#include <vector>
#include <string>
#include <array>
#include <cinttypes>
#include <stdexcept>

class DataStream;
namespace source2::resource
{
	class DLLUS2 MeshOptimizerVertexDecoder
	{
	public:
		static constexpr uint8_t VertexHeader = 0xa0;

		static constexpr int32_t VertexBlockSizeBytes = 8192;
		static constexpr int32_t VertexBlockMaxSize = 256;
		static constexpr int32_t ByteGroupSize = 16;
		static constexpr int32_t TailMaxSize = 32;

		static uint32_t GetVertexBlockSize(uint32_t vertexSize);
		static uint8_t Unzigzag8(uint8_t v);
		// Note: These have been moved to the cpp because the span class being used messes with the debugger.
		// Once C++-20 is available, these should be moved back to the header and tcb::span should be replaced by std::span!
		//static tcb::span<uint8_t> DecodeBytesGroup(const tcb::span<uint8_t> &data, tcb::span<uint8_t> destination, int bitslog2);
		//static tcb::span<uint8_t> DecodeBytes(tcb::span<uint8_t> data, tcb::span<uint8_t> destination);
		//static tcb::span<uint8_t> DecodeVertexBlock(tcb::span<uint8_t> data, tcb::span<uint8_t> vertexData, int vertexCount, int vertexSize, tcb::span<uint8_t> lastVertex);
		static std::vector<uint8_t> DecodeVertexBuffer(int vertexCount, int vertexSize, const std::vector<uint8_t> &vertexBuffer);
	};

	class DLLUS2 MeshOptimizerIndexDecoder
	{
	public:
		static constexpr uint8_t IndexHeader = 0xe0;

		static void PushEdgeFifo(std::array<std::pair<uint32_t, uint32_t>,16> &fifo, int &offset, uint32_t a, uint32_t b);
		static void PushVertexFifo(std::array<uint32_t,16> &fifo, int &offset, uint32_t v, bool cond = true);
		static uint32_t DecodeVByte(DataStream &data);
		static uint32_t DecodeIndex(DataStream &data, uint32_t next, uint32_t last);
		static void WriteTriangle(std::vector<uint8_t> &destination, int offset, int indexSize, uint32_t a, uint32_t b, uint32_t c);
		static std::vector<uint8_t> DecodeIndexBuffer(int indexCount, int indexSize, std::vector<uint8_t> &buffer);
	};
};

#endif
