#pragma once

#include "SM/Math.h"

namespace SM
{
    class Camera
    {
        public:
        Vec3 GetForward() const;
        Vec3 GetRight() const;
        Vec3 GetUp() const;
        Mat44 GetRotation() const;
        Mat44 GetViewTransform() const;
        Mat44 GetProjectionTransform() const;
        Mat44 GetViewProjectionTransform() const;

        void LookAt(const Vec3& lookAtPosition, const Vec3& upReference = Vec3::kZAxis);

        Vec3 m_worldPos = Vec3::kZero;
        F32	m_worldYawDegrees = 0.0f;
        F32	m_worldPitchDegrees = 0.0f;

        Mat44 m_projection;
    };

	inline Mat44 MakePerspectiveProjection(F32 verticalFovDeg, F32 n, F32 f, F32 aspect)
    {
        F32 focalLen = 1.0f / TanDeg(verticalFovDeg * 0.5f);
        return Mat44(focalLen / aspect, 0.0f, 0.0f, 0.0f,
                     0.0f, -focalLen, 0.0f, 0.0f,
                     0.0f, 0.0f, f / (f - n), 1.0f,
                     0.0f, 0.0f, -n * f / (f - n), 0.0f);
    }

	inline Mat44 MakeOrthographicProjection(F32 verticalFovDeg, F32 n, F32 f, F32 aspect)
    {
        return Mat44::kIdentity;
    }
}
