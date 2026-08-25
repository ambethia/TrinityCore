/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CascSourceManifest.h"
#include <CascLib.h>
#include <boost/filesystem/operations.hpp>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace
{
    bool IsZeroKey(std::array<uint8, 16> const& key)
    {
        return std::all_of(key.begin(), key.end(), [](uint8 byte) { return byte == 0; });
    }

    std::string HexKey(std::array<uint8, 16> const& key)
    {
        std::ostringstream output;
        output << std::hex << std::setfill('0');
        for (uint8 byte : key)
            output << std::setw(2) << uint32(byte);
        return output.str();
    }

    void WriteJsonString(std::ostream& output, std::string_view value)
    {
        output << '"';
        for (char ch : value)
        {
            switch (ch)
            {
                case '"': output << "\\\""; break;
                case '\\': output << "\\\\"; break;
                case '\b': output << "\\b"; break;
                case '\f': output << "\\f"; break;
                case '\n': output << "\\n"; break;
                case '\r': output << "\\r"; break;
                case '\t': output << "\\t"; break;
                default:
                    if (static_cast<unsigned char>(ch) < 0x20)
                        throw std::runtime_error("CASC source manifest string contains an unsupported control character");
                    output << ch;
                    break;
            }
        }
        output << '"';
    }

    auto IdentityKey(CASC::FileIdentity const& identity)
    {
        return std::tie(identity.FileDataId, identity.LocaleFlags, identity.ContentFlags,
            identity.ContentKey, identity.EncodedKey, identity.ContentSize);
    }
}

void CASC::SourceManifest::Record(FileIdentity const& identity)
{
    std::lock_guard lock(_mutex);
    _sources.push_back(identity);
}

void CASC::SourceManifest::Write(boost::filesystem::path const& destination, std::string_view stage,
    std::string_view product, uint32 build) const
{
    std::vector<FileIdentity> sources;
    {
        std::lock_guard lock(_mutex);
        sources = _sources;
    }

    if (stage.empty() || product.empty() || build == 0)
        throw std::runtime_error("CASC source manifest provenance must be complete");
    if (sources.empty())
        throw std::runtime_error("CASC source manifest contains no opened sources");
    if (std::any_of(sources.begin(), sources.end(), [](FileIdentity const& identity)
        {
            return !identity.Complete || identity.FileDataId == CASC_INVALID_ID ||
                IsZeroKey(identity.ContentKey) || IsZeroKey(identity.EncodedKey);
        }))
        throw std::runtime_error("CASC source manifest contains a source without a complete file identity");

    std::sort(sources.begin(), sources.end(), [](FileIdentity const& left, FileIdentity const& right)
        {
            return IdentityKey(left) < IdentityKey(right);
        });
    sources.erase(std::unique(sources.begin(), sources.end(), [](FileIdentity const& left, FileIdentity const& right)
        {
            return IdentityKey(left) == IdentityKey(right);
        }), sources.end());

    for (std::size_t index = 1; index < sources.size(); ++index)
    {
        FileIdentity const& previous = sources[index - 1];
        FileIdentity const& current = sources[index];
        if (std::tie(previous.FileDataId, previous.LocaleFlags, previous.ContentFlags) ==
            std::tie(current.FileDataId, current.LocaleFlags, current.ContentFlags) &&
            IdentityKey(previous) != IdentityKey(current))
            throw std::runtime_error("CASC source identity changed during extraction");
    }

    boost::filesystem::path const staging(destination.string() + ".tmp");
    std::ofstream output(staging.string(), std::ios::binary | std::ios::trunc);
    if (!output)
        throw std::runtime_error("could not create CASC source manifest staging file: " + staging.string());

    output << "{\n  \"schema\": \"luckydo.casc-sources\",\n  \"version\": 1,\n  \"stage\": ";
    WriteJsonString(output, stage);
    output << ",\n  \"product\": ";
    WriteJsonString(output, product);
    output << ",\n  \"build\": " << build << ",\n  \"sources\": [\n";
    for (std::size_t index = 0; index < sources.size(); ++index)
    {
        FileIdentity const& source = sources[index];
        output << "    {\"file_data_id\": " << source.FileDataId
            << ", \"content_key\": \"" << HexKey(source.ContentKey)
            << "\", \"encoded_key\": \"" << HexKey(source.EncodedKey)
            << "\", \"content_size\": " << source.ContentSize
            << ", \"locale_flags\": " << source.LocaleFlags
            << ", \"content_flags\": " << source.ContentFlags << "}";
        output << (index + 1 == sources.size() ? "\n" : ",\n");
    }
    output << "  ]\n}\n";
    output.close();
    if (!output)
        throw std::runtime_error("failed to write CASC source manifest: " + staging.string());

    boost::system::error_code error;
    boost::filesystem::rename(staging, destination, error);
    if (error)
    {
        boost::filesystem::remove(staging);
        throw std::runtime_error("failed to publish CASC source manifest: " + error.message());
    }
}
