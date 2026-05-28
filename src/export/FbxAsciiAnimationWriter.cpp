#include "export/FbxAsciiWriter.h"

#include "export/FbxAsciiFormatting.h"
#include "export/FbxAsciiMath.h"

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace ovtr {

void writeFbxAnimationStack(
    std::ostream& out,
    const std::int64_t stackId,
    const FbxTimelineSettings& timeline
)
{
    out << "    AnimationStack: " << stackId << ", \"AnimStack::Recorded Motion\", \"\" {\n";
    out << "        Properties70:  {\n";
    out << "            P: \"LocalStart\", \"KTime\", \"Time\", \"\"," << timeline.startTime << "\n";
    out << "            P: \"LocalStop\", \"KTime\", \"Time\", \"\"," << timeline.stopTime << "\n";
    out << "            P: \"ReferenceStart\", \"KTime\", \"Time\", \"\"," << timeline.startTime << "\n";
    out << "            P: \"ReferenceStop\", \"KTime\", \"Time\", \"\"," << timeline.stopTime << "\n";
    out << "        }\n";
    out << "    }\n";
}

void writeFbxCurve(
    std::ostream& out,
    const std::int64_t id,
    const std::string& name,
    const std::vector<FbxPoseKey>& keys,
    const bool rotation,
    const int axis
)
{
    std::vector<std::int64_t> times;
    std::vector<double> values;
    times.reserve(keys.size());
    values.reserve(keys.size());
    for (const FbxPoseKey& key : keys) {
        times.push_back(fbxSecondsToTicks(key.timeSeconds));
        values.push_back(rotation ? key.rotationDegrees[axis] : key.translation[axis]);
    }

    out << "    AnimationCurve: " << id << ", \"AnimCurve::" << name << "\", \"\" {\n";
    out << "        Default: 0\n";
    out << "        KeyVer: 4008\n";
    out << "        KeyTime: *" << times.size() << " {\n";
    out << "            a: " << joinedInt64(times) << "\n";
    out << "        }\n";
    out << "        KeyValueFloat: *" << values.size() << " {\n";
    out << "            a: " << joinedDoubles(values) << "\n";
    out << "        }\n";
    out << "        KeyAttrFlags: *1 {\n";
    out << "            a: 24836\n";
    out << "        }\n";
    out << "        KeyAttrDataFloat: *4 {\n";
    out << "            a: 0,0,9.999999747e-006,0\n";
    out << "        }\n";
    out << "        KeyAttrRefCount: *1 {\n";
    out << "            a: " << values.size() << "\n";
    out << "        }\n";
    out << "    }\n";
}

void writeFbxCurveNode(
    std::ostream& out,
    const std::int64_t id,
    const std::string& name,
    const bool rotation
)
{
    out << "    AnimationCurveNode: " << id << ", \"AnimCurveNode::" << name << "\", \"\" {\n";
    out << "        Properties70:  {\n";
    out << "            P: \"d|X\", \"Number\", \"\", \"A\",0\n";
    out << "            P: \"d|Y\", \"Number\", \"\", \"A\",0\n";
    out << "            P: \"d|Z\", \"Number\", \"\", \"A\",0\n";
    out << "        }\n";
    out << "        Channel: \"" << (rotation ? "R" : "T") << "\"\n";
    out << "    }\n";
}

} // namespace ovtr
