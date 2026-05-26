#include "import/GltfImporter.h"

#include "math/QuaternionUtils.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace ovtr {

namespace {

constexpr std::uint32_t kGlbMagic = 0x46546c67u;
constexpr std::uint32_t kGlbVersion = 2u;
constexpr std::uint32_t kGlbJsonChunkType = 0x4e4f534au;
constexpr std::uint32_t kGlbBinChunkType = 0x004e4942u;

constexpr int kGltfComponentByte = 5120;
constexpr int kGltfComponentUnsignedByte = 5121;
constexpr int kGltfComponentShort = 5122;
constexpr int kGltfComponentUnsignedShort = 5123;
constexpr int kGltfComponentUnsignedInt = 5125;
constexpr int kGltfComponentFloat = 5126;

struct JsonValue {
    enum class Type {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object,
    };

    Type type = Type::Null;
    bool boolValue = false;
    double numberValue = 0.0;
    std::string stringValue;
    std::vector<JsonValue> arrayValue;
    std::vector<std::pair<std::string, JsonValue>> objectValue;

    const JsonValue* find(const char* key) const
    {
        if (type != Type::Object || key == nullptr) {
            return nullptr;
        }
        for (const auto& entry : objectValue) {
            if (entry.first == key) {
                return &entry.second;
            }
        }
        return nullptr;
    }
};

class JsonParser {
public:
    explicit JsonParser(const std::string& text)
        : text_(text)
    {
    }

    bool parse(JsonValue& value)
    {
        skipWhitespace();
        if (!parseValue(value)) {
            return false;
        }
        skipWhitespace();
        if (position_ != text_.size()) {
            setError("unexpected trailing JSON data");
            return false;
        }
        return true;
    }

    const std::string& error() const
    {
        return error_;
    }

private:
    const std::string& text_;
    std::size_t position_ = 0;
    std::string error_;

    void setError(const std::string& message)
    {
        if (error_.empty()) {
            std::ostringstream stream;
            stream << message << " at byte " << position_;
            error_ = stream.str();
        }
    }

    void skipWhitespace()
    {
        while (position_ < text_.size()) {
            const unsigned char ch = static_cast<unsigned char>(text_[position_]);
            if (ch != ' ' && ch != '\n' && ch != '\r' && ch != '\t') {
                break;
            }
            ++position_;
        }
    }

    bool consume(const char expected)
    {
        if (position_ >= text_.size() || text_[position_] != expected) {
            std::string message = "expected '";
            message.push_back(expected);
            message.push_back('\'');
            setError(message);
            return false;
        }
        ++position_;
        return true;
    }

    bool consumeLiteral(const char* literal)
    {
        const std::size_t start = position_;
        while (*literal != '\0') {
            if (position_ >= text_.size() || text_[position_] != *literal) {
                position_ = start;
                setError("invalid JSON literal");
                return false;
            }
            ++position_;
            ++literal;
        }
        return true;
    }

    bool parseValue(JsonValue& value)
    {
        skipWhitespace();
        if (position_ >= text_.size()) {
            setError("unexpected end of JSON");
            return false;
        }

        const char ch = text_[position_];
        if (ch == '{') {
            return parseObject(value);
        }
        if (ch == '[') {
            return parseArray(value);
        }
        if (ch == '"') {
            value.type = JsonValue::Type::String;
            return parseString(value.stringValue);
        }
        if (ch == '-' || (ch >= '0' && ch <= '9')) {
            value.type = JsonValue::Type::Number;
            return parseNumber(value.numberValue);
        }
        if (ch == 't') {
            if (!consumeLiteral("true")) {
                return false;
            }
            value.type = JsonValue::Type::Bool;
            value.boolValue = true;
            return true;
        }
        if (ch == 'f') {
            if (!consumeLiteral("false")) {
                return false;
            }
            value.type = JsonValue::Type::Bool;
            value.boolValue = false;
            return true;
        }
        if (ch == 'n') {
            if (!consumeLiteral("null")) {
                return false;
            }
            value.type = JsonValue::Type::Null;
            return true;
        }

        setError("unexpected JSON token");
        return false;
    }

    bool parseObject(JsonValue& value)
    {
        if (!consume('{')) {
            return false;
        }
        value.type = JsonValue::Type::Object;
        value.objectValue.clear();

        skipWhitespace();
        if (position_ < text_.size() && text_[position_] == '}') {
            ++position_;
            return true;
        }

        while (true) {
            skipWhitespace();
            if (position_ >= text_.size() || text_[position_] != '"') {
                setError("expected JSON object key");
                return false;
            }

            std::string key;
            if (!parseString(key)) {
                return false;
            }

            skipWhitespace();
            if (!consume(':')) {
                return false;
            }

            JsonValue child;
            if (!parseValue(child)) {
                return false;
            }
            value.objectValue.emplace_back(std::move(key), std::move(child));

            skipWhitespace();
            if (position_ < text_.size() && text_[position_] == ',') {
                ++position_;
                continue;
            }
            if (position_ < text_.size() && text_[position_] == '}') {
                ++position_;
                return true;
            }

            setError("expected ',' or '}'");
            return false;
        }
    }

