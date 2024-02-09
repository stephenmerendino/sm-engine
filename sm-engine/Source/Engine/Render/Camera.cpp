#include "Engine/Render/Camera.h"
#include "Engine/Input/InputSystem.h"

Camera::Camera()
	:m_worldPos(Vec3::ZERO)
	,m_worldYawDeg(0.0f)
	,m_worldPitchDeg(0.0f)
{
}

void Camera::Update(F32 ds)
{
	Vec3 forward = GetForward();
	Vec3 right = GetRight();
	Vec3 up = GetUp();

	F32 moveSpeed = 3.0f; // meters per second
	if (g_inputSystem.IsKeyDown(KeyCode::KEY_SHIFT))
	{
		moveSpeed *= 3.0f;
	}

	Vec3 movement = Vec3::ZERO;

	// Position
	if (g_inputSystem.IsKeyDown(KeyCode::KEY_W))
	{
		movement += forward * moveSpeed * ds;
	}

	if (g_inputSystem.IsKeyDown(KeyCode::KEY_S))
	{
		movement += -forward * moveSpeed * ds;
	}

	if (g_inputSystem.IsKeyDown(KeyCode::KEY_D))
	{
		movement += right * moveSpeed * ds;
	}

	if (g_inputSystem.IsKeyDown(KeyCode::KEY_A))
	{
		movement += -right * moveSpeed * ds;
	}

	if (g_inputSystem.IsKeyDown(KeyCode::KEY_E))
	{
		movement += up * moveSpeed * ds;
	}

	if (g_inputSystem.IsKeyDown(KeyCode::KEY_Q))
	{
		movement += -up * moveSpeed * ds;
	}

	m_worldPos += movement;

	// Rotation
	const F32 rotateSpeed = 10000.0f;
	Vec2 mouseMovement = g_inputSystem.GetMouseMovement();
	if (mouseMovement.x != 0.0f)
	{
		F32 rotationDeg = -mouseMovement.x * rotateSpeed * ds;
		m_worldYawDeg += rotationDeg;
	}

	if (mouseMovement.y != 0.0f)
	{
		F32 rotationDeg = mouseMovement.y * rotateSpeed * ds;
		m_worldPitchDeg += rotationDeg;
		m_worldPitchDeg = Clamp(m_worldPitchDeg, -89.9f, 89.9f);
	}
}

Vec3 Camera::GetForward() const
{
	return GetRotation().i.xyz;
}

Vec3 Camera::GetRight() const
{
	return -GetRotation().j.xyz;
}

Vec3 Camera::GetUp() const
{
	return GetRotation().k.xyz;
}

Mat44 Camera::GetRotation() const
{
	Mat44 pitch = Mat44::CreateRotationYDegs(m_worldPitchDeg);
	Mat44 yaw = Mat44::CreateRotationZDegs(m_worldYawDeg);

	return pitch * yaw;
}

Mat44 Camera::GetViewTransform() const
{
	// Transform from world space to camera space by swapping x=>z, y=>-x, z=>y
	// World is right handed z up, Camera is left handed Y up / z forward
	Mat44 changeOfBasis = Mat44(0.0f, 0.0f, 1.0f, 0.0f,
								-1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 1.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f);

	Mat44 cameraToWorld = GetRotation();
	Mat44 worldToCamera = cameraToWorld.Transposed();

	Vec3 worldTranslation = m_worldPos;
	Vec3 worldToCamera_translation = -1.0f * worldToCamera.TransformPoint(worldTranslation);

	Mat44 viewTransform = worldToCamera * changeOfBasis;
	Vec3 view_translation = changeOfBasis.TransformPoint(worldToCamera_translation);
	viewTransform.t.xyz = view_translation;

	return viewTransform;
}

void Camera::LookAt(const Vec3& lookAtPos, const Vec3& upRef)
{
	Vec3 viewDir = lookAtPos - m_worldPos;
	viewDir.Normalize();

	Vec3 viewUp = upRef - (Dot(upRef, viewDir) * viewDir);
	viewUp.Normalize();

	Vec3 viewDirXY = Vec3(viewDir.x, viewDir.y, 0.0f);
	viewDirXY.Normalize();

	F32 cosYawRads = Dot(viewDirXY, Vec3::FORWARD);
	F32 cosPitchRads = Dot(viewDir, upRef);

	//#TODO(smerendino): Is there a more elegant way to get the correctly signed angle
	F32 angleSign = Dot(viewDirXY, Vec3::LEFT) > 0.0f ? 1.0f : -1.0f;
	F32 yawRads = acosf(cosYawRads) * angleSign;
	F32 pitchRads = acosf(cosPitchRads);

	m_worldYawDeg = RadToDeg(yawRads);

	F32 pitchDeg = RadToDeg(pitchRads);
	m_worldPitchDeg = Remap(pitchDeg, 0.0f, 180.0f, -90.0f, 90.0f); // remap angle between view dir and up to relative to xy plane
}