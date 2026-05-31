#pragma once

#include "platform/win32/ProfileModel.h"

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct ProfileFileEntry {
    std::wstring name;
    std::filesystem::path path;
};

std::filesystem::path profileDirectoryPath();
std::wstring sanitizedProfileFileStem(std::wstring name);
std::filesystem::path profilePathForName(const std::wstring& name);
bool ensureProfileDirectory(std::string& error);
bool listSavedProfiles(std::vector<ProfileFileEntry>& outProfiles, std::string& error);

std::string serializeProfile(const BodyProfile& profile);
bool parseProfile(std::istream& input, BodyProfile& outProfile, std::string& error);
bool saveProfileToPath(const BodyProfile& profile, const std::filesystem::path& path, std::string& error);
bool loadProfileFromPath(const std::filesystem::path& path, BodyProfile& outProfile, std::string& error);

} // namespace ovtr::win32
