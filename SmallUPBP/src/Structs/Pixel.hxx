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

#ifndef __PIXEL_HXX__
#define __PIXEL_HXX__

#include "Vector.hxx"

// Two-dimensional int vector, used mainly for pixel addressing.
class Pixel {
public:
    union{
        struct {
            int x;
            int y;
        };
        int _data[2];
    };    

    INLINE Pixel() { }

    INLINE Pixel(const int x, const int y) : x(x), y(y) { }

    // Creates an instance with floor()-ed values of a floating-point vector.
    INLINE explicit Pixel(const Vector2 vector) : x(int(vector.x)), y(int(vector.y)) { }

    INLINE explicit Pixel(const int val) : x(val), y(val) { }

    INLINE int operator[](const int index) const {
        UPBP_ASSERT(unsigned(index) < 2);
        return _data[index];
    }

    INLINE int& operator[](const int index) {
        UPBP_ASSERT(unsigned(index) < 2);
        return _data[index];
    }

    INLINE Pixel operator-(const Pixel other) const {
        return Pixel(this->x - other.x, this->y - other.y);
    }

    INLINE Pixel operator+(const Pixel other) const {
        return Pixel(this->x + other.x, this->y + other.y);
    }

    INLINE Pixel& operator+=(const Pixel other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    INLINE Pixel& operator-=(const Pixel other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    INLINE Vector2 operator*(const float factor) const {
        return Vector2(x*factor, y*factor);
    }

    INLINE Pixel operator*(const int factor) const {
        return Pixel(x*factor, y*factor);
    }

    INLINE Pixel operator/(const int factor) const {
        UPBP_ASSERT(factor != 0);
        return Pixel(x/factor, y/factor);
    }

    INLINE operator Vector2() const {
        return Vector2(float(x), float(y));
    }

	// Component-wise minimum.
    static INLINE Pixel min(const Pixel first, const Pixel second) {
        return Pixel(Utils::min(first.x, second.x), Utils::min(first.y, second.y));
    }

	// Component-wise maximum.
    static INLINE Pixel max(const Pixel first, const Pixel second) {
        return Pixel(Utils::max(first.x, second.x), Utils::max(first.y, second.y));
    }

	// Component-wise clamping.
    static INLINE Pixel clamp(const Pixel value, const Pixel minimum, const Pixel maximum) {
        UPBP_ASSERT(minimum.x <= maximum.x && minimum.y <= maximum.y);
        return max(minimum, min(value, maximum));
    }

    INLINE bool operator==(const Pixel other) const {
        return x == other.x && y == other.y;
    }

    INLINE bool operator!=(const Pixel other) const {
        return x != other.x || y != other.y;
    }

    INLINE Pixel operator<<(const int amount) const {
        return Pixel(x<<amount, y<<amount);
    }
    
    INLINE Pixel operator>>(const int amount) const {
        return Pixel(x>>amount, y>>amount);
    }

    INLINE Pixel& operator<<=(const int amount) {
        x <<= amount;
        y <<= amount;
        return *this;
    }
    INLINE Pixel& operator>>=(const int amount) {
        x >>= amount;
        y >>= amount;
        return *this;
    }

    INLINE Pixel operator-() const {
        return Pixel(-x, -y);
    }
    
    INLINE Pixel& operator*=(const int factor) {
        x *= factor;
        y *= factor;
        return *this;
    }

	// Divides input by factor component-wise.
    INLINE static void div(const Pixel input, const int factor, Pixel& quot, Pixel& rem) {
        div_t x = ::div(input.x, factor);
        div_t y = ::div(input.y, factor);
        rem = Pixel(x.rem, y.rem);
        quot = Pixel(x.quot, y.quot);
    }

    static const Pixel ZERO;
};

typedef Pixel Vec2i;


INLINE Vector2 operator+(const Vector2 first, const Pixel second) {
    return Vector2(first.x + second.x, first.y + second.y);
}
INLINE Vector2 operator+(const Pixel first, const Vector2 second) {
    return Vector2(first.x + second.x, first.y + second.y);
}
INLINE std::ostream& operator<<(std::ostream& os, const Pixel pixel) {
    os << pixel.x << ' ' << pixel.y;
    return os;
}

#endif //__PIXEL_HXX__