    bool parseArray(JsonValue& value)
    {
        if (!consume('[')) {
            return false;
        }
        value.type = JsonValue::Type::Array;
        value.arrayValue.clear();

        skipWhitespace();
        if (position_ < text_.size() && text_[position_] == ']') {
            ++position_;
            return true;
        }

        while (true) {
            JsonValue child;
            if (!parseValue(child)) {
                return false;
            }
            value.arrayValue.push_back(std::move(child));

            skipWhitespace();
            if (position_ < text_.size() && text_[position_] == ',') {
                ++position_;
                continue;
            }
            if (position_ < text_.size() && text_[position_] == ']') {
                ++position_;
                return true;
            }

            setError("expected ',' or ']'");
            return false;
        }
    }

    static int hexValue(const char ch)
    {
        if (ch >= '0' && ch <= '9') {
            return ch - '0';
        }
        if (ch >= 'a' && ch <= 'f') {
            return ch - 'a' + 10;
        }
        if (ch >= 'A' && ch <= 'F') {
            return ch - 'A' + 10;
        }
        return -1;
    }

    bool parseHex4(std::uint32_t& value)
    {
        if (position_ + 4 > text_.size()) {
            setError("incomplete JSON unicode escape");
            return false;
        }

        value = 0;
        for (int i = 0; i < 4; ++i) {
            const int digit = hexValue(text_[position_ + static_cast<std::size_t>(i)]);
            if (digit < 0) {
                setError("invalid JSON unicode escape");
                return false;
            }
            value = (value << 4u) | static_cast<std::uint32_t>(digit);
        }
        position_ += 4;
        return true;
    }

    static void appendUtf8(std::string& out, const std::uint32_t codePoint)
    {
        if (codePoint <= 0x7fu) {
            out.push_back(static_cast<char>(codePoint));
        } else if (codePoint <= 0x7ffu) {
            out.push_back(static_cast<char>(0xc0u | (codePoint >> 6u)));
            out.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
        } else if (codePoint <= 0xffffu) {
            out.push_back(static_cast<char>(0xe0u | (codePoint >> 12u)));
            out.push_back(static_cast<char>(0x80u | ((codePoint >> 6u) & 0x3fu)));
            out.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
        } else {
            out.push_back(static_cast<char>(0xf0u | (codePoint >> 18u)));
            out.push_back(static_cast<char>(0x80u | ((codePoint >> 12u) & 0x3fu)));
            out.push_back(static_cast<char>(0x80u | ((codePoint >> 6u) & 0x3fu)));
            out.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
        }
    }

    bool parseString(std::string& output)
    {
        if (!consume('"')) {
            return false;
        }

        output.clear();
        while (position_ < text_.size()) {
            const unsigned char raw = static_cast<unsigned char>(text_[position_++]);
            if (raw == '"') {
                return true;
            }
            if (raw < 0x20u) {
                setError("control character in JSON string");
                return false;
            }
            if (raw != '\\') {
                output.push_back(static_cast<char>(raw));
                continue;
            }

            if (position_ >= text_.size()) {
                setError("incomplete JSON string escape");
                return false;
            }
            const char escaped = text_[position_++];
            switch (escaped) {
            case '"':
            case '\\':
            case '/':
                output.push_back(escaped);
                break;
            case 'b':
                output.push_back('\b');
                break;
            case 'f':
                output.push_back('\f');
                break;
            case 'n':
                output.push_back('\n');
                break;
            case 'r':
                output.push_back('\r');
                break;
            case 't':
                output.push_back('\t');
                break;
            case 'u': {
                std::uint32_t codePoint = 0;
                if (!parseHex4(codePoint)) {
                    return false;
                }
                appendUtf8(output, codePoint);
                break;
            }
            default:
                setError("unsupported JSON string escape");
                return false;
            }
        }

        setError("unterminated JSON string");
        return false;
    }

