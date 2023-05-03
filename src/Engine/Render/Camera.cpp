#include "Engine/Render/Camera.h"
#include "Engine/Input/InputSystem.h"

Camera::Camera()
	:m_worldPosition(Vec3::ZERO)
	,m_worldYawDeg(0.0f)
	,m_worldPitchDeg(0.0f)
{
}

void Camera::Update(float ds)
{
	Vec3 forward = GetForward();
	Vec3 right = GetRight();
	Vec3 up = GetUp();

	const F32 kMovementSpeed = 3.0f; // meters per second

	// Position
	if (g_inputSystem.IsKeyDown(KeyCode::KEY_W))
	{
		Vec3 movement = forward * kMovementSpeed * ds;
		m_worldPosition += movement;
	}

	if (g_inputSystem.IsKeyDown(KeyCode::KEY_S))
	{
		Vec3 movement = -forward * kMovementSpeed * ds;
		m_worldPosition += movement;
	}

	if (g_inputSystem.IsKeyDown(KeyCode::KEY_D))
	{
		Vec3 movement = right * kMovementSpeed * ds;
		m_worldPosition += movement;
	}

	if (g_inputSystem.IsKeyDown(KeyCode::KEY_A))
	{
		Vec3 movement = -right * kMovementSpeed * ds;
		m_worldPosition += movement;
	}

	if (g_inputSystem.IsKeyDown(KeyCode::KEY_E))
	{ 
		Vec3 movement = up * kMovementSpeed * ds;
		m_worldPosition += movement;
	}

	if (g_inputSystem.IsKeyDown(KeyCode::KEY_Q))
	{
		Vec3 movement = -up * kMovementSpeed * ds;
		m_worldPosition += movement;
	}

	// Rotation
	const F32 kMouseRotationSpeed = 5000.0f;
	Vec2 mouseMovement = g_inputSystem.GetMouseMovement();
	if (mouseMovement.m_x != 0.0f)
	{
		F32 rotationDeg = -mouseMovement.m_x * kMouseRotationSpeed * ds;		
		m_worldYawDeg += rotationDeg;
	}

	if (mouseMovement.m_y != 0.0f)
	{
		F32 rotationDeg = mouseMovement.m_y * kMouseRotationSpeed * ds;
		m_worldPitchDeg += rotationDeg;
		m_worldPitchDeg = Clamp(m_worldPitchDeg, -89.9f, 89.9f);
	}
}

void Camera::SetPosition(const Vec3& pos)
{
	m_worldPosition = pos;
}

Vec3 Camera::GetPosition() const
{
	return m_worldPosition;
}

Vec3 Camera::GetForward() const
{
	return GetRotation().GetIBasis();
}

Vec3 Camera::GetRight() const
{
	return -GetRotation().GetJBasis();
}

Vec3 Camera::GetUp() const
{
	return GetRotation().GetKBasis();
}

Mat44 Camera::GetRotation() const
{
	Mat44 pitchRotation = Mat44::MakeRotationYDeg(m_worldPitchDeg);
	Mat44 yawRotation = Mat44::MakeRotationZDeg(m_worldYawDeg);

	return pitchRotation * yawRotation;
}

void Camera::LookAt(const Vec3& lookAtPos, const Vec3& upReference)
{
	Vec3 viewDir = lookAtPos - GetPosition();
	viewDir.Normalize();

	Vec3 viewUp = upReference - (Dot(upReference, viewDir) * viewDir);
	viewUp.Normalize();

	Vec3 viewDirXY = Vec3(viewDir.m_x, viewDir.m_y, 0.0f);
	viewDirXY.Normalize();

	F32 cosYawRads = Dot(viewDirXY, Vec3::FORWARD);
	F32 cosPitchRads = Dot(viewDir, upReference);

	//#TODO Is there a more elegant way to get the correctly signed angle
	F32 angleSign = Dot(viewDirXY, Vec3::LEFT) > 0.0f ? 1.0f : -1.0f;
	F32 yawRads = acosf(cosYawRads) * angleSign;
	F32 pitchRads = acosf(cosPitchRads);

	m_worldYawDeg = RadiansToDegrees(yawRads);
	
	F32 pitchDeg = RadiansToDegrees(pitchRads);
	m_worldPitchDeg = Remap(pitchDeg, 0.0f, 180.0f, -90.0f, 90.0f); // remap angle between view dir and up to relative to xy plane
}

Mat44 Camera::GetViewTransform()
{
	// Transform from world space to camera space by swapping x=>z, y=>-x, z=>y
	// World is right handed z up, Camera is left handed Y up / z forward
	Mat44 changeOfBasis = Mat44(0.0f, 0.0f, 1.0f, 0.0f,
								-1.0f, 0.0f, 0.0f, 0.0f,
								0.0f, 1.0f, 0.0f, 0.0f,
								0.0f, 0.0f, 0.0f, 1.0f);

	Mat44 cameraToWorldRotation = GetRotation();
	Vec3 worldTranslation = GetPosition();

	Mat44 worldToCameraRotation = cameraToWorldRotation.GetTransposed();

	Vec3 worldToCameraTranslation = -1.0f * worldToCameraRotation.TransformPoint(worldTranslation);
	
	Mat44 viewTransform = worldToCameraRotation * changeOfBasis;
	Vec3 viewTranslation = changeOfBasis.TransformPoint(worldToCameraTranslation);
	viewTransform.SetTranslation(viewTranslation);

	return viewTransform;
}

