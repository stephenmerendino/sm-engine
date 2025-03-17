#include "sm/math/mat44.h"

using namespace sm;

const mat44_t mat44_t::IDENTITY;

inline static f32 calc_cofactor(const mat44_t& m, u32 row, u32 column)
{
    mat33_t submatrix;

    // Build Mat33 submatrix by removing the row and column we are getting cofactor for
    u32 cur_row = 0;
    for (u32 src_row = 0; src_row < 4; ++src_row)
    {
        if (src_row == row)
            continue;

        u32 cur_col = 0;
        for (u32 src_col = 0; src_col < 4; ++src_col)
        {
            if (src_col == column)
                continue;

            submatrix[cur_row][cur_col] = m[cur_row][src_col];
            cur_col++;
        }

        cur_row++;
    }

    // Calculate determinant of submatrix
    f32 sub_determinant = determinant(submatrix);

    // Use correct sign based on row and column
    if ((row + column) % 2 != 0)
    {
        sub_determinant = -sub_determinant;
    }

    return sub_determinant;
}

f32 sm::determinant(const mat44_t& m)
{
    return m.ix * calc_cofactor(m, 0, 0) +
           m.iy * calc_cofactor(m, 0, 1) +
           m.iz * calc_cofactor(m, 0, 2) +
           m.iw * calc_cofactor(m, 0, 3);
}

inline void sm::inverse(mat44_t& m)
{
    // Cramer's method of inverse determinant multiplied by adjoint matrix
    f32 inv_det = 1.0f / determinant(m);

    // Build matrix of cofactors
    mat44_t cofactor_mat;
    for (u32 row = 0; row < 4; ++row)
    {
        for (u32 col = 0; col < 4; ++col)
        {
            cofactor_mat[row][col] = calc_cofactor(m, row, col);
        }
    }

    // Turn cofactor matrix into adjoint matrix by transposing
    transpose(cofactor_mat);

    m = cofactor_mat * inv_det;
}