    bool parseNumber(double& output)
    {
        const char* start = text_.c_str() + position_;
        char* end = nullptr;
        errno = 0;
        output = std::strtod(start, &end);
        if (end == start || errno == ERANGE || !std::isfinite(output)) {
            setError("invalid JSON number");
            return false;
        }
        position_ = static_cast<std::size_t>(end - text_.c_str());
        return true;
    }
};

struct BufferView {
    int buffer = 0;
    std::size_t byteOffset = 0;
    std::size_t byteLength = 0;
    std::size_t byteStride = 0;
};

struct Accessor {
    int bufferView = -1;
    std::size_t byteOffset = 0;
    int componentType = 0;
    int count = 0;
    std::string type;
};

struct GlbPayload {
    std::string json;
    std::vector<std::uint8_t> binary;
};

struct NodeAnimationValue {
    bool hasTranslation = false;
    bool hasRotation = false;
    std::array<float, 3> translation{0.0f, 0.0f, 0.0f};
    std::array<float, 4> rotation{0.0f, 0.0f, 0.0f, 1.0f};
};

using NodeAnimationBuilder = std::map<double, NodeAnimationValue>;

std::vector<std::uint8_t> readBinaryFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return {};
    }
    return std::vector<std::uint8_t>(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    );
}

std::uint32_t readLittleEndianUint32(const std::vector<std::uint8_t>& bytes, const std::size_t offset)
{
    if (offset + 4 > bytes.size()) {
        return 0;
    }
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8u) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16u) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24u);
}

bool readGlbPayload(const std::filesystem::path& path, GlbPayload& payload, std::string& error)
{
    const std::vector<std::uint8_t> bytes = readBinaryFile(path);
    if (bytes.empty()) {
        error = "could not read GLB file";
        return false;
    }
    if (bytes.size() < 20) {
        error = "GLB file is too small";
        return false;
    }
    if (readLittleEndianUint32(bytes, 0) != kGlbMagic) {
        error = "file is not a GLB";
        return false;
    }
    if (readLittleEndianUint32(bytes, 4) != kGlbVersion) {
        error = "unsupported GLB version";
        return false;
    }

    const std::uint32_t declaredLength = readLittleEndianUint32(bytes, 8);
    if (declaredLength > bytes.size()) {
        error = "GLB declared length exceeds file size";
        return false;
    }

    std::size_t offset = 12;
    while (offset + 8 <= bytes.size() && offset < declaredLength) {
        const std::uint32_t chunkLength = readLittleEndianUint32(bytes, offset);
        const std::uint32_t chunkType = readLittleEndianUint32(bytes, offset + 4);
        offset += 8;
        if (offset + chunkLength > bytes.size()) {
            error = "GLB chunk length exceeds file size";
            return false;
        }

        if (chunkType == kGlbJsonChunkType) {
            payload.json.assign(
                reinterpret_cast<const char*>(bytes.data() + offset),
                reinterpret_cast<const char*>(bytes.data() + offset + chunkLength)
            );
        } else if (chunkType == kGlbBinChunkType) {
            payload.binary.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                bytes.begin() + static_cast<std::ptrdiff_t>(offset + chunkLength));
        }

        offset += chunkLength;
    }

    if (payload.json.empty()) {
        error = "GLB does not contain a JSON chunk";
        return false;
    }
    if (payload.binary.empty()) {
        error = "GLB does not contain an embedded binary chunk";
        return false;
    }
    return true;
}

bool jsonNumber(const JsonValue& value, double& output)
{
    if (value.type != JsonValue::Type::Number) {
        return false;
    }
    output = value.numberValue;
    return true;
}

int jsonIntMember(const JsonValue& object, const char* key, const int defaultValue = -1)
{
    const JsonValue* value = object.find(key);
    if (!value || value->type != JsonValue::Type::Number) {
        return defaultValue;
    }
    return static_cast<int>(value->numberValue);
}

std::size_t jsonSizeMember(const JsonValue& object, const char* key, const std::size_t defaultValue = 0)
{
    const JsonValue* value = object.find(key);
    if (!value || value->type != JsonValue::Type::Number || value->numberValue < 0.0) {
        return defaultValue;
    }
    return static_cast<std::size_t>(value->numberValue);
}

std::string jsonStringMember(const JsonValue& object, const char* key, std::string defaultValue = {})
{
    const JsonValue* value = object.find(key);
    if (!value || value->type != JsonValue::Type::String) {
        return defaultValue;
    }
    return value->stringValue;
}

bool readJsonFloatArray(
    const JsonValue& object,
    const char* key,
    float* output,
    const std::size_t expectedCount
)
{
    const JsonValue* value = object.find(key);
    if (!value || value->type != JsonValue::Type::Array || value->arrayValue.size() < expectedCount) {
        return false;
    }

    for (std::size_t i = 0; i < expectedCount; ++i) {
        double number = 0.0;
        if (!jsonNumber(value->arrayValue[i], number)) {
            return false;
        }
        output[i] = static_cast<float>(number);
    }
    return true;
}

