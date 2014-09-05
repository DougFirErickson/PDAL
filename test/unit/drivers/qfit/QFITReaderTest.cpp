/******************************************************************************
* Copyright (c) 2011, Michael P. Gerlek (mpg@flaxen.com)
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

#include <boost/test/unit_test.hpp>
#include <boost/cstdint.hpp>

#include <pdal/StageIterator.hpp>
#include <pdal/Options.hpp>
#include <pdal/PointBuffer.hpp>
#include <pdal/drivers/qfit/Reader.hpp>
#include <pdal/PipelineReader.hpp>
#include <pdal/PipelineManager.hpp>
#include "StageTester.hpp"
#include "Support.hpp"

#include <iostream>

#ifdef PDAL_COMPILER_GCC
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

using namespace pdal;

BOOST_AUTO_TEST_SUITE(QFITReaderTest)


#define Compare(x,y)    BOOST_CHECK_CLOSE(x,y,0.00001);


void Check_Point(const pdal::PointBuffer& data,
                 std::size_t index,
                 double xref, double yref, double zref,
                 boost::int32_t tref)
{
    const Schema& schema = data.getSchema();

    Dimension const& dimX = schema.getDimension("X");
    Dimension const& dimY = schema.getDimension("Y");
    Dimension const& dimZ = schema.getDimension("Z");
    Dimension const& dimTime = schema.getDimension("Time");

    double x = data.getFieldAs<double>(dimX, index);
    double y = data.getFieldAs<double>(dimY, index);
    double z = data.getFieldAs<double>(dimZ, index);
    double t = data.getFieldAs<double>(dimTime, index);

    Compare(x, xref);
    Compare(y, yref);
    Compare(z, zref);
    BOOST_CHECK_EQUAL(t, tref);
}

BOOST_AUTO_TEST_CASE(test_10_word)
{
    PointContext ctx;

    Options options;

    Option filename("filename", Support::datapath("qfit/10-word.qi"),
        "Input filename for reader to use");
    Option flip_coordinates("flip_coordinates", false,
        "Flip coordinates from 0-360 to -180-180");
    Option scale_z("scale_z", 0.001f, "Z scale from mm to m");

    options.add(scale_z);
    options.add(flip_coordinates);
    options.add(filename);
    drivers::qfit::Reader reader(options);
    BOOST_CHECK(reader.getDescription() == "QFIT Reader");
    BOOST_CHECK_EQUAL(reader.getName(), "drivers.qfit.reader");
    reader.prepare(ctx);

    StageTester::ready(&reader, ctx);
    PointBuffer data(ctx);
    StageSequentialIterator* iter = reader.createSequentialIterator();
    {
        uint32_t numRead = iter->read(data, 3);
        BOOST_CHECK_EQUAL(numRead, 3);
    }

    delete iter;

    Check_Point(data, 0, 221.826822, 59.205160, 32.0900, 0);
    Check_Point(data, 1, 221.826740, 59.205161, 32.0190, 0);
    Check_Point(data, 2, 221.826658, 59.205164, 32.0000, 0);
}

BOOST_AUTO_TEST_CASE(test_14_word)
{
    PointContext ctx;
    Options options;

    Option filename("filename", Support::datapath("qfit/14-word.qi"),
        "Input filename for reader to use");
    options.add(filename);
    Option flip_coordinates("flip_coordinates", false,
        "Flip coordinates from 0-360 to -180-180");
    Option scale_z("scale_z", 0.001f, "Z scale from mm to m");

    options.add(scale_z);
    options.add(flip_coordinates);

    drivers::qfit::Reader reader(options);
    reader.prepare(ctx);

    StageTester::ready(&reader, ctx);
    PointBuffer data(ctx);
    StageSequentialIterator* iter = reader.createSequentialIterator();
    {
        boost::uint32_t numRead = iter->read(data, 3);
        BOOST_CHECK_EQUAL(numRead, 3);
    }
    delete iter;

    Check_Point(data, 0, 244.306337, 35.623317, 1056.830000000, 903);
    Check_Point(data, 1, 244.306260, 35.623280, 1056.409000000, 903);
    Check_Point(data, 2, 244.306204, 35.623257, 1056.483000000, 903);
}

BOOST_AUTO_TEST_SUITE_END()
