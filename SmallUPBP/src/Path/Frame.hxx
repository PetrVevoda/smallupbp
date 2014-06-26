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

#ifndef __FRAME_HXX__
#define __FRAME_HXX__

#include <vector>
#include <cmath>

#include "..\Structs\Vector3.hxx"

class Frame
{
public:

    Frame()
    {
        mX = Dir(1,0,0);
        mY = Dir(0,1,0);
        mZ = Dir(0,0,1);
    };

    Frame(
        const Dir& x,
        const Dir& y,
        const Dir& z
    ) :
        mX(x),
        mY(y),
        mZ(z)
    {}

    void SetFromZ(const Dir& z)
    {
		Dir tmpZ = mZ = z.getNormalized();
        Dir tmpX = (std::abs(tmpZ.x()) > 0.99f) ? Dir(0,1,0) : Dir(1,0,0);
		mY = cross(tmpZ, tmpX).getNormalized();
        mX = cross(mY, tmpZ);
    }

    Dir ToWorld(const Dir& a) const
    {
        return mX * a.x() + mY * a.y() + mZ * a.z();
    }

    Dir ToLocal(const Dir& a) const
    {
        return Dir(dot(a, mX), dot(a, mY), dot(a, mZ));
    }

    const Dir& Binormal() const { return mX; }
    const Dir& Tangent () const { return mY; }
    const Dir& Normal  () const { return mZ; }

public:

    Dir mX, mY, mZ;
};


#endif //__FRAME_HXX__