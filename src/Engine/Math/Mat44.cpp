#include <Engine/Math/Mat44.h>
#include <Engine/Math/Mat33.h>
#include <Engine/Math/MathUtils.h>

Mat44 Mat44::IDENTITY = Mat44(1.0f, 0.0f, 0.0f, 0.0f,
							  0.0f, 1.0f, 0.0f, 0.0f,
							  0.0f, 0.0f, 1.0f, 0.0f,
							  0.0f, 0.0f, 0.0f, 1.0f);

Mat44& Mat44::operator*=(const Mat44& other)
{
	Mat44 result;

	result.m_ix = (m_ix * other.m_ix) + (m_iy * other.m_jx) + (m_iz * other.m_kx) + (m_iw * other.m_tx);
	result.m_iy = (m_ix * other.m_iy) + (m_iy * other.m_jy) + (m_iz * other.m_ky) + (m_iw * other.m_ty);
	result.m_iz = (m_ix * other.m_iz) + (m_iy * other.m_jz) + (m_iz * other.m_kz) + (m_iw * other.m_tz);
	result.m_iw = (m_ix * other.m_iw) + (m_iy * other.m_jw) + (m_iz * other.m_kw) + (m_iw * other.m_tw);

	result.m_jx = (m_jx * other.m_ix) + (m_jy * other.m_jx) + (m_jz * other.m_kx) + (m_jw * other.m_tx);
	result.m_jy = (m_jx * other.m_iy) + (m_jy * other.m_jy) + (m_jz * other.m_ky) + (m_jw * other.m_ty);
	result.m_jz = (m_jx * other.m_iz) + (m_jy * other.m_jz) + (m_jz * other.m_kz) + (m_jw * other.m_tz);
	result.m_jw = (m_jx * other.m_iw) + (m_jy * other.m_jw) + (m_jz * other.m_kw) + (m_jw * other.m_tw);

	result.m_kx = (m_kx * other.m_ix) + (m_ky * other.m_jx) + (m_kz * other.m_kx) + (m_kw * other.m_tx);
	result.m_ky = (m_kx * other.m_iy) + (m_ky * other.m_jy) + (m_kz * other.m_ky) + (m_kw * other.m_ty);
	result.m_kz = (m_kx * other.m_iz) + (m_ky * other.m_jz) + (m_kz * other.m_kz) + (m_kw * other.m_tz);
	result.m_kw = (m_kx * other.m_iw) + (m_ky * other.m_jw) + (m_kz * other.m_kw) + (m_kw * other.m_tw);

	result.m_tx = (m_tx * other.m_ix) + (m_ty * other.m_jx) + (m_tz * other.m_kx) + (m_tw * other.m_tx);
	result.m_ty = (m_tx * other.m_iy) + (m_ty * other.m_jy) + (m_tz * other.m_ky) + (m_tw * other.m_ty);
	result.m_tz = (m_tx * other.m_iz) + (m_ty * other.m_jz) + (m_tz * other.m_kz) + (m_tw * other.m_tz);
	result.m_tw = (m_tx * other.m_iw) + (m_ty * other.m_jw) + (m_tz * other.m_kw) + (m_tw * other.m_tw);

	*this = result;
	return *this;
}

void Mat44::Transpose()
{
	Swap(m_data[1], m_data[4]);
	Swap(m_data[2], m_data[8]);
	Swap(m_data[3], m_data[12]);
	Swap(m_data[6], m_data[9]);
	Swap(m_data[7], m_data[13]);
	Swap(m_data[11], m_data[14]);
}

F32 Mat44::CalcDeterminant() const 
{ 
	return m_ix * CalcCofactor(0, 0) +
		   m_iy * CalcCofactor(0, 1) +
		   m_iz * CalcCofactor(0, 2) +
		   m_iw * CalcCofactor(0, 3);
}

F32 Mat44::CalcCofactor(U32 row, U32 column) const
{
	Mat33 submatrix;
	
	// Build Mat33 submatrix by removing the row and column we are getting cofactor for
	U32 currentIndex = 0;
	for (U32 srcRow = 0; srcRow < 4; ++srcRow)
	{
		if (srcRow == row)
		{
			continue;
		}

		for (U32 srcCol = 0; srcCol < 4; ++srcCol)
		{
			if (srcCol == column)
			{
				continue;
			}

			submatrix.SetElement(currentIndex, GetElement(srcRow, srcCol));
			currentIndex++;
		}
	}

	// Calculate determinant of submatrix
	F32 subDet = submatrix.CalcDeterminant();

	// Use correct sign based on row and column
	if ((row + column) % 2 != 0)
	{
		subDet = -subDet;
	}
	
	return subDet;
}