int componentCountForType(const std::string& type)
{
    if (type == "SCALAR") {
        return 1;
    }
    if (type == "VEC2") {
        return 2;
    }
    if (type == "VEC3") {
        return 3;
    }
    if (type == "VEC4") {
        return 4;
    }
    return 0;
}

std::size_t componentByteSize(const int componentType)
{
    switch (componentType) {
    case kGltfComponentByte:
    case kGltfComponentUnsignedByte:
        return 1;
    case kGltfComponentShort:
    case kGltfComponentUnsignedShort:
        return 2;
    case kGltfComponentUnsignedInt:
    case kGltfComponentFloat:
        return 4;
    default:
        return 0;
    }
}

bool parseBufferViews(const JsonValue& root, std::vector<BufferView>& bufferViews, std::string& error)
{
    const JsonValue* bufferViewsValue = root.find("bufferViews");
    if (!bufferViewsValue || bufferViewsValue->type != JsonValue::Type::Array) {
        error = "glTF bufferViews are missing";
        return false;
    }

    bufferViews.clear();
    bufferViews.reserve(bufferViewsValue->arrayValue.size());
    for (const JsonValue& value : bufferViewsValue->arrayValue) {
        if (value.type != JsonValue::Type::Object) {
            error = "glTF bufferView must be an object";
            return false;
        }
        BufferView view;
        view.buffer = jsonIntMember(value, "buffer", 0);
        view.byteOffset = jsonSizeMember(value, "byteOffset", 0);
        view.byteLength = jsonSizeMember(value, "byteLength", 0);
        view.byteStride = jsonSizeMember(value, "byteStride", 0);
        if (view.buffer != 0) {
            error = "only embedded GLB buffer 0 is supported";
            return false;
        }
        bufferViews.push_back(view);
    }
    return true;
}

bool parseAccessors(const JsonValue& root, std::vector<Accessor>& accessors, std::string& error)
{
    const JsonValue* accessorsValue = root.find("accessors");
    if (!accessorsValue || accessorsValue->type != JsonValue::Type::Array) {
        error = "glTF accessors are missing";
        return false;
    }

    accessors.clear();
    accessors.reserve(accessorsValue->arrayValue.size());
    for (const JsonValue& value : accessorsValue->arrayValue) {
        if (value.type != JsonValue::Type::Object) {
            error = "glTF accessor must be an object";
            return false;
        }

        Accessor accessor;
        accessor.bufferView = jsonIntMember(value, "bufferView", -1);
        accessor.byteOffset = jsonSizeMember(value, "byteOffset", 0);
        accessor.componentType = jsonIntMember(value, "componentType", 0);
        accessor.count = jsonIntMember(value, "count", 0);
        accessor.type = jsonStringMember(value, "type");
        if (accessor.bufferView < 0 || accessor.count < 0 || componentCountForType(accessor.type) <= 0 ||
            componentByteSize(accessor.componentType) == 0) {
            error = "glTF accessor has unsupported layout";
            return false;
        }
        accessors.push_back(std::move(accessor));
    }
    return true;
}

bool accessorByteLayout(
    const std::vector<BufferView>& bufferViews,
    const std::vector<Accessor>& accessors,
    const int accessorIndex,
    const int expectedComponentCount,
    const std::vector<std::uint8_t>& binary,
    const BufferView*& view,
    const Accessor*& accessor,
    std::size_t& baseOffset,
    std::size_t& stride,
    std::size_t& componentSize,
    std::string& error
)
{
    if (accessorIndex < 0 || static_cast<std::size_t>(accessorIndex) >= accessors.size()) {
        error = "glTF accessor index is out of range";
        return false;
    }

    accessor = &accessors[static_cast<std::size_t>(accessorIndex)];
    if (accessor->bufferView < 0 || static_cast<std::size_t>(accessor->bufferView) >= bufferViews.size()) {
        error = "glTF accessor bufferView is out of range";
        return false;
    }
    view = &bufferViews[static_cast<std::size_t>(accessor->bufferView)];

    const int componentCount = componentCountForType(accessor->type);
    if (componentCount != expectedComponentCount) {
        error = "glTF accessor type does not match expected data";
        return false;
    }

    componentSize = componentByteSize(accessor->componentType);
    stride = view->byteStride != 0
        ? view->byteStride
        : componentSize * static_cast<std::size_t>(componentCount);
    baseOffset = view->byteOffset + accessor->byteOffset;
    if (accessor->count == 0) {
        return true;
    }

    const std::size_t lastElementOffset = baseOffset + stride * static_cast<std::size_t>(accessor->count - 1);
    const std::size_t requiredEnd =
        lastElementOffset + componentSize * static_cast<std::size_t>(componentCount);
    const std::size_t viewEnd = view->byteOffset + view->byteLength;
    if (requiredEnd > binary.size() || requiredEnd > viewEnd) {
        error = "glTF accessor reads past its bufferView";
        return false;
    }
    return true;
}

