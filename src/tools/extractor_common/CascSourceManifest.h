/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef CascSourceManifest_h__
#define CascSourceManifest_h__

#include "CascHandles.h"
#include <mutex>
#include <string_view>
#include <vector>

namespace boost::filesystem
{
    class path;
}

namespace CASC
{
    class SourceManifest
    {
    public:
        void Record(FileIdentity const& identity);
        void Write(boost::filesystem::path const& destination, std::string_view stage,
            std::string_view product, uint32 build) const;

    private:
        mutable std::mutex _mutex;
        std::vector<FileIdentity> _sources;
    };
}

#endif // CascSourceManifest_h__
