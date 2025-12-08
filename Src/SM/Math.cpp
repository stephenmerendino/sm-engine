#include "SM/Math.h"

using namespace SM;

const Vec2 Vec2::kZero(0.0f, 0.0f);
const Vec3 Vec3::kZero(0.0, 0.0f, 0.0f);
const Vec3 Vec3::kXAxis(1.0f, 0.0f, 0.0f);
const Vec3 Vec3::kYAxis(0.0f, 1.0f, 0.0f);
const Vec3 Vec3::kZAxis(0.0f, 0.0f, 1.0f);
const Vec3 Vec3::kWorldForward = Vec3::kYAxis;
const Vec3 Vec3::kWorldBackward = -Vec3::kWorldForward;
const Vec3 Vec3::kWorldUp = Vec3::kZAxis;
const Vec3 Vec3::kWorldDown = -Vec3::kWorldUp;
const Vec3 Vec3::kWorldLeft = -Vec3::kXAxis;
const Vec3 Vec3::kWorldRight = -Vec3::kWorldLeft;
const Vec4 Vec4::kZero(0.0f, 0.0f, 0.0f, 0.0f);
const IVec2 IVec2::kZero(0, 0);
const IVec3 IVec3::kZero(0, 0, 0);
const Mat33 Mat33::kIdentity;
const Mat44 Mat44::kIdentity;

// Based on https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/ 
// #TODO(smerendino): Implement ULP into this as well based on the source text
bool SM::IsCloseEnough(F32 a, F32 b, F32 acceptableError)
{
	// relative epsilon comparison
	F32 diff = fabsf(a - b);
	F32 absA = fabsf(a);
	F32 absB = fabsf(b);

	// #TODO(smerendino): Implement a get_largest() template func that takes in variable amount of args and returns biggest
	// compare to larger of two values
	F32 largest = absA > absB ? absA : absB;

	if (diff <= largest * acceptableError)
		return true;

	return false;
};

static F32 CalcCofactor(const Mat44& m, U32 row, U32 column)
{
    Mat33 submatrix;

    // Build Mat33 submatrix by removing the row and column we are getting cofactor for
    U32 curRow = 0;
    for (U32 srcRow = 0; srcRow < 4; ++srcRow)
    {
        if (srcRow == row)
            continue;

        U32 curCol = 0;
        for (U32 srcCol = 0; srcCol < 4; ++srcCol)
        {
            if (srcCol == column)
                continue;

            submatrix[curRow][curCol] = m[curRow][srcCol];
            curCol++;
        }

        curRow++;
    }

    // Calculate determinant of submatrix
    F32 subDeterminant = submatrix.Determinant();

    // Use correct sign based on row and column
    if ((row + column) % 2 != 0)
    {
        subDeterminant = -subDeterminant;
    }

    return subDeterminant;
}

F32 Mat44::Determinant() const
{
    return ix * CalcCofactor(*this, 0, 0) +
           iy * CalcCofactor(*this, 0, 1) +
           iz * CalcCofactor(*this, 0, 2) +
           iw * CalcCofactor(*this, 0, 3);
}

void Mat44::Inverse()
{
    // Cramer's method of inverse determinant multiplied by adjoint matrix
    F32 invDet = 1.0f / Determinant();

    // Build matrix of cofactors
    Mat44 cofactorMat;
    for (U32 row = 0; row < 4; ++row)
    {
        for (U32 col = 0; col < 4; ++col)
        {
            cofactorMat[row][col] = CalcCofactor(*this, row, col);
        }
    }

    // Turn cofactor matrix into adjoint matrix by transposing
    cofactorMat.Transpose();

    *this = cofactorMat * invDet;
}