float readFloatComponent(const std::vector<std::uint8_t>& binary, const std::size_t offset)
{
    float value = 0.0f;
    std::memcpy(&value, binary.data() + offset, sizeof(float));
    return value;
}

bool readAccessorFloats(
    const std::vector<BufferView>& bufferViews,
    const std::vector<Accessor>& accessors,
    const int accessorIndex,
    const int expectedComponentCount,
    const std::vector<std::uint8_t>& binary,
    std::vector<float>& output,
    std::string& error
)
{
    const BufferView* view = nullptr;
    const Accessor* accessor = nullptr;
    std::size_t baseOffset = 0;
    std::size_t stride = 0;
    std::size_t componentSize = 0;
    if (!accessorByteLayout(
            bufferViews,
            accessors,
            accessorIndex,
            expectedComponentCount,
            binary,
            view,
            accessor,
            baseOffset,
            stride,
            componentSize,
            error
        )) {
        return false;
    }
    (void)view;
    (void)componentSize;

    if (accessor->componentType != kGltfComponentFloat) {
        error = "only float accessors are supported for transforms and vertices";
        return false;
    }

    output.clear();
    output.reserve(static_cast<std::size_t>(accessor->count) * static_cast<std::size_t>(expectedComponentCount));
    for (int element = 0; element < accessor->count; ++element) {
        const std::size_t elementOffset = baseOffset + stride * static_cast<std::size_t>(element);
        for (int component = 0; component < expectedComponentCount; ++component) {
            output.push_back(readFloatComponent(
                binary,
                elementOffset + sizeof(float) * static_cast<std::size_t>(component)
            ));
        }
    }
    return true;
}

std::uint32_t readUnsignedComponent(
    const std::vector<std::uint8_t>& binary,
    const std::size_t offset,
    const int componentType
)
{
    if (componentType == kGltfComponentUnsignedByte) {
        return binary[offset];
    }
    if (componentType == kGltfComponentUnsignedShort) {
        return static_cast<std::uint32_t>(binary[offset]) |
            (static_cast<std::uint32_t>(binary[offset + 1]) << 8u);
    }
    if (componentType == kGltfComponentUnsignedInt) {
        return readLittleEndianUint32(binary, offset);
    }
    return 0;
}

bool readAccessorIndices(
    const std::vector<BufferView>& bufferViews,
    const std::vector<Accessor>& accessors,
    const int accessorIndex,
    const std::vector<std::uint8_t>& binary,
    std::vector<std::uint16_t>& output,
    std::string& error
)
{
    const BufferView* view = nullptr;
    const Accessor* accessor = nullptr;
    std::size_t baseOffset = 0;
    std::size_t stride = 0;
    std::size_t componentSize = 0;
    if (!accessorByteLayout(
            bufferViews,
            accessors,
            accessorIndex,
            1,
            binary,
            view,
            accessor,
            baseOffset,
            stride,
            componentSize,
            error
        )) {
        return false;
    }
    (void)view;
    (void)componentSize;

    if (accessor->componentType != kGltfComponentUnsignedByte &&
        accessor->componentType != kGltfComponentUnsignedShort &&
        accessor->componentType != kGltfComponentUnsignedInt) {
        error = "glTF mesh indices must be unsigned integer data";
        return false;
    }

    output.clear();
    output.reserve(static_cast<std::size_t>(accessor->count));
    for (int element = 0; element < accessor->count; ++element) {
        const std::uint32_t index = readUnsignedComponent(
            binary,
            baseOffset + stride * static_cast<std::size_t>(element),
            accessor->componentType
        );
        if (index > std::numeric_limits<std::uint16_t>::max()) {
            error = "mesh has more than 65535 vertices, which this viewer path does not support yet";
            return false;
        }
        output.push_back(static_cast<std::uint16_t>(index));
    }
    return true;
}

