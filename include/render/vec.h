/*
    Based on Dartmouth Academic Ray Tracing Skeleton.
    Copyright (c) 2017-2021 by Wojciech Jarosz
*/

#pragma once

#include <linalg.h>

#include "render/common.h"
// #include <corecrt_math_defines.h>

/*! \addtogroup group_math
    @{
*/

// Definition des vecteurs et matrices 
// en utilisant linalg
// Plus d'information: https://github.com/sgorsten/linalg 

// Template version
template <class T>
using TVec2 = linalg::vec<T, 2>; ///< 2D vector of type T
template <class T>
using TVec3 = linalg::vec<T, 3>; ///< 3D vector of type T
template <class T>
using TVec4 = linalg::vec<T, 4>; ///< 4D vector of type T

// 2D
using Vec2 = TVec2<Real>;           ///< 2D vector
using Vec2i = TVec2<std::int32_t>;  ///< 2D integer vector (32-bit)
using Vec2u = TVec2<std::uint32_t>; ///< 2D unsigned vector (32-bit)

// 3D
using Vec3    = TVec3<Real>;          ///< 3D vector
using Vec3i   = TVec3<std::int32_t>;  ///< 3D integer vector (32-bit)
using Vec3u   = TVec3<std::uint32_t>; ///< 3D unsigned vector (32-bit)
using Color3 = TVec3<Real>;          ///< RGB color
using Color4 = TVec4<Real>;          ///< RGB color
using Color3u = TVec3<std::uint32_t>; ///< RGB 32-bit unsigned integer color
using Color3c = TVec3<std::uint8_t>;  ///< RGB 8-bit unsigned integer

// 4D
using Vec4 = TVec4<Real>;            ///< 4D vector

// Matrices
using Mat2 = linalg::mat<Real, 2, 2>; ///< 2x2 matrix
using Mat3 = linalg::mat<Real, 3, 3>; ///< 3x3 matrix
using Mat4 = linalg::mat<Real, 4, 4>; ///< 4x4 matrix
constexpr linalg::identity_t Identity{1}; ///< Identity constructor

// Operators for printing elements
template<class C, class T> std::basic_ostream<C> & operator << (std::basic_ostream<C> & out, const linalg::vec<T,1> & v) { return out << '[' << v[0] << ']'; }
template<class C, class T> std::basic_ostream<C> & operator << (std::basic_ostream<C> & out, const linalg::vec<T,2> & v) { return out << '[' << v[0] << ',' << v[1] << ']'; }
template<class C, class T> std::basic_ostream<C> & operator << (std::basic_ostream<C> & out, const linalg::vec<T,3> & v) { return out << '[' << v[0] << ',' << v[1] << ',' << v[2] << ']'; }
template<class C, class T> std::basic_ostream<C> & operator << (std::basic_ostream<C> & out, const linalg::vec<T,4> & v) { return out << '[' << v[0] << ',' << v[1] << ',' << v[2] << ',' << v[3] << ']'; }
template<class C, class T, int M> std::basic_ostream<C> & operator << (std::basic_ostream<C> & out, const linalg::mat<T,M,1> & m) { return out << '[' << m[0] << ']'; }
template<class C, class T, int M> std::basic_ostream<C> & operator << (std::basic_ostream<C> & out, const linalg::mat<T,M,2> & m) { return out << '[' << m[0] << ',' << m[1] << ']'; }
template<class C, class T, int M> std::basic_ostream<C> & operator << (std::basic_ostream<C> & out, const linalg::mat<T,M,3> & m) { return out << '[' << m[0] << ',' << m[1] << ',' << m[2] << ']'; }
template<class C, class T, int M> std::basic_ostream<C> & operator << (std::basic_ostream<C> & out, const linalg::mat<T,M,4> & m) { return out << '[' << m[0] << ',' << m[1] << ',' << m[2] << ',' << m[3] << ']'; }
// To make these function compatible with fmt and spglog
#include <spdlog/fmt/ostr.h>

// Function
/// Convert from linear RGB to sRGB
inline Color3 to_sRGB(const Color3 &c)
{
    Color3 result;

    for (int i = 0; i < 3; ++i)
    {
        Real value = c[i];
        if (value <= Real(0.0031308))
            result[i] = Real(12.92) * value;
        else
            result[i] = Real(1.0 + 0.055) * std::pow(value, Real(1.0) / Real(2.4)) - Real(0.055);
    }

    return result;
}

