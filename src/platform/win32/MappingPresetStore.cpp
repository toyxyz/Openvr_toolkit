#include "platform/win32/MappingPresetStore.h"

#include "platform/win32/ConfigStore.h"
#include "platform/win32/ConfigTextInternal.h"
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

std::string slotKey(const int index)
{
    return "slot_" + std::to_string(index);
}

std::string profileKey(const std::string& key)
{
    return "profile_" + key;
}

bool parseEmbeddedProfile(
    const std::unordered_map<std::string, std::string>& values,
    MappingPreset& preset,
    std::string& error
)
{
    const auto profileName = values.find("profile_name");
    if (profileName == values.end()) {
        return true;
    }

    std::ostringstream profileText;
    profileText << "name=" << profileName->second << "\n";
    const auto& definitions = profileMeasurementDefinitions();
    for (int i = 0; i < kProfileMeasurementCount; ++i) {
        const std::string key = profileKey(definitions[static_cast<std::size_t>(i)].key);
        const auto found = values.find(key);
        if (found != values.end()) {
            profileText << definitions[static_cast<std::size_t>(i)].key << "=" << found->second << "\n";
        }
    }

    std::istringstream input(profileText.str());
    if (!parseProfile(input, preset.profile, error)) {
        error = "invalid embedded mapping profile: " + error;
        return false;
    }
    preset.hasProfile = true;
    return true;
}

} // namespace

std::filesystem::path mappingPresetDirectoryPath()
{
    return executableDirectoryPath() / "mapping";
}

std::wstring sanitizedMappingPresetFileStem(std::wstring name)
{
    return sanitizedProfileFileStem(std::move(name));
}

std::filesystem::path mappingPresetPathForName(const std::wstring& name)
{
    return mappingPresetDirectoryPath() / (sanitizedMappingPresetFileStem(name) + L".mapping");
}

bool ensureMappingPresetDirectory(std::string& error)
{
    std::error_code createError;
    const std::filesystem::path directory = mappingPresetDirectoryPath();
    std::filesystem::create_directories(directory, createError);
    if (createError) {
        error = "could not create " + directory.string() + ": " + createError.message();
        return false;
    }
    return true;
}

bool listSavedMappingPresets(std::vector<MappingPresetFileEntry>& outPresets, std::string& error)
{
    outPresets.clear();
    const std::filesystem::path directory = mappingPresetDirectoryPath();
    std::error_code existsError;
    if (!std::filesystem::exists(directory, existsError)) {
        if (existsError) {
            error = "could not inspect mapping directory: " + existsError.message();
            return false;
        }
        return true;
    }
    if (!std::filesystem::is_directory(directory, existsError) || existsError) {
        error = "mapping path is not a directory: " + directory.string();
        return false;
    }

    std::error_code iterateError;
    for (std::filesystem::directory_iterator it(directory, iterateError), end; it != end; it.increment(iterateError)) {
        if (iterateError) {
            error = "could not enumerate mapping directory: " + iterateError.message();
            return false;
        }
        std::error_code fileError;
        if (!it->is_regular_file(fileError) || fileError || it->path().extension() != L".mapping") {
            continue;
        }
        outPresets.push_back(MappingPresetFileEntry{it->path().stem().wstring(), it->path()});
    }

    std::sort(outPresets.begin(), outPresets.end(), [](const auto& left, const auto& right) {
        return left.name < right.name;
    });
    return true;
}

std::string serializeMappingPreset(const MappingPreset& preset)
{
    std::ostringstream output;
    output << "# toyxyz_openvr_toolkit mapping preset v3\n";
    output << "version=3\n";
    output << "name=" << narrow(preset.name) << "\n";
    if (preset.hasProfile) {
        output << "profile_name=" << narrow(preset.profile.name) << "\n";
        output << std::setprecision(9);
        const auto& definitions = profileMeasurementDefinitions();
        for (int i = 0; i < kProfileMeasurementCount; ++i) {
            const auto index = static_cast<std::size_t>(i);
            output << profileKey(definitions[index].key) << "="
                   << preset.profile.measurements[index] << "\n";
        }
    }
    for (int i = 0; i < kMappingSlotCount; ++i) {
        output << slotKey(i) << "=" << narrow(preset.deviceSerials[static_cast<std::size_t>(i)]) << "\n";
    }
    return output.str();
}

bool parseMappingPreset(std::istream& input, MappingPreset& outPreset, std::string& error)
{
    std::unordered_map<std::string, std::string> values;
    std::string line;
    while (std::getline(input, line)) {
        line = trimAscii(line);
        if (line.empty() || line.front() == '#') {
            continue;
        }
        detail::ConfigAssignment assignment;
        if (!detail::parseConfigAssignmentLine(line, assignment)) {
            error = "invalid mapping preset line: " + line;
            return false;
        }
        values[assignment.key] = assignment.value;
    }

    MappingPreset preset;
    const auto name = values.find("name");
    preset.name = name == values.end() ? L"mapping" : widen(name->second);
    for (int i = 0; i < kMappingSlotCount; ++i) {
        const auto found = values.find(slotKey(i));
        if (found == values.end()) {
            error = "missing mapping preset field: " + slotKey(i);
            return false;
        }
        preset.deviceSerials[static_cast<std::size_t>(i)] = widen(found->second);
    }
    if (!parseEmbeddedProfile(values, preset, error)) {
        return false;
    }
    outPreset = std::move(preset);
    return true;
}

bool saveMappingPresetToPath(const MappingPreset& preset, const std::filesystem::path& path, std::string& error)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        error = "could not open mapping preset for writing: " + path.string();
        return false;
    }
    output << serializeMappingPreset(preset);
    if (!output) {
        error = "failed while writing mapping preset: " + path.string();
        return false;
    }
    return true;
}

bool loadMappingPresetFromPath(const std::filesystem::path& path, MappingPreset& outPreset, std::string& error)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "could not open mapping preset: " + path.string();
        return false;
    }
    return parseMappingPreset(input, outPreset, error);
}

} // namespace ovtr::win32
