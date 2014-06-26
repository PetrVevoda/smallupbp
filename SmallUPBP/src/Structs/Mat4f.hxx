/*
 * Copyright (C) 2014, Petr Vevoda, Martin Sik (http://cgg.mff.cuni.cz/~sik/), 
 * Tomas Davidovic (http://www.davidovic.cz), Iliyan Georgiev (http://www.iliyan.com/), 
 * Jaroslav Krivanek (http://cgg.mff.cuni.cz/~jaroslav/)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * (The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
 */

#ifndef __MAT4F_HXX__
#define __MAT4F_HXX__

#include <cmath>

#include "Pixel.hxx"
#include "Rgb.hxx"

// Class representing 4x4 matrix of floats.
class Mat4f
{
public:

    Mat4f(){}
    Mat4f(float a){ for(int i=0; i<16; i++) GetPtr()[i] = a; }

    const float* GetPtr() const { return reinterpret_cast<const float*>(this); }
    float*       GetPtr()       { return reinterpret_cast<float*>(this);       }

    const float& Get(int r, int c) const { return GetPtr()[r + c*4]; }
    float&       Get(int r, int c)       { return GetPtr()[r + c*4]; }

	// Sets row at the index r to (a, b, c, d).
    void SetRow(int r, float a, float b, float c, float d)
    {
        Get(r, 0) = a;
        Get(r, 1) = b;
        Get(r, 2) = c;
        Get(r, 3) = d;
    }

	// Sets row at the index r to (a.x(), a.y(), a.z(), b).
    void SetRow(int r, const Dir &a, float b)
    {
        for(int i=0; i<3; i++)
            Get(r, i) = a[i];

        Get(r, 3) = b;
    }

	// Transforms the given point. That is performs this_matrix * [aVec, 1]^T and homogeneous division.
    Pos TransformPoint(const Pos& aVec) const
    {
        float w = Get(3,3);

        for(int c=0; c<3; c++)
            w += Get(3, c) * aVec[c];

        const float invW = 1.f / w;

        Pos res(0);

        for(int r=0; r<3; r++)
        {
            res[r] = Get(r, 3);

            for(int c=0; c<3; c++)
                res[r] += aVec[c] * Get(r, c);

            res[r] *= invW;
        }
        return res;
    }

	// Zero matrix.
    static Mat4f Zero() { Mat4f res(0); return res; }

	// Identity matrix.
    static Mat4f Identity()
    {
        Mat4f res(0);
        for(int i=0; i<4; i++) res.Get(i,i) = 1.f;
        return res;
    }

	// Creates matrix representing scaling by the given scales.
    static Mat4f Scale(const Dir& aScale)
    {
        Mat4f res = Mat4f::Identity();
        for(int i=0; i<3; i++) res.Get(i,i) = aScale[i];
        res.Get(3,3) = 1;
        return res;
    }

	// Creates matrix representing translation by the given vector.
    static Mat4f Translate(const Dir& aTranslate)
    {
        Mat4f res = Mat4f::Identity();
        for(int i=0; i<3; i++) res.Get(i,3) = aTranslate[i];
        res.Get(3,3) = 1;
        return res;
    }

	// Creates matrix for perspective transformation done by a camera with the given parameters.
    static Mat4f Perspective(
        float aFov,
        float aNear,
        float aFar)
    {
        // Camera points towards -z.  0 < near < far.
        // Matrix maps z range [-near, -far] to [-1, 1], after homogeneous division.
        float f = 1.f / (std::tan(aFov * PI_F / 360.0f));
        float d = 1.f / (aNear - aFar);

        Mat4f r;
        r.m00 = f;    r.m01 = 0.0f; r.m02 = 0.0f;               r.m03 = 0.0f;
        r.m10 = 0.0f; r.m11 = -f;   r.m12 = 0.0f;               r.m13 = 0.0f;
        r.m20 = 0.0f; r.m21 = 0.0f; r.m22 = (aNear + aFar) * d; r.m23 = 2.0f * aNear * aFar * d;
        r.m30 = 0.0f; r.m31 = 0.0f; r.m32 = -1.0f;              r.m33 = 0.0f;

        return r;
    }
public:

