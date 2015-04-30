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

#include <pdal/pdal_types.hpp>
#include <pdal/Writer.hpp>

#include <cassert>
#include <cstdint>
#include <vector>

namespace pdal
{

namespace tilercommon
{
    
class Rectangle
{
public:
    enum Quadrant
    {
        QuadrantSW=0, QuadrantNW=1, QuadrantSE=2, QuadrantNE=3,
    };

    Rectangle() : m_north(0.0), m_south(0.0), m_east(0.0), m_west(0.0), m_midx(0.0), m_midy(0.0)
        {}

    Rectangle(double w, double s, double e, double n) :
        m_north(n), m_south(s), m_east(e), m_west(w), m_midx((w+e)*0.5), m_midy((s+n)*0.5)
        {}

    Rectangle(const Rectangle& r) :
        m_north(r.m_north), m_south(r.m_south), m_east(r.m_east), m_west(r.m_west),
        m_midx((r.m_west+r.m_east)*0.5), m_midy((r.m_south+r.m_north)*0.5)
        {}

    Rectangle& operator=(const Rectangle& r)
    {
        m_north = r.m_north;
        m_south = r.m_south;
        m_east = r.m_east;
        m_west = r.m_west;
        m_midx = r.m_midx;
        m_midy = r.m_midy;
        return *this;
    }

    // return the quadrant of the child of this rect, for the given point
    Quadrant getQuadrantOf(double lon, double lat)
    {
        // NW=1  NE=3
        // SW=0  SE=2
        const bool lowX = (lon <= m_midx);
        const bool lowY = (lat <= m_midy);

        if (lowX && lowY)
            return QuadrantSW;
        if (lowX && !lowY)
            return QuadrantNW;
        if (!lowX && lowY)
            return QuadrantSE;
        return QuadrantNE;
    }

    // return the rect of the given child quadrant of this rect
    Rectangle getQuadrantRect(Quadrant q)
    {
        switch (q) {
        case QuadrantSW:
            return Rectangle(m_west, m_south, m_midx, m_midy);
        case QuadrantNW:
            return Rectangle(m_west, m_midy, m_midx, m_north);
        case QuadrantSE:
            return Rectangle(m_midx, m_south, m_east, m_midy);
        case QuadrantNE:
            return Rectangle(m_midx, m_midy, m_east, m_north);
        }

        throw pdal_error("invalid quadrant");
    }

    bool contains(double lon, double lat) const
    {
        return (lon >= m_west) && (lon <= m_east) &&
               (lat >= m_south) && (lat <= m_north);
    }

    double north() const { return m_north; }
    double south() const { return m_south; }
    double east() const { return m_east; }
    double west() const { return m_west; }
    double midx() const { return m_midx; }
    double midy() const { return m_midy; }

private:
    double m_north, m_south, m_east, m_west, m_midx, m_midy;
};


class Tile;


class TileSet
{
    public:
        TileSet(PointTableRef table, uint32_t maxLevel, LogPtr log);
        ~TileSet();

        void prep(const PointView* sourceView, PointViewSet* outputSet);
        void addPoint(PointId, double lon, double lat);
        uint32_t getMaxLevel() const { return m_maxLevel; }
        LogPtr log() { return m_log; }
        void setMasks();
        MetadataNode& metadata() { return m_metadata; }    
        PointViewPtr createPointView();
        
    private:
        
        PointTableRef m_table;
        const PointView* m_sourceView;
        PointViewSet* m_outputSet;
        uint32_t m_maxLevel;
        LogPtr m_log;
        Tile** m_roots;
        MetadataNode m_metadata;
};


class Tile
{
public:
    Tile(TileSet& tileSet, uint32_t level, uint32_t tx, uint32_t ty, Rectangle r);
    ~Tile();

    void add(const PointView* pointView, PointId pointNumber, double lon, double lat);
    
    void collectStats(std::vector<uint32_t> numTilesPerLevel,
        std::vector<uint64_t> numPointsPerLevel) const;
    
    MetadataNode& metadata() { return m_metadata; }
    
    void setMasks();
    
    void write(const char* dir);
    void writeData(FILE*) const;

private:
    LogPtr log() { return m_tileSet.log(); }
    char* getPointData(const PointView& buf, PointId& idx) const;
    void setMaskMetadata();
    
    TileSet& m_tileSet;
    uint32_t m_level;
    uint32_t m_tileX;
    uint32_t m_tileY;
    Tile** m_children;
    Rectangle m_rect;
    uint64_t m_skip;
    PointViewPtr m_pointView;
    MetadataNode m_metadata;
};

} // namespace tilercommon
} // namespace pdal
