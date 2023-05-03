#include "Engine/Math/Mat33.h"

Mat33 Mat33::IDENTITY = Mat33(1.0f, 0.0f, 0.0f,
							  0.0f, 1.0f, 0.0f,
							  0.0f, 0.0f, 1.0f);

Mat33& Mat33::operator*=(const Mat33& other)
{
	Mat33 result;

	result.m_ix = (m_ix * other.m_ix) + (m_iy * other.m_jx) + (m_iz * other.m_kx);
	result.m_iy = (m_ix * other.m_iy) + (m_iy * other.m_jy) + (m_iz * other.m_ky);
	result.m_iz = (m_ix * other.m_iz) + (m_iy * other.m_jz) + (m_iz * other.m_kz);

	result.m_jx = (m_jx * other.m_ix) + (m_jy * other.m_jx) + (m_jz * other.m_kx);
	result.m_jy = (m_jx * other.m_iy) + (m_jy * other.m_jy) + (m_jz * other.m_ky);
	result.m_jz = (m_jx * other.m_iz) + (m_jy * other.m_jz) + (m_jz * other.m_kz);

	result.m_kx = (m_kx * other.m_ix) + (m_ky * other.m_jx) + (m_kz * other.m_kx);
	result.m_ky = (m_kx * other.m_iy) + (m_ky * other.m_jy) + (m_kz * other.m_ky);
	result.m_kz = (m_kx * other.m_iz) + (m_ky * other.m_jz) + (m_kz * other.m_kz);

	*this = result;
	return *this;
}

void Mat33::Transpose()
{
	Swap(m_data[1], m_data[3]);
	Swap(m_data[2], m_data[6]);
	Swap(m_data[5], m_data[7]);
}

F32 Mat33::CalcDeterminant() const 
{ 
	return m_ix * (m_jy * m_kz - m_jz * m_ky) -
		   m_iy * (m_jx * m_kz - m_jz * m_kx) +
		   m_iz * (m_jx * m_ky - m_jy * m_kx);
}

Vec3 operator*(const Vec3& v, const Mat33& mat)
{
	Vec3 result;
	result.m_x = (v.m_x * mat.m_ix) + (v.m_y * mat.m_jx) + (v.m_z * mat.m_kx);
	result.m_y = (v.m_x * mat.m_iy) + (v.m_y * mat.m_jy) + (v.m_z * mat.m_ky);
	result.m_z = (v.m_x * mat.m_iz) + (v.m_y * mat.m_jz) + (v.m_z * mat.m_kz);
	return result;
}