bool parseMeshes(
    const JsonValue& root,
    const std::vector<BufferView>& bufferViews,
    const std::vector<Accessor>& accessors,
    const std::vector<std::uint8_t>& binary,
    ImportedGltfScene& scene,
    std::string& error
)
{
    const JsonValue* meshesValue = root.find("meshes");
    if (!meshesValue || meshesValue->type != JsonValue::Type::Array) {
        return true;
    }

    scene.meshes.resize(meshesValue->arrayValue.size());
    for (std::size_t meshIndex = 0; meshIndex < meshesValue->arrayValue.size(); ++meshIndex) {
        const JsonValue& meshValue = meshesValue->arrayValue[meshIndex];
        if (meshValue.type != JsonValue::Type::Object) {
            error = "glTF mesh must be an object";
            return false;
        }

        const JsonValue* primitivesValue = meshValue.find("primitives");
        if (!primitivesValue || primitivesValue->type != JsonValue::Type::Array ||
            primitivesValue->arrayValue.empty()) {
            continue;
        }

        const JsonValue& primitive = primitivesValue->arrayValue.front();
        if (primitive.type != JsonValue::Type::Object) {
            error = "glTF mesh primitive must be an object";
            return false;
        }

        const JsonValue* attributes = primitive.find("attributes");
        if (!attributes || attributes->type != JsonValue::Type::Object) {
            continue;
        }

        const int positionAccessor = jsonIntMember(*attributes, "POSITION", -1);
        const int normalAccessor = jsonIntMember(*attributes, "NORMAL", -1);
        const int indexAccessor = jsonIntMember(primitive, "indices", -1);
        if (positionAccessor < 0 || indexAccessor < 0) {
            continue;
        }

        std::vector<float> positions;
        if (!readAccessorFloats(bufferViews, accessors, positionAccessor, 3, binary, positions, error)) {
            return false;
        }
        std::vector<float> normals;
        if (normalAccessor >= 0 &&
            !readAccessorFloats(bufferViews, accessors, normalAccessor, 3, binary, normals, error)) {
            return false;
        }
        std::vector<std::uint16_t> indices;
        if (!readAccessorIndices(bufferViews, accessors, indexAccessor, binary, indices, error)) {
            return false;
        }

        const std::size_t vertexCount = positions.size() / 3;
        RenderModelGeometry geometry;
        geometry.vertices.reserve(vertexCount);
        for (std::size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
            RenderModelVertex vertex;
            vertex.position = {
                positions[vertexIndex * 3],
                positions[vertexIndex * 3 + 1],
                positions[vertexIndex * 3 + 2],
            };
            if (normals.size() >= vertexIndex * 3 + 3) {
                vertex.normal = {
                    normals[vertexIndex * 3],
                    normals[vertexIndex * 3 + 1],
                    normals[vertexIndex * 3 + 2],
                };
            }
            geometry.vertices.push_back(vertex);
        }
        geometry.indices = std::move(indices);
        geometry.available = !geometry.vertices.empty() && !geometry.indices.empty();
        scene.meshes[meshIndex] = std::move(geometry);
    }
    return true;
}

std::vector<ImportedGltfNode> parseNodes(const JsonValue& root)
{
    std::vector<ImportedGltfNode> nodes;
    const JsonValue* nodesValue = root.find("nodes");
    if (!nodesValue || nodesValue->type != JsonValue::Type::Array) {
        return nodes;
    }

    nodes.reserve(nodesValue->arrayValue.size());
    for (std::size_t nodeIndex = 0; nodeIndex < nodesValue->arrayValue.size(); ++nodeIndex) {
        ImportedGltfNode node;
        node.nodeIndex = static_cast<int>(nodeIndex);
        const JsonValue& nodeValue = nodesValue->arrayValue[nodeIndex];
        if (nodeValue.type == JsonValue::Type::Object) {
            node.name = jsonStringMember(nodeValue, "name");
            node.meshIndex = jsonIntMember(nodeValue, "mesh", -1);
            readJsonFloatArray(nodeValue, "translation", node.translation.data(), node.translation.size());
            readJsonFloatArray(nodeValue, "rotation", node.rotation.data(), node.rotation.size());
            readJsonFloatArray(nodeValue, "scale", node.scale.data(), node.scale.size());
            node.rotation = normalizeQuaternion(node.rotation);
        }
        nodes.push_back(std::move(node));
    }
    return nodes;
}

