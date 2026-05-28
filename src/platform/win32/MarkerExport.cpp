#include "platform/win32/MarkerExport.h"

#include "export/RenderModelGeometry.h"
#include "util/Identifier.h"

#include <utility>

namespace ovtr::win32 {
namespace {

constexpr std::uint32_t kMarkerRuntimeIndexBase = 0xF0000000u;

ovtr::DeviceDescriptor markerDescriptor(const SceneMarker& marker)
{
    ovtr::DeviceDescriptor descriptor;
    descriptor.id = kMarkerRuntimeIndexBase + marker.id;
    descriptor.runtimeIndex = kMarkerRuntimeIndexBase + marker.id;
    descriptor.serial = marker.name;
    descriptor.displayName = marker.name;
    descriptor.role = "marker";
    descriptor.deviceClass = ovtr::DeviceClass::Other;
    descriptor.modelName = "Marker Box";
    descriptor.recordEnabled = true;
    return descriptor;
}

} // namespace

std::vector<ovtr::ExportStaticPoseTrack> makeMarkerStaticExportTracks(const AppMarkerState& markerState)
{
    std::vector<ovtr::ExportStaticPoseTrack> tracks;
    tracks.reserve(markerState.markers.size());
    for (const SceneMarker& marker : markerState.markers) {
        ovtr::ExportStaticPoseTrack track;
        track.device = markerDescriptor(marker);
        track.nodeName = ovtr::sanitizeIdentifier(marker.name);
        track.translation = marker.position;
        track.rotation = marker.rotation;
        track.geometry = ovtr::makeBoxRenderModelGeometry(marker.sizeMeters);
        tracks.push_back(std::move(track));
    }
    return tracks;
}

} // namespace ovtr::win32
