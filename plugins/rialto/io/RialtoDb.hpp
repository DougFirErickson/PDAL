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

#include <cstdint>
#include <string>

// TODO: just used for Patch? (SQLite can be fwd declared)
#include <../plugins/sqlite/io/SQLiteCommon.hpp> // TODO: fix path

namespace pdal {
    class Log;
    class SQLite;
}

namespace rialtosupport
{

using namespace pdal;



class PDAL_DLL RialtoDb
{
public:
    // values match PDAL's
    enum DataType {
        Invalid = 0,
        Uint8 = 0x200 | 1,
        Uint16 = 0x200 | 2,
        Uint32 = 0x200 | 4,
        Uint64 = 0x200 | 8,
        Int8 = 0x100 | 1,
        Int16 = 0x100 | 2,
        Int32 = 0x100 | 4,
        Int64 = 0x100 | 8,
        Float32 = 0x400 | 4,
        Float64 = 0x400 | 8
    };

    struct DimensionInfo {
        std::string name;
        uint32_t position;
        DataType dataType;
        double minimum;
        double mean;
        double maximum;
    };

    // note we always use EPSG:4325
    struct TileSetInfo {
        std::string name; // aka filename
        uint32_t maxLevel;
        uint32_t numCols;
        uint32_t numRows;
        double minx; // tile set extents (not extents of actual data)
        double miny;
        double maxx;
        double maxy;
        uint32_t numDimensions;
    };

    struct TileInfo {
        uint32_t tileSetId;
        uint32_t level;
        uint32_t x; // col
        uint32_t y; // row
        Patch patch;
    };

    RialtoDb(const std::string& connection);

    ~RialtoDb();

    void create();
    void open(bool writable=false);

    void close();

    // adds a tile set to the database
    //
    // returns id of new data set
    uint32_t addTileSet(const RialtoDb::TileSetInfo& data);

    // returns id of new tile
    uint32_t addTile(const RialtoDb::TileInfo& data);

    // add all the dimensions of the tile set
    void addDimensions(uint32_t tileSetId,
                       const std::vector<DimensionInfo>& dimensions);

    // get list all the tile sets in the database, as a list of its
    std::vector<uint32_t> getTileSetIds();

    // get info about a specific tile set
    TileSetInfo getTileSetInfo(uint32_t tileSetId);

    // get info about one of the dimensions of a tile set
    DimensionInfo getDimensionInfo(uint32_t tileSetId, uint32_t dimension);

    // get info about a tile
    TileInfo getTileInfo(uint32_t tileId, bool withPoints);

    // use with caution for levels greater than 16 or so
    std::vector<uint32_t> getTileIdsAtLevel(uint32_t tileSetId, uint32_t level);

    // query for all the points of a tile set, bounded by bbox region
    // returns a pipeline made up of a BufferReader and a CropFilter
    Stage* query(uint32_t tileSetId,
                 double minx, double miny,
                 double max, double maxy,
                 uint32_t minLevel, uint32_t maxLevel);

private:
    // query for all the tiles of a tile set, bounded by bbox region
    std::vector<uint32_t> getTileSets(uint32_t tileSetId,
                                      double minx, double miny,
                                      double max, double maxy,
                                      uint32_t minLevel, uint32_t maxLevel);

    void createTileSetsTable();
    void createTilesTable();
    void createDimensionsTable();

    void query();

    LogPtr log() const { return m_log; }

    std::string m_connection;
    SQLite* m_session;
    LogPtr m_log;
    int m_srid;

    RialtoDb& operator=(const RialtoDb&); // not implemented
    RialtoDb(const RialtoDb&); // not implemented
};

} // namespace rialtosupport