/// Convert from sRGB to linear RGB
inline Color3 to_linear_RGB(const Color3 &c)
{
    Color3 result;

    for (int i = 0; i < 3; ++i)
    {
        Real value = c[i];

        if (value <= Real(0.04045))
            result[i] = value * Real(1.0 / 12.92);
        else
            result[i] = std::pow((value + Real(0.055)) * Real(1.0 / 1.055), Real(2.4));
    }

    return result;
}

/// Check if the color vector contains a NaN/Inf/negative value
inline bool is_valid_color(const Color3 &c)
{
    for (int i = 0; i < 3; ++i)
    {
        Real value = c[i];
        if (value < 0 || !std::isfinite(value))
            return false;
    }
    return true;
}

/// Return the associated luminance
inline Real luminance(const Color4 &c)
{
    return dot(c.xyz(), {Real(0.212671), Real(0.715160), Real(0.072169)});
}

/// Return the associated luminance
inline Real luminance(const Color3 &c)
{
    return dot(c, {Real(0.212671), Real(0.715160), Real(0.072169)});
}

struct Frame {
    Vec3 x;
    Vec3 y;
    Vec3 z; 

    Frame(const Vec3& n): z(n) {
        // Based on "Building an Orthonormal Basis, Revisited" by
        // Tom Duff, James Burgess, Per Christensen, Christophe Hery, Andrew Kensler,
        // Max Liani, and Ryusuke Villemin
        // https://graphics.pixar.com/library/OrthonormalB/paper.pdf
        Real sign = copysign(Real(1.0), n.z);
        const Real a = -Real(1.0) / (sign + n.z);
        const Real b = n.x * n.y * a;
        x = Vec3(Real(1.0) + sign * n.x * n.x * a, sign * b, -sign * n.x);
        y = Vec3(b, sign + n.y * n.y * a, -n.y);
    }

    Vec3 to_world(const Vec3& v) const {
        return x* v.x + y * v.y + z * v.z;
    }
    Vec3 to_local(const Vec3& v) const {
        return { dot(v, x), dot(v, y), dot(v, z) };
    }
};

inline Vec3 sample_spherical(const Vec2& sample) {
    const double phi = 2 * M_PI * sample.x;
    const double cos_theta = 1 - 2 * sample.y;
    const double sin_theta = sqrt(1 - cos_theta * cos_theta);
    return Vec3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);
}
inline double pdf_spherical(const Vec3& dir) {
    return M_1_PI / 4.0;
}

inline Vec3 sample_hemisphere(const Vec2& sample) {
    const double root = sqrt(1 - sample.x * sample.x);
    const double a = 2 * M_PI * sample.y; 
    return {root * cos(a), root * sin(a), sample.x};
}
inline double pdf_hemisphere(const Vec3& dir) {
    if (dir.z > 0.0){
        return M_1_PI / 2.0;
    }
    else{
        return 0.0;
    }
}

inline Vec3 sample_hemisphere_cos(const Vec2& sample) {
    const double phi = 2 * M_PI * sample.x;
    const double cos_theta = sqrt(sample.y);
    const double sin_theta = sqrt(1 - cos_theta * cos_theta);
    return Vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}
inline double pdf_hemisphere_cos(const Vec3& dir) {
    if (dir.z > 0.0){
        return M_1_PI * dir.z;
    }
    else{
        return 0.0;
    }
}

inline Vec3 sample_hemisphere_cos_pow(const Vec2& sample, double exponent) {
    const double phi = 2 * M_PI * sample.x;
    const double cos_theta = pow(sample.y, 1 / (exponent + 1));
    const double sin_theta = sqrt(1 - cos_theta * cos_theta);
    return Vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}
inline double pdf_hemisphere_cos_pow(const Vec3& dir, double exponent) {
    if (dir.z > 0.0){
        return 0.5 * M_1_PI * (exponent + 1) * pow(dir.z, exponent);
    }
    else{
        return 0.0;
    }
}

inline Vec3 sample_cone(const Vec2& sample, double theta_max) {
    const double cos_theta_max = cos(theta_max);
    const double phi = 2 * M_PI * sample.x;
    const double cos_theta = 1 - sample.y * (1 - cos_theta_max);
    const double sin_theta = sqrt(1 - cos_theta * cos_theta);
    return Vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
}
inline double pdf_cone(const Vec3& dir, double theta_max) {
    const double cos_theta_max = cos(theta_max);
    if (dir.z >= cos_theta_max & dir.z > 0.0){
        const double norm = (1 - cos_theta_max) / 2;
        return 0.5 * M_1_PI * (1 / (1 - cos_theta_max));
    }
    else{
        return 0.0;
    }
}

/*! @}*/

/*!
    \file
    \brief Contains various classes for linear algebra: vectors, matrices, rays, axis-aligned bounding boxes.
*/