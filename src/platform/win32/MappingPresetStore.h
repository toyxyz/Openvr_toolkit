#pragma once

#include "platform/win32/MappingModel.h"
#include "platform/win32/ProfileModel.h"

#include <array>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct MappingPreset {
    std::wstring name;
    bool hasProfile = false;
    BodyProfile profile;
    std::array<std::wstring, kMappingSlotCount> deviceSerials{};
};

struct MappingPresetFileEntry {
    std::wstring name;
    std::filesystem::path path;
};

std::filesystem::path mappingPresetDirectoryPath();
std::wstring sanitizedMappingPresetFileStem(std::wstring name);
std::filesystem::path mappingPresetPathForName(const std::wstring& name);
bool ensureMappingPresetDirectory(std::string& error);
bool listSavedMappingPresets(std::vector<MappingPresetFileEntry>& outPresets, std::string& error);

std::string serializeMappingPreset(const MappingPreset& preset);
bool parseMappingPreset(std::istream& input, MappingPreset& outPreset, std::string& error);
bool saveMappingPresetToPath(
    const MappingPreset& preset,
    const std::filesystem::path& path,
    std::string& error
);
bool loadMappingPresetFromPath(
    const std::filesystem::path& path,
    MappingPreset& outPreset,
    std::string& error
);

} // namespace ovtr::win32
