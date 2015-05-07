/******************************************************************************
* Copyright (c) 2015, RadiantBlue Technologies, Inc.
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

#pragma once

#include <pdal/pdal_export.hpp>
#include <pdal/Writer.hpp>

#include "RialtoDb.hpp"

#include <cstdint>
#include <string>

namespace pdal
{

class Options;

class PDAL_DLL RialtoWriter : public Writer
{
public:
    RialtoWriter() :
        m_table(NULL)
    {}

    static void * create();
    static int32_t destroy(void *);
    
    // implemented in derived classes
    virtual std::string getName() const=0;
    virtual Options getDefaultOptions()=0;

    // helper functions
    
    static void serializeToTileSetInfo(MetadataNode tileSetNode,
                                       PointLayoutPtr layout,
                                       rialtosupport::RialtoDb::TileSetInfo& tileSetInfo);
    static void serializeToDimensionInfo(MetadataNode tileSetNode,
                                         PointLayoutPtr layout,
                                         std::vector<rialtosupport::RialtoDb::DimensionInfo>& infoList);
    static void serializeToPatch(PointView* view, Patch& patch);
    static void serializeToTileInfo(MetadataNode tileNode, PointView* view, rialtosupport::RialtoDb::TileInfo& tileInfo);

    static uint32_t getMetadataU32(const MetadataNode& parent, const std::string& name);
    static double getMetadataF64(const MetadataNode& parent, const std::string& name);
    static void extractStatistics(MetadataNode& tileSetNode, const std::string& dimName,
                                  double& minimum, double& mean, double& maximum);
    static unsigned char* createBlob(PointView* view, size_t& buflen);

protected:
    
    // implemented in derived classes
    virtual void processOptions(const Options& options)=0;
    
    // implemented in derived classes
    virtual void localStart() = 0;
    virtual void writeHeader(MetadataNode tileSetNode,
                             PointLayoutPtr layout) = 0;
    virtual void writeTile(MetadataNode, PointView*) = 0;
    virtual void localFinish() = 0;
    
private:
    virtual void ready(PointTableRef table);
    virtual void write(const PointViewPtr view);
    virtual void done(PointTableRef table);

    char* makeBuffer(PointView*);
    void makePointViewMap(MetadataNode tileSetNode);
    void writeEmptyTiles();
    
    BasePointTable *m_table;
    std::string m_directory;

    std::map<uint32_t, MetadataNode> m_pointViewMap;

    RialtoWriter& operator=(const RialtoWriter&); // not implemented
    RialtoWriter(const RialtoWriter&); // not implemented
};

} // namespace pdal