void Mat44::Inverse() 
{
	// Cramer's method of inverse determinant multiplied by adjoint matrix
	F32 invDeterminant = 1.0f / CalcDeterminant();

	// Build matrix of cofactors
	Mat44 cofactorMatrix;
	for (U32 row = 0; row < 4; ++row)
	{
		for (U32 col = 0; col < 4; ++col)
		{
			cofactorMatrix.SetElement(row, col, CalcCofactor(row, col));
		}
	}

	// Turn cofactor matrix into adjoint matrix by transposing
	cofactorMatrix.Transpose();

	*this = cofactorMatrix * invDeterminant;
};

// Inverses rotation and translation
void Mat44::FastOrthogonalInverse()
{
	// Transpose upper 3x3 matrix
	Swap(m_data[1], m_data[4]);
	Swap(m_data[2], m_data[8]);
	Swap(m_data[6], m_data[9]);

	// Negate translation
	m_t.m_xyz = -m_t.m_xyz;
}

void Mat44::Translate(const Vec3& t)
{
	SetTranslation(m_t.m_xyz + t);
}

void Mat44::SetTranslation(const Vec3& t)
{
	m_t = Vec4(t, 1.0f);
}

Vec3 Mat44::GetTranslation() const
{
	return m_t.m_xyz;
}

Mat44 Mat44::MakeTranslation(const Vec3& t)
{
	Mat44 translation = Mat44::IDENTITY;
	translation.Translate(t);
	return translation;
}

void Mat44::RotateXDeg(F32 xDegrees)
{
	RotateXRad(DegreesToRadians(xDegrees));
}

void Mat44::RotateYDeg(F32 yDegrees)
{
	RotateYRad(DegreesToRadians(yDegrees));
}

void Mat44::RotateZDeg(F32 zDegrees)
{
	RotateZRad(DegreesToRadians(zDegrees));
}

void Mat44::RotateXRad(F32 xRadians)
{
	F32 cosRads = cosf(xRadians);
	F32 sinRads = sinf(xRadians);

	Mat44 xRot = Mat44(1.0f, 0.0f, 0.0f, 0.0f,
					   0.0f, cosRads, sinRads, 0.0f,
					   0.0f, -sinRads, cosRads, 0.0f,
					   0.0f, 0.0f, 0.0f, 1.0f);

	*this *= xRot;
}

void Mat44::RotateYRad(F32 yRadians)
{
	F32 cosRads = cosf(yRadians);
	F32 sinRads = sinf(yRadians);

	Mat44 yRot = Mat44(cosRads, 0.0f, -sinRads, 0.0f,
					   0.0f, 1.0f, 0.0f, 0.0f,
					   sinRads, 0.0f, cosRads, 0.0f,
					   0.0f, 0.0f, 0.0f, 1.0f);

	*this *= yRot;
}

void Mat44::RotateZRad(F32 zRadians)
{
	F32 cosRads = cosf(zRadians);
	F32 sinRads = sinf(zRadians);

	Mat44 zRot = Mat44(cosRads, sinRads, 0.0f, 0.0f,
					   -sinRads, cosRads, 0.0f, 0.0f,
					   0.0f, 0.0f, 1.0f, 0.0f,
					   0.0f, 0.0f, 0.0f, 1.0f);

	*this *= zRot;
}

Mat44 Mat44::MakeRotationXDeg(F32 xDegrees)
{
	Mat44 rotation = Mat44::IDENTITY;
	rotation.RotateXDeg(xDegrees);
	return rotation;
}

Mat44 Mat44::MakeRotationYDeg(F32 yDegrees)
{
	Mat44 rotation = Mat44::IDENTITY;
	rotation.RotateYDeg(yDegrees);
	return rotation;
}

Mat44 Mat44::MakeRotationZDeg(F32 zDegrees)
{
	Mat44 rotation = Mat44::IDENTITY;
	rotation.RotateZDeg(zDegrees);
	return rotation;
}

Mat44 Mat44::MakeRotationXRad(F32 xRadians)
{
	Mat44 rotation = Mat44::IDENTITY;
	rotation.RotateXRad(xRadians);
	return rotation;
}

Mat44 Mat44::MakeRotationYRad(F32 yRadians)
{
	Mat44 rotation = Mat44::IDENTITY;
	rotation.RotateYRad(yRadians);
	return rotation;
}

