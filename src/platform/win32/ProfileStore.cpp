#include "platform/win32/ProfileStore.h"

#include "platform/win32/ConfigStore.h"
#include "platform/win32/ConfigTextInternal.h"
#include "platform/win32/Win32String.h"

#include <algorithm>
#include <cctype>
#include <cwctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ovtr::win32 {
namespace {

bool isReservedWindowsDeviceName(std::wstring name)
{
    const std::size_t dot = name.find(L'.');
    if (dot != std::wstring::npos) {
        name.resize(dot);
    }
    for (wchar_t& ch : name) {
        ch = static_cast<wchar_t>(std::towupper(ch));
    }
    return name == L"CON" || name == L"PRN" || name == L"AUX" || name == L"NUL" ||
        name == L"COM1" || name == L"COM2" || name == L"COM3" || name == L"COM4" ||
        name == L"COM5" || name == L"COM6" || name == L"COM7" || name == L"COM8" ||
        name == L"COM9" || name == L"LPT1" || name == L"LPT2" || name == L"LPT3" ||
        name == L"LPT4" || name == L"LPT5" || name == L"LPT6" || name == L"LPT7" ||
        name == L"LPT8" || name == L"LPT9";
}

bool isInvalidWindowsFileNameChar(const wchar_t ch) noexcept
{
    return ch < 32 || ch == L'<' || ch == L'>' || ch == L':' || ch == L'"' ||
        ch == L'/' || ch == L'\\' || ch == L'|' || ch == L'?' || ch == L'*';
}

bool parseRequiredFloat(
    const std::unordered_map<std::string, std::string>& values,
    const std::string& key,
    float& outValue,
    std::string& error
)
{
    const auto found = values.find(key);
    if (found == values.end()) {
        error = "missing profile field: " + key;
        return false;
    }

    const std::string trimmed = trimAscii(found->second);
    char* end = nullptr;
    const float parsed = std::strtof(trimmed.c_str(), &end);
    if (end == trimmed.c_str()) {
        error = "invalid numeric profile field: " + key;
        return false;
    }
    while (*end != '\0' && std::isspace(static_cast<unsigned char>(*end)) != 0) {
        ++end;
    }
    if (*end != '\0' || !std::isfinite(parsed) || parsed < 0.0f) {
        error = "invalid numeric profile field: " + key;
        return false;
    }
    outValue = parsed;
    return true;
}

bool parseOptionalFloat(
    const std::unordered_map<std::string, std::string>& values,
    const std::string& key,
    float& value,
    std::string& error
)
{
    if (values.find(key) == values.end()) {
        return true;
    }
    return parseRequiredFloat(values, key, value, error);
}

bool isRequiredProfileMeasurement(const std::string& key) noexcept
{
    return key != "neck_length_cm" &&
        key != "ankle_height_cm" &&
        key != "toe_tip_height_cm" &&
        key != "arm_bend_degrees" &&
        key != "leg_bend_degrees";
}

} // namespace

std::filesystem::path profileDirectoryPath()
{
    return executableDirectoryPath() / "profile";
}

std::wstring sanitizedProfileFileStem(std::wstring name)
{
    name = trimWide(std::move(name));
    for (wchar_t& ch : name) {
        if (isInvalidWindowsFileNameChar(ch)) {
            ch = L'_';
        }
    }
    while (!name.empty() && (name.back() == L'.' || std::iswspace(name.back()))) {
        name.pop_back();
    }
    if (!name.empty() && isReservedWindowsDeviceName(name)) {
        name.insert(name.begin(), L'_');
    }
    return name;
}

std::filesystem::path profilePathForName(const std::wstring& name)
{
    return profileDirectoryPath() / (sanitizedProfileFileStem(name) + L".profile");
}

bool ensureProfileDirectory(std::string& error)
{
    std::error_code createError;
    const std::filesystem::path directory = profileDirectoryPath();
    std::filesystem::create_directories(directory, createError);
    if (createError) {
        error = "could not create " + directory.string() + ": " + createError.message();
        return false;
    }
    return true;
}

bool listSavedProfiles(std::vector<ProfileFileEntry>& outProfiles, std::string& error)
{
    outProfiles.clear();
    const std::filesystem::path directory = profileDirectoryPath();
    std::error_code existsError;
    if (!std::filesystem::exists(directory, existsError)) {
        if (existsError) {
            error = "could not inspect profile directory: " + existsError.message();
            return false;
        }
        return true;
    }
    if (!std::filesystem::is_directory(directory, existsError) || existsError) {
        error = "profile path is not a directory: " + directory.string();
        return false;
    }

    std::error_code iterateError;
    for (std::filesystem::directory_iterator it(directory, iterateError), end; it != end; it.increment(iterateError)) {
        if (iterateError) {
            error = "could not enumerate profile directory: " + iterateError.message();
            return false;
        }
        std::error_code fileError;
        if (!it->is_regular_file(fileError) || fileError || it->path().extension() != L".profile") {
            continue;
        }
        outProfiles.push_back(ProfileFileEntry{it->path().stem().wstring(), it->path()});
    }

    std::sort(
        outProfiles.begin(),
        outProfiles.end(),
        [](const ProfileFileEntry& left, const ProfileFileEntry& right) {
            return left.name < right.name;
        }
    );
    return true;
}

std::string serializeProfile(const BodyProfile& profile)
{
    std::ostringstream output;
    output << "# toyxyz_openvr_toolkit profile v5\n";
    output << "version=5\n";
    output << "name=" << narrow(profile.name) << "\n";
    output << std::setprecision(9);
    const auto& definitions = profileMeasurementDefinitions();
    for (int i = 0; i < kProfileMeasurementCount; ++i) {
        output << definitions[static_cast<std::size_t>(i)].key << "="
               << profile.measurements[static_cast<std::size_t>(i)] << "\n";
    }
    return output.str();
}

bool parseProfile(std::istream& input, BodyProfile& outProfile, std::string& error)
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
            error = "invalid profile line: " + line;
            return false;
        }
        values[assignment.key] = assignment.value;
    }

    const auto name = values.find("name");
    if (name == values.end() || trimAscii(name->second).empty()) {
        error = "missing profile field: name";
        return false;
    }

    BodyProfile parsed;
    parsed.name = widen(name->second);

    const auto& definitions = profileMeasurementDefinitions();
    for (int i = 0; i < kProfileMeasurementCount; ++i) {
        const auto index = static_cast<std::size_t>(i);
        const bool parsedField = isRequiredProfileMeasurement(definitions[index].key)
            ? parseRequiredFloat(values, definitions[index].key, parsed.measurements[index], error)
            : parseOptionalFloat(values, definitions[index].key, parsed.measurements[index], error);
        if (!parsedField) {
            return false;
        }
    }

    outProfile = std::move(parsed);
    return true;
}

bool saveProfileToPath(const BodyProfile& profile, const std::filesystem::path& path, std::string& error)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        error = "could not open profile for writing: " + path.string();
        return false;
    }
    output << serializeProfile(profile);
    if (!output) {
        error = "failed while writing profile: " + path.string();
        return false;
    }
    return true;
}

bool loadProfileFromPath(const std::filesystem::path& path, BodyProfile& outProfile, std::string& error)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "could not open profile: " + path.string();
        return false;
    }
    return parseProfile(input, outProfile, error);
}

} // namespace ovtr::win32
