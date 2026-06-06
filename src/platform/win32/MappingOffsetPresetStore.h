#pragma once

#include "platform/win32/MappingCalibrationModel.h"

#include <array>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace ovtr::win32 {

struct MappingOffsetPresetFileEntry {
    std::wstring name;
    std::filesystem::path path;
};

std::filesystem::path mappingOffsetPresetDirectoryPath();
std::wstring sanitizedMappingOffsetPresetFileStem(std::wstring name);
std::filesystem::path mappingOffsetPresetPathForName(const std::wstring& name);
bool ensureMappingOffsetPresetDirectory(std::string& error);
bool listSavedMappingOffsetPresets(std::vector<MappingOffsetPresetFileEntry>& outPresets, std::string& error);

std::string serializeMappingOffsetPresetList(
    const std::wstring& name,
    const std::array<MappingTransform, kMappingSlotCount>& offsets
);
bool parseMappingOffsetPresetList(
    std::istream& input,
    std::array<MappingTransform, kMappingSlotCount>& outOffsets,
    std::string& error
);
bool saveMappingOffsetPresetListToPath(
    const std::wstring& name,
    const std::array<MappingTransform, kMappingSlotCount>& offsets,
    const std::filesystem::path& path,
    std::string& error
);
bool loadMappingOffsetPresetListFromPath(
    const std::filesystem::path& path,
    std::array<MappingTransform, kMappingSlotCount>& outOffsets,
    std::string& error
);

} // namespace ovtr::win32
