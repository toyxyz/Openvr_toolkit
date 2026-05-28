#include "export/FbxAsciiWriter.h"

#include "export/FbxAsciiFormatting.h"

#include <cstdint>
#include <ostream>

namespace ovtr {

void writeFbxHeader(
    std::ostream& out,
    const FbxCoordinatePolicy policy,
    const FbxTimelineSettings& timeline
)
{
    out << "; FBX 7.4.0 project file\n";
    out << "; Created by OpenVR Tracker Recorder\n";
    out << "FBXHeaderExtension:  {\n";
    out << "    FBXHeaderVersion: 1003\n";
    out << "    FBXVersion: 7400\n";
    out << "    Creator: \"OpenVR Tracker Recorder\"\n";
    out << "}\n";
    out << "GlobalSettings:  {\n";
    out << "    Version: 1000\n";
    out << "    Properties70:  {\n";
    if (policy == FbxCoordinatePolicy::Blender) {
        out << "        P: \"UpAxis\", \"int\", \"Integer\", \"\",2\n";
        out << "        P: \"UpAxisSign\", \"int\", \"Integer\", \"\",1\n";
        out << "        P: \"FrontAxis\", \"int\", \"Integer\", \"\",1\n";
        out << "        P: \"FrontAxisSign\", \"int\", \"Integer\", \"\",-1\n";
        out << "        P: \"CoordAxis\", \"int\", \"Integer\", \"\",0\n";
        out << "        P: \"CoordAxisSign\", \"int\", \"Integer\", \"\",1\n";
    } else {
        out << "        P: \"UpAxis\", \"int\", \"Integer\", \"\",1\n";
        out << "        P: \"UpAxisSign\", \"int\", \"Integer\", \"\",1\n";
        out << "        P: \"FrontAxis\", \"int\", \"Integer\", \"\",2\n";
        out << "        P: \"FrontAxisSign\", \"int\", \"Integer\", \"\",1\n";
        out << "        P: \"CoordAxis\", \"int\", \"Integer\", \"\",0\n";
        out << "        P: \"CoordAxisSign\", \"int\", \"Integer\", \"\",1\n";
    }
    out << "        P: \"UnitScaleFactor\", \"double\", \"Number\", \"\",1\n";
    out << "        P: \"OriginalUnitScaleFactor\", \"double\", \"Number\", \"\",1\n";
    out << "        P: \"AmbientColor\", \"ColorRGB\", \"Color\", \"\",0,0,0\n";
    out << "        P: \"DefaultCamera\", \"KString\", \"\", \"\", \"Producer Perspective\"\n";
    out << "        P: \"TimeMode\", \"enum\", \"\", \"\"," << timeline.timeMode << "\n";
    out << "        P: \"TimeProtocol\", \"enum\", \"\", \"\",2\n";
    out << "        P: \"SnapOnFrameMode\", \"enum\", \"\", \"\",0\n";
    out << "        P: \"TimeSpanStart\", \"KTime\", \"Time\", \"\"," << timeline.startTime << "\n";
    out << "        P: \"TimeSpanStop\", \"KTime\", \"Time\", \"\"," << timeline.stopTime << "\n";
    out << "        P: \"CustomFrameRate\", \"double\", \"Number\", \"\"," << timeline.customFrameRate << "\n";
    out << "        P: \"TimeMarker\", \"Compound\", \"\", \"\"\n";
    out << "        P: \"CurrentTimeMarker\", \"int\", \"Integer\", \"\",-1\n";
    out << "    }\n";
    out << "}\n";
}

void writeFbxModel(std::ostream& out, const FbxDeviceExport& device)
{
    const FbxPoseKey defaultKey = device.keys.empty() ? FbxPoseKey{} : device.keys.front();
    out << "    Model: " << device.modelId << ", \"Model::" << escapeFbxString(device.nodeName) << "\", \"Mesh\" {\n";
    out << "        Version: 232\n";
    out << "        Properties70:  {\n";
    out << "            P: \"Lcl Translation\", \"Lcl Translation\", \"\", \"A\","
        << defaultKey.translation[0] << "," << defaultKey.translation[1] << "," << defaultKey.translation[2] << "\n";
    out << "            P: \"Lcl Rotation\", \"Lcl Rotation\", \"\", \"A\","
        << defaultKey.rotationDegrees[0] << "," << defaultKey.rotationDegrees[1] << "," << defaultKey.rotationDegrees[2] << "\n";
    out << "            P: \"RotationOrder\", \"enum\", \"\", \"\",0\n";
    out << "        }\n";
    out << "        Shading: T\n";
    out << "        Culling: \"CullingOff\"\n";
    out << "    }\n";
}

void writeFbxRootModel(std::ostream& out, const std::int64_t rootModelId)
{
    out << "    Model: " << rootModelId << ", \"Model::OpenVR_Root\", \"Null\" {\n";
    out << "        Version: 232\n";
    out << "        Properties70:  {\n";
    out << "            P: \"Lcl Translation\", \"Lcl Translation\", \"\", \"A\",0,0,0\n";
    out << "            P: \"Lcl Rotation\", \"Lcl Rotation\", \"\", \"A\",0,0,0\n";
    out << "            P: \"Lcl Scaling\", \"Lcl Scaling\", \"\", \"A\",1,1,1\n";
    out << "        }\n";
    out << "        Shading: T\n";
    out << "        Culling: \"CullingOff\"\n";
    out << "    }\n";
}

} // namespace ovtr
