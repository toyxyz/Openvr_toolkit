#include "platform/win32/MappingOffsetPresetStore.h"

#include "platform/win32/ConfigStore.h"
#include "platform/win32/ConfigTextInternal.h"
#include "math/QuaternionUtils.h"
#include "platform/win32/ProfileStore.h"
#include "platform/win32/Win32String.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <unordered_map>
#include <utility>

namespace ovtr::win32 {
namespace {

bool parseRequiredFloat(
    const std::unordered_map<std::string, std::string>& values,
    const std::string& key,
    float& out,
    std::string& error
)
{
    const auto found = values.find(key);
    if (found == values.end() || !parseFloatConfigValue(found->second, out)) {
        error = "invalid offset preset field: " + key;
        return false;
    }
    return true;
}

std::string slotKey(const int slot, const char* field)
{
    return "slot" + std::to_string(slot) + "_" + field;
}

bool parseTransformFields(
    const std::unordered_map<std::string, std::string>& values,
    const std::string& prefix,
    MappingTransform& out,
    std::string& error
)
{
    const bool ok = parseRequiredFloat(values, prefix + "position_x", out.position.x, error) &&
        parseRequiredFloat(values, prefix + "position_y", out.position.y, error) &&
        parseRequiredFloat(values, prefix + "position_z", out.position.z, error) &&
        parseRequiredFloat(values, prefix + "rotation_x", out.rotation[0], error) &&
        parseRequiredFloat(values, prefix + "rotation_y", out.rotation[1], error) &&
        parseRequiredFloat(values, prefix + "rotation_z", out.rotation[2], error) &&
        parseRequiredFloat(values, prefix + "rotation_w", out.rotation[3], error);
    if (ok) {
        out.rotation = ovtr::normalizeQuaternion(out.rotation);
    }
    return ok;
}

bool readPresetValues(std::istream& input, std::unordered_map<std::string, std::string>& values, std::string& error)
{
    values.clear();
    std::string line;
    while (std::getline(input, line)) {
        line = trimAscii(line);
        if (line.empty() || line.front() == '#') {
            continue;
        }
        detail::ConfigAssignment assignment;
        if (!detail::parseConfigAssignmentLine(line, assignment)) {
            error = "invalid offset preset line: " + line;
            return false;
        }
        values[assignment.key] = assignment.value;
    }
    return true;
}

} // namespace

std::filesystem::path mappingOffsetPresetDirectoryPath()
{
    return executableDirectoryPath() / "offset";
}

std::wstring sanitizedMappingOffsetPresetFileStem(std::wstring name)
{
    return sanitizedProfileFileStem(std::move(name));
}

std::filesystem::path mappingOffsetPresetPathForName(const std::wstring& name)
{
    return mappingOffsetPresetDirectoryPath() / (sanitizedMappingOffsetPresetFileStem(name) + L".offset");
}

bool ensureMappingOffsetPresetDirectory(std::string& error)
{
    std::error_code createError;
    const std::filesystem::path directory = mappingOffsetPresetDirectoryPath();
    std::filesystem::create_directories(directory, createError);
    if (createError) {
        error = "could not create " + directory.string() + ": " + createError.message();
        return false;
    }
    return true;
}

bool listSavedMappingOffsetPresets(std::vector<MappingOffsetPresetFileEntry>& outPresets, std::string& error)
{
    outPresets.clear();
    const std::filesystem::path directory = mappingOffsetPresetDirectoryPath();
    std::error_code existsError;
    if (!std::filesystem::exists(directory, existsError)) {
        if (existsError) {
            error = "could not inspect offset directory: " + existsError.message();
            return false;
        }
        return true;
    }
    if (!std::filesystem::is_directory(directory, existsError) || existsError) {
        error = "offset path is not a directory: " + directory.string();
        return false;
    }

    std::error_code iterateError;
    for (std::filesystem::directory_iterator it(directory, iterateError), end; it != end; it.increment(iterateError)) {
        if (iterateError) {
            error = "could not enumerate offset directory: " + iterateError.message();
            return false;
        }
        std::error_code fileError;
        if (!it->is_regular_file(fileError) || fileError || it->path().extension() != L".offset") {
            continue;
        }
        outPresets.push_back(MappingOffsetPresetFileEntry{it->path().stem().wstring(), it->path()});
    }
    std::sort(outPresets.begin(), outPresets.end(), [](const auto& left, const auto& right) {
        return left.name < right.name;
    });
    return true;
}

std::string serializeMappingOffsetPresetList(
    const std::wstring& name,
    const std::array<MappingTransform, kMappingSlotCount>& offsets
)
{
    std::ostringstream output;
    output << "# toyxyz_openvr_toolkit offset preset v2\n";
    output << "version=2\n";
    output << "name=" << narrow(name) << "\n";
    output << std::setprecision(9);
    for (int slot = 0; slot < kMappingSlotCount; ++slot) {
        const MappingTransform& offset = offsets[static_cast<std::size_t>(slot)];
        output << slotKey(slot, "position_x") << "=" << offset.position.x << "\n";
        output << slotKey(slot, "position_y") << "=" << offset.position.y << "\n";
        output << slotKey(slot, "position_z") << "=" << offset.position.z << "\n";
        output << slotKey(slot, "rotation_x") << "=" << offset.rotation[0] << "\n";
        output << slotKey(slot, "rotation_y") << "=" << offset.rotation[1] << "\n";
        output << slotKey(slot, "rotation_z") << "=" << offset.rotation[2] << "\n";
        output << slotKey(slot, "rotation_w") << "=" << offset.rotation[3] << "\n";
    }
    return output.str();
}

bool parseMappingOffsetPresetList(
    std::istream& input,
    std::array<MappingTransform, kMappingSlotCount>& outOffsets,
    std::string& error
)
{
    std::unordered_map<std::string, std::string> values;
    if (!readPresetValues(input, values, error)) {
        return false;
    }
    std::array<MappingTransform, kMappingSlotCount> parsed{};
    for (int slot = 0; slot < kMappingSlotCount; ++slot) {
        MappingTransform offset;
        if (!parseTransformFields(values, "slot" + std::to_string(slot) + "_", offset, error)) {
            return false;
        }
        parsed[static_cast<std::size_t>(slot)] = offset;
    }
    outOffsets = parsed;
    return true;
}

bool saveMappingOffsetPresetListToPath(
    const std::wstring& name,
    const std::array<MappingTransform, kMappingSlotCount>& offsets,
    const std::filesystem::path& path,
    std::string& error
)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        error = "could not open offset preset for writing: " + path.string();
        return false;
    }
    output << serializeMappingOffsetPresetList(name, offsets);
    if (!output) {
        error = "failed while writing offset preset: " + path.string();
        return false;
    }
    return true;
}

bool loadMappingOffsetPresetListFromPath(
    const std::filesystem::path& path,
    std::array<MappingTransform, kMappingSlotCount>& outOffsets,
    std::string& error
)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "could not open offset preset: " + path.string();
        return false;
    }
    return parseMappingOffsetPresetList(input, outOffsets, error);
}

} // namespace ovtr::win32
