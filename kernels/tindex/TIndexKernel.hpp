#include <pdal/Kernel.hpp>
/******************************************************************************
* Copyright (c) 2015, Howard Butler (howard@hobu.co)
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

#include <pdal/Kernel.hpp>
#include <pdal/Stage.hpp>
#include <pdal/util/FileUtils.hpp>
#include <pdal/plugin.hpp>


extern "C" int32_t TIndexKernel_ExitFunc();
extern "C" PF_ExitFunc TIndexKernel_InitPlugin();

namespace pdal
{
    

class PDAL_DLL TIndexKernel : public Kernel
{
public:
    static void * create();
    static int32_t destroy(void *);
    std::string getName() const;
    int execute(); // overrride
    
    
private:
    TIndexKernel();
    void addSwitches(); // overrride
    void validateSwitches(); // overrride

    void* fetchGeometry(MetadataNode metadata);
    void createDS(std::string const& filename);
    MetadataNode fetchInfo(std::string const& filename);
    void createLayer(std::string const& filename, std::string const& srs_wkt);
    
    std::string m_outputFilename;
    std::string m_indexDirectory;
    std::vector<std::string> m_files;
    std::string m_layerName;
    std::string m_driverName;
    std::string m_tileIndexColumnName;
    std::string m_srsColumnName;


    void* m_kernelFactory;
    void* m_DS;
    void* m_Layer;
    void* m_fDefn;
    bool m_bAbsolutePaths;
    std::string m_targetSRSString;
};

} // namespace pdal

