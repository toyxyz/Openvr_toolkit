#include "export/GltfJsonSections.h"

#include "util/JsonWriter.h"

#include <ostream>

namespace ovtr::detail {

void writeGltfJsonAnimation(
    std::ostream& out,
    const RecordingSession& session,
    const std::vector<GltfAnimationTarget>& animationTargets
)
{
    out << "  \"animations\": [\n";
    out << "    {\n";
    out << "      \"name\": \"" << escapeJsonString(session.sessionName.empty() ? session.sessionId : session.sessionName) << "\",\n";
    out << "      \"samplers\": [\n";
    int samplerIndex = 0;
    for (std::size_t targetIndex = 0; targetIndex < animationTargets.size(); ++targetIndex) {
        const GltfAnimationTarget& target = animationTargets[targetIndex];
        if (targetIndex != 0) {
            out << ",\n";
        }
        out << "        { \"input\": " << target.timeAccessor << ", \"output\": " << target.translationAccessor
            << ", \"interpolation\": \"LINEAR\" },\n";
        out << "        { \"input\": " << target.timeAccessor << ", \"output\": " << target.rotationAccessor
            << ", \"interpolation\": \"LINEAR\" }";
        samplerIndex += 2;
    }
    (void)samplerIndex;
    out << "\n";
    out << "      ],\n";
    out << "      \"channels\": [\n";
    samplerIndex = 0;
    for (std::size_t targetIndex = 0; targetIndex < animationTargets.size(); ++targetIndex) {
        const GltfAnimationTarget& target = animationTargets[targetIndex];
        if (targetIndex != 0) {
            out << ",\n";
        }
        out << "        { \"sampler\": " << samplerIndex << ", \"target\": { \"node\": " << target.node
            << ", \"path\": \"translation\" } },\n";
        out << "        { \"sampler\": " << (samplerIndex + 1) << ", \"target\": { \"node\": " << target.node
            << ", \"path\": \"rotation\" } }";
        samplerIndex += 2;
    }
    out << "\n";
    out << "      ]\n";
    out << "    }\n";
    out << "  ],\n";
}

} // namespace ovtr::detail