bool appendAnimationChannel(
    const JsonValue& channel,
    const JsonValue& samplers,
    const std::vector<BufferView>& bufferViews,
    const std::vector<Accessor>& accessors,
    const std::vector<std::uint8_t>& binary,
    std::vector<NodeAnimationBuilder>& builders,
    std::string& error
)
{
    if (channel.type != JsonValue::Type::Object) {
        error = "glTF animation channel must be an object";
        return false;
    }

    const JsonValue* target = channel.find("target");
    if (!target || target->type != JsonValue::Type::Object) {
        error = "glTF animation channel target is missing";
        return false;
    }

    const int nodeIndex = jsonIntMember(*target, "node", -1);
    const std::string path = jsonStringMember(*target, "path");
    const int samplerIndex = jsonIntMember(channel, "sampler", -1);
    if (nodeIndex < 0 || static_cast<std::size_t>(nodeIndex) >= builders.size() ||
        samplerIndex < 0 || static_cast<std::size_t>(samplerIndex) >= samplers.arrayValue.size()) {
        return true;
    }
    if (path != "translation" && path != "rotation") {
        return true;
    }

    const JsonValue& sampler = samplers.arrayValue[static_cast<std::size_t>(samplerIndex)];
    if (sampler.type != JsonValue::Type::Object) {
        error = "glTF animation sampler must be an object";
        return false;
    }

    const int inputAccessor = jsonIntMember(sampler, "input", -1);
    const int outputAccessor = jsonIntMember(sampler, "output", -1);
    std::vector<float> times;
    if (!readAccessorFloats(bufferViews, accessors, inputAccessor, 1, binary, times, error)) {
        return false;
    }

    const int componentCount = path == "rotation" ? 4 : 3;
    std::vector<float> values;
    if (!readAccessorFloats(bufferViews, accessors, outputAccessor, componentCount, binary, values, error)) {
        return false;
    }
    if (values.size() < times.size() * static_cast<std::size_t>(componentCount)) {
        error = "glTF animation sampler output has too few values";
        return false;
    }

    NodeAnimationBuilder& builder = builders[static_cast<std::size_t>(nodeIndex)];
    for (std::size_t keyIndex = 0; keyIndex < times.size(); ++keyIndex) {
        NodeAnimationValue& value = builder[static_cast<double>(times[keyIndex])];
        if (path == "translation") {
            value.hasTranslation = true;
            value.translation = {
                values[keyIndex * 3],
                values[keyIndex * 3 + 1],
                values[keyIndex * 3 + 2],
            };
        } else {
            value.hasRotation = true;
            value.rotation = normalizeQuaternion({
                values[keyIndex * 4],
                values[keyIndex * 4 + 1],
                values[keyIndex * 4 + 2],
                values[keyIndex * 4 + 3],
            });
        }
    }
    return true;
}

bool parseAnimations(
    const JsonValue& root,
    const std::vector<BufferView>& bufferViews,
    const std::vector<Accessor>& accessors,
    const std::vector<std::uint8_t>& binary,
    std::vector<ImportedGltfNode>& nodes,
    double& durationSeconds,
    std::string& error
)
{
    durationSeconds = 0.0;
    if (nodes.empty()) {
        return true;
    }

    std::vector<NodeAnimationBuilder> builders(nodes.size());
    const JsonValue* animations = root.find("animations");
    if (!animations || animations->type != JsonValue::Type::Array) {
        return true;
    }

    for (const JsonValue& animation : animations->arrayValue) {
        if (animation.type != JsonValue::Type::Object) {
            error = "glTF animation must be an object";
            return false;
        }
        const JsonValue* samplers = animation.find("samplers");
        const JsonValue* channels = animation.find("channels");
        if (!samplers || !channels ||
            samplers->type != JsonValue::Type::Array ||
            channels->type != JsonValue::Type::Array) {
            continue;
        }

        for (const JsonValue& channel : channels->arrayValue) {
            if (!appendAnimationChannel(
                    channel,
                    *samplers,
                    bufferViews,
                    accessors,
                    binary,
                    builders,
                    error
                )) {
                return false;
            }
        }
    }

    for (std::size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex) {
        ImportedGltfNode& node = nodes[nodeIndex];
        NodeAnimationBuilder& builder = builders[nodeIndex];
        if (builder.empty()) {
            continue;
        }

        std::array<float, 3> translation = node.translation;
        std::array<float, 4> rotation = node.rotation;
        node.keys.clear();
        node.keys.reserve(builder.size());
        for (const auto& entry : builder) {
            if (entry.second.hasTranslation) {
                translation = entry.second.translation;
            }
            if (entry.second.hasRotation) {
                rotation = entry.second.rotation;
            }

            node.keys.push_back(ImportedGltfKey{
                entry.first,
                translation,
                normalizeQuaternion(rotation),
            });
            durationSeconds = std::max(durationSeconds, entry.first);
        }
    }
    return true;
}

double quaternionDot(const std::array<float, 4>& left, const std::array<float, 4>& right)
{
    return static_cast<double>(left[0]) * right[0] +
        static_cast<double>(left[1]) * right[1] +
        static_cast<double>(left[2]) * right[2] +
        static_cast<double>(left[3]) * right[3];
}

std::array<float, 3> lerpVector(
    const std::array<float, 3>& from,
    const std::array<float, 3>& to,
    const double factor
)
{
    return {
        static_cast<float>(from[0] + (to[0] - from[0]) * factor),
        static_cast<float>(from[1] + (to[1] - from[1]) * factor),
        static_cast<float>(from[2] + (to[2] - from[2]) * factor),
    };
}