    // m_row_col; stored column major
    float m00, m10, m20, m30;
    float m01, m11, m21, m31;
    float m02, m12, m22, m32;
    float m03, m13, m23, m33;
};

inline Mat4f operator*(const Mat4f& left, const Mat4f& right)
{
    Mat4f res(0);
    for(int row=0; row<4; row++)
        for(int col=0; col<4; col++)
            for(int i=0; i<4; i++)
                res.Get(row, col) += left.Get(row, i) * right.Get(i, col);

    return res;
}

// Inverts matrix. Code for inversion taken from:
// http://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix
inline Mat4f Invert(const Mat4f& aMatrix)
{
    const float *m = aMatrix.GetPtr();
    float inv[16], det;
    int i;

    inv[0] = m[5] * m[10] * m[15] -
        m[5]  * m[11] * m[14] -
        m[9]  * m[6]  * m[15] +
        m[9]  * m[7]  * m[14] +
        m[13] * m[6]  * m[11] -
        m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] +
        m[4]  * m[11] * m[14] +
        m[8]  * m[6]  * m[15] -
        m[8]  * m[7]  * m[14] -
        m[12] * m[6]  * m[11] +
        m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] -
        m[4]  * m[11] * m[13] -
        m[8]  * m[5] * m[15] +
        m[8]  * m[7] * m[13] +
        m[12] * m[5] * m[11] -
        m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] +
        m[4]  * m[10] * m[13] +
        m[8]  * m[5] * m[14] -
        m[8]  * m[6] * m[13] -
        m[12] * m[5] * m[10] +
        m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] +
        m[1]  * m[11] * m[14] +
        m[9]  * m[2] * m[15] -
        m[9]  * m[3] * m[14] -
        m[13] * m[2] * m[11] +
        m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] -
        m[0]  * m[11] * m[14] -
        m[8]  * m[2] * m[15] +
        m[8]  * m[3] * m[14] +
        m[12] * m[2] * m[11] -
        m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] +
        m[0]  * m[11] * m[13] +
        m[8]  * m[1] * m[15] -
        m[8]  * m[3] * m[13] -
        m[12] * m[1] * m[11] +
        m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] -
        m[0]  * m[10] * m[13] -
        m[8]  * m[1] * m[14] +
        m[8]  * m[2] * m[13] +
        m[12] * m[1] * m[10] -
        m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] -
        m[1]  * m[7] * m[14] -
        m[5]  * m[2] * m[15] +
        m[5]  * m[3] * m[14] +
        m[13] * m[2] * m[7] -
        m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] +
        m[0]  * m[7] * m[14] +
        m[4]  * m[2] * m[15] -
        m[4]  * m[3] * m[14] -
        m[12] * m[2] * m[7] +
        m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] -
        m[0]  * m[7] * m[13] -
        m[4]  * m[1] * m[15] +
        m[4]  * m[3] * m[13] +
        m[12] * m[1] * m[7] -
        m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] +
        m[0]  * m[6] * m[13] +
        m[4]  * m[1] * m[14] -
        m[4]  * m[2] * m[13] -
        m[12] * m[1] * m[6] +
        m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] +
        m[1] * m[7] * m[10] +
        m[5] * m[2] * m[11] -
        m[5] * m[3] * m[10] -
        m[9] * m[2] * m[7] +
        m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] -
        m[0] * m[7] * m[10] -
        m[4] * m[2] * m[11] +
        m[4] * m[3] * m[10] +
        m[8] * m[2] * m[7] -
        m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] +
        m[0] * m[7] * m[9] +
        m[4] * m[1] * m[11] -
        m[4] * m[3] * m[9] -
        m[8] * m[1] * m[7] +
        m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] -
        m[0] * m[6] * m[9] -
        m[4] * m[1] * m[10] +
        m[4] * m[2] * m[9] +
        m[8] * m[1] * m[6] -
        m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return Mat4f::Identity();

    det = 1.f / det;

    Mat4f res;
    for (i = 0; i < 16; i++)
        res.GetPtr()[i] = inv[i] * det;

    return res;
}

#endif //__MAT4F_HXX__