/******************************************************************************
* Copyright (c) 2014, Andrew Bell
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following
* conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in
*       the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of Hobu, Inc. or Flaxen Geo Consulting nor the
*       names of its contributors may be used to endorse or promote
*       products derived from this software without specific prior
*       written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
****************************************************************************/

#include <vector>
#include <algorithm>

#include <pdal/drivers/bpf/BpfHeader.hpp>
#include <pdal/drivers/bpf/BpfReader.hpp>
#include <pdal/drivers/bpf/BpfSeqIterator.hpp>
#include <pdal/PointBuffer.hpp>
#include <pdal/Schema.hpp>

#ifdef PDAL_HAVE_ZLIB
#include <zlib.h>
#endif

namespace pdal
{

BpfSeqIterator::BpfSeqIterator(const BpfDimensionList& dims,
        const BpfHeader& header, ILeStream& stream) :
    m_dims(dims), m_header(header), m_stream(stream), m_index(0),
    m_start(m_stream.position()), m_xDim(NULL), m_yDim(NULL), m_zDim(NULL)
{
    for (auto & d : m_dims)
    {
        if (d.m_label == "X")
            m_xDim = d.m_dim;
        if (d.m_label == "Y")
            m_yDim = d.m_dim;
        if (d.m_label == "Z")
            m_zDim = d.m_dim;
    }
    if (!m_xDim || !m_yDim || !m_zDim)
        throw pdal_error("Can't read BPF file without X, Y and Z dimensions.");

    if (m_header.m_compression)
    {
#ifdef PDAL_HAVE_ZLIB
        m_deflateBuf.resize(m_header.m_numPts * m_dims.size() * sizeof(float));
        size_t index = 0;
        size_t bytesRead = 0;
        do
        {
            bytesRead = readBlock(m_deflateBuf, index);
            index += bytesRead;
        } while (bytesRead > 0 && index < m_deflateBuf.size());
        m_charbuf.initialize(m_deflateBuf.data(), m_deflateBuf.size(), m_start);
        m_stream.pushStream(new std::istream(&m_charbuf));
#else
        throw "BPF compression required, but ZLIB is unavailable.";
#endif
    }
}

BpfSeqIterator::~BpfSeqIterator()
{
     delete m_stream.popStream();
}

point_count_t BpfSeqIterator::readBufferImpl(PointBuffer& data)
{
    return read(data, std::numeric_limits<point_count_t>::max());
}

point_count_t BpfSeqIterator::readImpl(PointBuffer& data, point_count_t count)
{
    return read(data, count);
}

size_t BpfSeqIterator::readBlock(std::vector<char>& outBuf, size_t index)
{
#ifdef PDAL_HAVE_ZLIB
    boost::uint32_t finalBytes;
    boost::uint32_t compressBytes;

    m_stream >> finalBytes;
    m_stream >> compressBytes;

    std::vector<char> in(compressBytes);

    // Fill the input bytes from the stream.
    m_stream.get(in);
    int ret = inflate(in.data(), compressBytes,
        outBuf.data() + index, finalBytes);
    return (ret ? 0 : finalBytes);
#else
    throw pdal_error("BPF compression required, but ZLIB is unavailable");
#endif
}
    
point_count_t BpfSeqIterator::read(PointBuffer& data, point_count_t count)
{
    switch (m_header.m_pointFormat)
    {
    case BpfFormat::PointMajor:
        return readPointMajor(data, count);
    case BpfFormat::DimMajor:
        return readDimMajor(data, count);
    case BpfFormat::ByteMajor:
        return readByteMajor(data, count);
    default:
        break;
    }
    return 0;
}

boost::uint64_t BpfSeqIterator::skipImpl(boost::uint64_t pointsToSkip)
{
    point_count_t lastIndex = m_index;
    m_index += (point_count_t)pointsToSkip;
    m_index = std::min(m_index, size_t(m_header.m_numPts));
    return std::min((point_count_t)pointsToSkip, m_index - lastIndex);
}

bool BpfSeqIterator::atEndImpl() const
{
    return m_index >= m_header.m_numPts;
}

point_count_t BpfSeqIterator::readPointMajor(PointBuffer& data,
    point_count_t count)
{
    PointId nextId = data.size();
    PointId idx = m_index;
    point_count_t numRead = 0;
    seekPointMajor(idx);
    while (numRead < count && idx < m_header.m_numPts)
    {
        for (size_t d = 0; d < m_dims.size(); ++d)
        {
            float f;

            m_stream >> f;
            data.setField(*m_dims[d].m_dim, nextId, f + m_dims[d].m_offset);
        }

        // Transformation X, Y and Z
        double x = data.getFieldAs<double>(*m_xDim, nextId);
        double y = data.getFieldAs<double>(*m_yDim, nextId);
        double z = data.getFieldAs<double>(*m_zDim, nextId);
        m_header.m_xform.apply(x, y, z);
        data.setField(*m_xDim, nextId, x);
        data.setField(*m_yDim, nextId, y);
        data.setField(*m_zDim, nextId, z);

        idx++;
        numRead++;
        nextId++;
    }
    m_index = idx;
    return numRead;
}

point_count_t BpfSeqIterator::readDimMajor(PointBuffer& data,
    point_count_t count)
{
    PointId idx;
    PointId startId = data.size();
    point_count_t numRead = 0;
    for (size_t d = 0; d < m_dims.size(); ++d)
    {
        idx = m_index;
        PointId nextId = startId;
        numRead = 0;
        seekDimMajor(d, idx);
        for (; numRead < count && idx < m_header.m_numPts;
            idx++, numRead++, nextId++)
        {
            float f;

            m_stream >> f;
            data.setField<float>(*m_dims[d].m_dim, nextId, f);
        }
    }
    m_index = idx;

    // Transform X, Y and Z
    for (idx = startId; idx < data.size(); idx++)
    {
        double x = data.getFieldAs<double>(*m_xDim, idx);
        double y = data.getFieldAs<double>(*m_yDim, idx);
        double z = data.getFieldAs<double>(*m_zDim, idx);
        m_header.m_xform.apply(x, y, z);
        data.setField(*m_xDim, idx, x);
        data.setField(*m_yDim, idx, y);
        data.setField(*m_zDim, idx, z);
    }

    return numRead;
}

point_count_t BpfSeqIterator::readByteMajor(PointBuffer& data,
    point_count_t count)
{
    PointId idx;
    PointId startId = data.size();
    point_count_t numRead = 0;

    for (size_t d = 0; d < m_dims.size(); ++d)
    {
        for (size_t b = 0; b < sizeof(float); ++b)
        {
            idx = m_index;
            numRead = 0;
            PointId nextId = startId;
            seekByteMajor(d, b, idx);
            for (;numRead < count && idx < m_header.m_numPts;
                idx++, numRead++, nextId++)
            {
                union 
                {
                    float f;
                    uint32_t u32;
                } u;

                u.u32 = 0;

                if (b)
                {
                    u.f = data.getFieldAs<float>(*m_dims[d].m_dim, nextId);
                }
                uint8_t u8;
                m_stream >> u8;
                u.u32 |= ((uint32_t)u8 << (b * CHAR_BIT));
                data.setField(*m_dims[d].m_dim, nextId, u.f);
            }
        }
    }
    m_index = idx;

    // Transform X, Y and Z
    for (idx = startId; idx < data.size(); idx++)
    {
        double x = data.getFieldAs<double>(*m_xDim, idx);
        double y = data.getFieldAs<double>(*m_yDim, idx);
        double z = data.getFieldAs<double>(*m_zDim, idx);
        m_header.m_xform.apply(x, y, z);
        data.setField(*m_xDim, idx, x);
        data.setField(*m_yDim, idx, y);
        data.setField(*m_zDim, idx, z);
    }

    return numRead;
}

void BpfSeqIterator::seekPointMajor(uint32_t ptIdx)
{
    std::streamoff offset = ptIdx * sizeof(float) * m_dims.size();
    m_stream.seek(m_start + offset);
}

void BpfSeqIterator::seekDimMajor(size_t dimIdx, uint32_t ptIdx)
{
    std::streamoff offset = ((sizeof(float) * dimIdx * m_header.m_numPts) +
        (sizeof(float) * ptIdx));
    m_stream.seek(m_start + offset);
}

void BpfSeqIterator::seekByteMajor(size_t dimIdx, size_t byteIdx, uint32_t ptIdx)
{
    std::streamoff offset =
        (dimIdx * m_header.m_numPts * sizeof(float)) +
        (byteIdx * m_header.m_numPts) +
        ptIdx;
    m_stream.seek(m_start + offset);
}

#ifdef PDAL_HAVE_ZLIB
int BpfSeqIterator::inflate(char *buf, size_t insize, char *outbuf,
    size_t outsize)
{
   if (insize == 0)
        return 0;

    int ret;
    z_stream strm;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    if (inflateInit(&strm) != Z_OK)
        return -2;

    strm.avail_in = insize;
    strm.next_in = (unsigned char *)buf;
    strm.avail_out = outsize;
    strm.next_out = (unsigned char *)outbuf;

    ret = ::inflate(&strm, Z_NO_FLUSH);
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? 0 : -1;
}
#endif

} //namespace