Mat44 Mat44::MakeRotationZRad(F32 zRadians)
{
	Mat44 rotation = Mat44::IDENTITY;
	rotation.RotateZRad(zRadians);
	return rotation;
}

Mat44 Mat44::GetRotation()
{
	Mat44 rotationOnly = Mat44::IDENTITY;
	rotationOnly.SetIBasis(this->GetIBasis());
	rotationOnly.SetJBasis(this->GetJBasis());
	rotationOnly.SetKBasis(this->GetKBasis());
	return rotationOnly;
}

void Mat44::SetRotation(const Mat44& rotation)
{
	SetIBasis(rotation.GetIBasis());
	SetJBasis(rotation.GetJBasis());
	SetKBasis(rotation.GetKBasis());
}

void Mat44::Scale(F32 uniformScale)
{
	m_i.m_xyz *= uniformScale;
	m_j.m_xyz *= uniformScale;
	m_k.m_xyz *= uniformScale;
}

void Mat44::Scale(F32 iScale, F32 jScale, F32 kScale)
{
	m_i.m_x *= iScale;
	m_i.m_y *= jScale;
	m_i.m_z *= kScale;

	m_j.m_x *= iScale;
	m_j.m_y *= jScale;
	m_j.m_z *= kScale;

	m_k.m_x *= iScale;
	m_k.m_y *= jScale;
	m_k.m_z *= kScale;
}

void Mat44::Scale(const Vec3& scaleFactors)
{
	m_i.m_x *= scaleFactors.m_x;
	m_i.m_y *= scaleFactors.m_y;
	m_i.m_z *= scaleFactors.m_z;

	m_j.m_x *= scaleFactors.m_x;
	m_j.m_y *= scaleFactors.m_y;
	m_j.m_z *= scaleFactors.m_z;

	m_k.m_x *= scaleFactors.m_x;
	m_k.m_y *= scaleFactors.m_y;
	m_k.m_z *= scaleFactors.m_z;
}

void Mat44::SetIBasis(const Vec3& iBasis)
{
	m_i = Vec4(iBasis, 0.0f);
}

void Mat44::SetJBasis(const Vec3& jBasis)
{
	m_j = Vec4(jBasis, 0.0f);
}

void Mat44::SetKBasis(const Vec3& kBasis)
{
	m_k = Vec4(kBasis, 0.0f);
}

Vec3 Mat44::GetIBasis() const
{
	return m_i.m_xyz;
}

Vec3 Mat44::GetJBasis() const
{
	return m_j.m_xyz;
}

Vec3 Mat44::GetKBasis() const
{
	return m_k.m_xyz;
}

Vec3 Mat44::TransformVector(const Vec3& v) const
{
	Vec4 res = Vec4(v, 0.0f) * (*this);
	return res.m_xyz;
}

Vec3 Mat44::TransformPoint(const Vec3& p) const
{
	Vec4 res = Vec4(p, 1.0f) * (*this);
	return res.m_xyz;
}

Vec4 operator*(const Vec4& v, const Mat44& mat)
{
	Vec4 result;
	result.m_x = (v.m_x * mat.m_ix) + (v.m_y * mat.m_jx) + (v.m_z * mat.m_kx) + (v.m_w * mat.m_tx);
	result.m_y = (v.m_x * mat.m_iy) + (v.m_y * mat.m_jy) + (v.m_z * mat.m_ky) + (v.m_w * mat.m_ty);
	result.m_z = (v.m_x * mat.m_iz) + (v.m_y * mat.m_jz) + (v.m_z * mat.m_kz) + (v.m_w * mat.m_tz);
	result.m_w = (v.m_x * mat.m_iw) + (v.m_y * mat.m_jw) + (v.m_z * mat.m_kw) + (v.m_w * mat.m_tw);
	return result;
}

Mat44 CreatePerspectiveProjection(F32 verticalFovDegrees, F32 nearPlane, F32 farPlane, F32 aspect)
{
	F32 focalLength = 1.0f / TanDegrees(verticalFovDegrees * 0.5f);

	return Mat44(focalLength / aspect,	0.0f,			0.0f,								0.0f,
				 0.0f,					-focalLength,	0.0f,								0.0f,
				 0.0f,					0.0f,			farPlane / (farPlane - nearPlane),	1.0f,
				 0.0f,					0.0f,			-nearPlane * farPlane / (farPlane - nearPlane),	0.0f);
}