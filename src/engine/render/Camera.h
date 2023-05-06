#pragma once

#include <Engine/Math/Mat44.h>

class Camera
{
public:
	Vec3 m_worldPosition;
	F32 m_worldYawDeg;
	F32 m_worldPitchDeg;

	Camera();

	void Update(float ds);

	void SetPosition(const Vec3& pos);
	Vec3 GetPosition() const;

	Vec3 GetForward() const;
	Vec3 GetRight() const;
	Vec3 GetUp() const;
	Mat44 GetRotation() const;

	void LookAt(const Vec3& lookAtPos, const Vec3& upReference = Vec3::UP);
	Mat44 GetViewTransform();
};