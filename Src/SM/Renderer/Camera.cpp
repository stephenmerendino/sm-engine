#include "SM/Renderer/Camera.h"

using namespace SM;

Vec3 Camera::GetForward() const
{
    return GetRotation().GetJBasis();
}

Vec3 Camera::GetRight() const
{
    return GetRotation().GetIBasis();
}

Vec3 Camera::GetUp() const
{
    return GetRotation().GetKBasis();
}

Mat44 Camera::GetRotation() const
{
	Mat44 pitch = Mat44::CreateRotationYDegs(m_worldPitchDegrees);
	Mat44 yaw = Mat44::CreateRotationZDegs(m_worldYawDegrees);

	return pitch * yaw;
}

Mat44 Camera::GetViewTransform() const
{
    Mat44 view;
	// Transform from world space to camera space by swapping y=>z, x=>x, z=>y
	// World is right handed z up, Camera is left handed Y up / z forward
	Mat44 changeOfBasis = Mat44(1.0f, 0.0f, 0.0f, 0.0f,
							    0.0f, 0.0f, 1.0f, 0.0f,
							    0.0f, 1.0f, 0.0f, 0.0f,
							    0.0f, 0.0f, 0.0f, 1.0f);

	Mat44 cameraToWorld = GetRotation();
	Mat44 worldToCamera = cameraToWorld.GetTransposed();

	Vec3 worldTranslation = m_worldPos;
	Vec3 worldToCameraTranslation = worldToCamera.TransformPoint(-1.0f * worldTranslation);

	view = worldToCamera * changeOfBasis;
	Vec3 viewTranslation = changeOfBasis.TransformPoint(worldToCameraTranslation);
    view.SetTranslation(viewTranslation);

	return view;
}

Mat44 Camera::GetProjectionTransform() const
{
    return m_projection;
}

Mat44 Camera::GetViewProjectionTransform() const
{
    return GetViewTransform() * GetProjectionTransform();
}

void Camera::LookAt(const Vec3& lookAtPosition, const Vec3& upReference)
{
	Vec3 viewDir = lookAtPosition - m_worldPos;
    viewDir.Normalize();

	Vec3 viewUp = upReference - (Dot(upReference, viewDir) * viewDir);
    viewUp.Normalize();

	Vec3 viewDirXY = Vec3(viewDir.x, viewDir.y, 0.0f);
    viewDirXY.Normalize();

	F32 cosYawRads = Dot(viewDirXY, Vec3::kWorldForward);
	F32 cosPitchRads = Dot(viewDir, upReference);

	//#TODO(smerendino): Is there a more elegant way to get the correctly signed angle
	F32 angleSign = Dot(viewDirXY, Vec3::kWorldLeft) > 0.0f ? 1.0f : -1.0f;
	F32 yawRads = acosf(cosYawRads) * angleSign;
	F32 pitchRads = acosf(cosPitchRads);

	m_worldYawDegrees = RadToDeg(yawRads);

	F32 pitchDeg = RadToDeg(pitchRads);
	m_worldPitchDegrees = Remap(pitchDeg, 0.0f, 180.0f, -90.0f, 90.0f); // remap angle between view dir and up to relative to xy plane
}
