#pragma once

#include "data/SessionTypes.h"
#include "platform/win32/MappingCalibrationModel.h"

namespace ovtr::win32 {

Vec3 addMappingVec3(Vec3 left, Vec3 right) noexcept;
Vec3 subMappingVec3(Vec3 left, Vec3 right) noexcept;
Vec3 scaleMappingVec3(Vec3 value, float scale) noexcept;
float dotMappingVec3(Vec3 left, Vec3 right) noexcept;
float lengthMappingVec3(Vec3 value) noexcept;
float distanceMappingVec3(Vec3 left, Vec3 right) noexcept;
Vec3 normalizeMappingVec3Or(Vec3 value, Vec3 fallback) noexcept;

MappingTransform identityMappingTransform() noexcept;
MappingTransform mappingTransformFromPose(const ovtr::PoseSample& pose) noexcept;
MappingTransform composeMappingTransforms(
    const MappingTransform& left,
    const MappingTransform& right
) noexcept;
MappingTransform inverseMappingTransform(const MappingTransform& transform) noexcept;
Vec3 transformMappingPoint(const MappingTransform& transform, Vec3 point) noexcept;
Vec3 rotateMappingVector(const MappingTransform& transform, Vec3 vector) noexcept;

} // namespace ovtr::win32