std::array<float, 4> slerpQuaternion(
    const std::array<float, 4>& from,
    const std::array<float, 4>& to,
    const double factor
)
{
    std::array<float, 4> start = normalizeQuaternion(from);
    std::array<float, 4> end = normalizeQuaternion(to);
    double dot = quaternionDot(start, end);
    if (dot < 0.0) {
        for (float& component : end) {
            component = -component;
        }
        dot = -dot;
    }

    if (dot > 0.9995) {
        std::array<float, 4> result{};
        for (int component = 0; component < 4; ++component) {
            result[static_cast<std::size_t>(component)] =
                static_cast<float>(start[static_cast<std::size_t>(component)] +
                    (end[static_cast<std::size_t>(component)] -
                        start[static_cast<std::size_t>(component)]) * factor);
        }
        return normalizeQuaternion(result);
    }

    dot = std::clamp(dot, -1.0, 1.0);
    const double theta = std::acos(dot);
    const double sinTheta = std::sin(theta);
    const double startWeight = std::sin((1.0 - factor) * theta) / sinTheta;
    const double endWeight = std::sin(factor * theta) / sinTheta;
    return normalizeQuaternion({
        static_cast<float>(start[0] * startWeight + end[0] * endWeight),
        static_cast<float>(start[1] * startWeight + end[1] * endWeight),
        static_cast<float>(start[2] * startWeight + end[2] * endWeight),
        static_cast<float>(start[3] * startWeight + end[3] * endWeight),
    });
}

} // namespace

GltfImportResult importGlbScene(const std::filesystem::path& inputPath)
{
    GltfImportResult result;
    result.inputPath = inputPath;

    GlbPayload payload;
    if (!readGlbPayload(inputPath, payload, result.error)) {
        return result;
    }

    JsonValue root;
    JsonParser parser(payload.json);
    if (!parser.parse(root)) {
        result.error = "failed to parse GLB JSON: " + parser.error();
        return result;
    }
    if (root.type != JsonValue::Type::Object) {
        result.error = "GLB JSON root must be an object";
        return result;
    }

    std::vector<BufferView> bufferViews;
    if (!parseBufferViews(root, bufferViews, result.error)) {
        return result;
    }
    std::vector<Accessor> accessors;
    if (!parseAccessors(root, accessors, result.error)) {
        return result;
    }

    ImportedGltfScene scene;
    scene.sourcePath = inputPath;
    scene.name = inputPath.stem().string();
    if (!parseMeshes(root, bufferViews, accessors, payload.binary, scene, result.error)) {
        return result;
    }

    std::vector<ImportedGltfNode> allNodes = parseNodes(root);
    if (!parseAnimations(
            root,
            bufferViews,
            accessors,
            payload.binary,
            allNodes,
            scene.durationSeconds,
            result.error
        )) {
        return result;
    }

    for (ImportedGltfNode& node : allNodes) {
        if (node.meshIndex >= 0 || !node.keys.empty()) {
            scene.nodes.push_back(std::move(node));
        }
    }

    if (scene.nodes.empty()) {
        result.error = "GLB contains no visible or animated nodes";
        return result;
    }

    result.scene = std::move(scene);
    result.success = true;
    return result;
}

bool sampleImportedGltfNodePose(
    const ImportedGltfNode& node,
    const double timeSeconds,
    std::array<float, 3>& translation,
    std::array<float, 4>& rotation
)
{
    if (node.keys.empty()) {
        translation = node.translation;
        rotation = node.rotation;
        return true;
    }

    if (timeSeconds <= node.keys.front().timeSeconds) {
        translation = node.keys.front().translation;
        rotation = node.keys.front().rotation;
        return true;
    }
    if (timeSeconds >= node.keys.back().timeSeconds) {
        translation = node.keys.back().translation;
        rotation = node.keys.back().rotation;
        return true;
    }

    auto upper = std::upper_bound(
        node.keys.begin(),
        node.keys.end(),
        timeSeconds,
        [](const double value, const ImportedGltfKey& key) {
            return value < key.timeSeconds;
        }
    );
    if (upper == node.keys.begin() || upper == node.keys.end()) {
        translation = node.keys.front().translation;
        rotation = node.keys.front().rotation;
        return true;
    }

    const ImportedGltfKey& to = *upper;
    const ImportedGltfKey& from = *(upper - 1);
    const double duration = to.timeSeconds - from.timeSeconds;
    const double factor = duration > 0.0
        ? std::clamp((timeSeconds - from.timeSeconds) / duration, 0.0, 1.0)
        : 0.0;

    translation = lerpVector(from.translation, to.translation, factor);
    rotation = slerpQuaternion(from.rotation, to.rotation, factor);
    return true;
}

} // namespace ovtr
