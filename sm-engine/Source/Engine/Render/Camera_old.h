#pragma once

#include "Engine/Math/Vec3_old.h"
#include "Engine/Math/Mat44_old.h"

class Camera
{
public:
	Camera();

	void Update(F32 ds);
	Vec3 GetForward() const;
	Vec3 GetRight() const;
	Vec3 GetUp() const;
	Mat44 GetRotation() const;
	Mat44 GetViewTransform() const;
	void LookAt(const Vec3& lookAtPos, const Vec3& upRef = Vec3::UP);

	Vec3 m_worldPos;
	F32 m_worldYawDeg;
	F32 m_worldPitchDeg;
};
