/******************************************************************************
* Copyright (c) 2012, Howard Butler (hobu@hobu.net)
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

#include <pdal/filters/Selector.hpp>

//#include <iostream>
#include <map>

//#include <boost/algorithm/string.hpp>
#include <boost/uuid/string_generator.hpp>

namespace pdal
{
namespace filters
{


void Selector::processOptions(const Options &options)
{
    m_overwriteExisting = options.getValueOrDefault<bool>(
        "overwite_existing_dimensions", true);
    m_ignoreDefault = options.getValueOrDefault<bool>("ignore_default", true);

    try
    {
        Option ignored = options.getOption("ignore");
        boost::optional<Options const&> ignored_options = ignored.getOptions();

        if (ignored_options)
        {
            std::vector<Option> ignored_dims =
                ignored_options->getOptions("dimension");
            for (auto i = ignored_dims.begin(); i != ignored_dims.end(); ++i)
                m_ignoredMap[i->getValue<std::string>()] = true;
        }
    }
    catch (option_not_found&)
    {}

    try
    {
        Option keep = m_options.getOption("keep");
        boost::optional<Options const&> keep_options = keep.getOptions();

        if (keep_options)
        {
            std::vector<Option> keep_dims =
                keep_options->getOptions("dimension");
            for (auto i = keep_dims.begin(); i != keep_dims.end(); ++i)
                m_ignoredMap[i->getValue<std::string>()] = false;
        }
    }
    catch (option_not_found&)
    {}

    try
    {
        Option create = m_options.getOption("create");
        boost::optional<Options const&> create_options = create.getOptions();

        if (create_options)
        {
            std::vector<Option> create_dims =
                create_options->getOptions("dimension");
            for (auto i = create_dims.begin(); i != create_dims.end(); ++i)
            {
                Option const& o = *i;
                boost::optional<Options const&> dim_ops = o.getOptions();

                if (!dim_ops)
                    throw pdal_error("create dimension option has no sub "
                        "options!");

                Options const& ops = *dim_ops;

                std::string name = o.getValue<std::string>();
                double scale = ops.getValueOrDefault<double>("scale", 1.0);
                double offset = ops.getValueOrDefault<double>("offset", 0.0);
                uint32_t size = ops.getValueOrDefault<uint32_t>("size", 1);
                std::string description =
                    ops.getValueOrDefault<std::string>("description", "");
                std::string interpretation =
                    ops.getValueOrDefault<std::string>("interpretation",
                        "int32_t");
                dimension::id uuid =
                    ops.getValueOrDefault<dimension::id>("uuid",
                        boost::uuids::nil_uuid());
                dimension::id parent_uuid =
                    ops.getValueOrDefault<dimension::id>("parent_uuid",
                        boost::uuids::nil_uuid());
                double minimum = ops.getValueOrDefault<double>("minimum", 0.0);
                double maximum = ops.getValueOrDefault<double>("maximum", 0.0);

                dimension::Interpretation interp =
                    Dimension::getInterpretation(interpretation);

                Dimension d(name, interp, size, description);
                m_hasScaleMap[&d] = ops.hasOption("scale");
                m_hasOffsetMap[&d] = ops.hasOption("offset");
                d.setUUID(uuid);

                if (d.getUUID().is_nil())
                    d.createUUID();
                d.setParent(parent_uuid);
                d.setNumericScale(scale);
                d.setNumericOffset(offset);
                d.setMinimum(minimum);
                d.setMaximum(maximum);
                d.setNamespace(getName());
                m_createDimensions.push_back(d);
            }
        }
    }
    catch (option_not_found&)
    {}
}


void Selector::buildSchema(Schema *schema)
{
    // Add new dimensions to the schema first in case we wanted to ignore them
    // or something silly like that.
    for (auto di = m_createDimensions.begin();
        di != m_createDimensions.end(); ++di)
    {
        Dimension& d = *di;
        boost::optional<Dimension const&> old_dim =
            schema->getDimensionOptional(d.getName());

        if (old_dim && !m_overwriteExisting)
            continue;

        if (old_dim)
            d.setParent(old_dim->getUUID());
        m_ignoredMap[d.getName()] = false;
        Dimension *td = schema->appendDimension(d);
        m_hasScaleMap[td] = m_hasScaleMap[&d];
        m_hasScaleMap.erase(&d);
        m_hasOffsetMap[td] = m_hasOffsetMap[&d];
        m_hasOffsetMap.erase(&d);
    }

    schema::Map& dimensions = schema->getDimensions();
    schema::index_by_index& dims = dimensions.get<schema::index>();
    for (auto di = dims.begin(); di != dims.end(); ++di)
    {
        Dimension d = *di;
        auto ii = m_ignoredMap.find(d.getName());
        if  ((ii == m_ignoredMap.end() && m_ignoreDefault) ||
            ii->second)
        {
            // Set the ingore flag on these dimensions.
            dims.modify(di,
                [d](Dimension &dr)
                { dr.setFlags(d.getFlags() | dimension::IsIgnored); }
            );
        }
    }
}


void Selector::ready(PointContext ctx)
{
    Schema *s = ctx.schema();
    
    for (auto mi : m_hasScaleMap)
    {
        if (!mi.second && mi.first->getParent().is_nil())
        {
            Dimension *parent = s->getDimensionPtr(mi.first->getParent());
            mi.first->setNumericScale(parent->getNumericScale());
        }
    }

    for (auto mi : m_hasOffsetMap)
    {
        if (!mi.second && mi.first->getParent().is_nil())
        {
            Dimension *parent = s->getDimensionPtr(mi.first->getParent());
            mi.first->setNumericOffset(parent->getNumericOffset());
        }
    }
}


Options Selector::getDefaultOptions()
{
    Options options;
    Option ignore("ignore", "DimensionName",
        "An Options set with entries of name 'dimension' names to ignore");
    return options;
}

} // namespace filter
} // namespace pdal
