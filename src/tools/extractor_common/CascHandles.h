/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CascHandles_h__
#define CascHandles_h__

#include "Define.h"
#include <CascPort.h>
#include <array>
#include <functional>

namespace boost
{
    namespace filesystem
    {
        class path;
    }
}

namespace CASC
{
    char const* HumanReadableCASCError(uint32 error);

    struct FileIdentity
    {
        std::array<uint8, 16> ContentKey = { };
        std::array<uint8, 16> EncodedKey = { };
        uint64 ContentSize = 0;
        uint32 FileDataId = 0;
        uint32 LocaleFlags = 0;
        uint32 ContentFlags = 0;
        bool Complete = false;
    };

    class File;

    class Storage
    {
    public:
        using FileOpenObserver = std::function<void(FileIdentity const&)>;

        ~Storage();

        static Storage* Open(boost::filesystem::path const& path, uint32 localeMask, char const* product);
        static Storage* OpenRemote(boost::filesystem::path const& path, uint32 localeMask, char const* product, char const* region);

        uint32 GetBuildNumber() const;
        uint32 GetInstalledLocalesMask() const;
        bool HasTactKey(uint64 keyLookup) const;

        void SetFileOpenObserver(FileOpenObserver observer);

        File* OpenFile(char const* fileName, uint32 localeMask, bool printErrors = false, bool zerofillEncryptedParts = false) const;
        File* OpenFile(uint32 fileDataId, uint32 localeMask, bool printErrors = false, bool zerofillEncryptedParts = false) const;

    private:
        Storage(HANDLE handle);

        bool LoadOnlineTactKeys();
        File* OpenedFile(HANDLE handle) const;

        HANDLE _handle;
        FileOpenObserver _fileOpenObserver;
    };

    class File
    {
        friend class Storage;

    public:
        ~File();

        uint32 GetId() const;
        int64 GetSize() const;
        int64 GetPointer() const;
        bool SetPointer(int64 position);
        bool ReadFile(void* buffer, uint32 bytes, uint32* bytesRead);

    private:
        File(HANDLE handle);

        HANDLE _handle;
    };
}

#endif // CascHandles_h__
