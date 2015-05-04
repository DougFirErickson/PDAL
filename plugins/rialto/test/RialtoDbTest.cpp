//#include <pdal/pdal_test_main.hpp>
#include "gtest/gtest.h"

#include <pdal/Options.hpp>
#include <pdal/PointTable.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/util/Bounds.hpp>
#include <pdal/util/FileUtils.hpp>

#include <pdal/BufferReader.hpp>
#include <tiler/TilerFilter.hpp>
#include <stats/StatsFilter.hpp>

#include "RialtoDb.hpp"

#include "Support.hpp"

using namespace pdal;

TEST(RialtoDbTest, foo)
{
    RialtoDb db;

    EXPECT_EQ(db.foo(), 17);
}
