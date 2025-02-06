#pragma once

// Includes
#include <map>       // for map
#include <memory>    // for shared_ptr and make_shared
#include <set>       // for set
#include <stdexcept> // for runtime_error
#include <string>    // for operator+, basic_string
#include <utility>   // for make_pair, pair
#include <vector>    // for vector
#include <cstdio>    // for size_t

#include <spdlog/spdlog.h>


// Souvent en rendu, nous utilisons uniquement la précision simple
// pour des raisons de performances. Cependant, l'utilisation de précision
// simple peut causer plus de problèmes de précision. 
// Vu que l'on ne va pas se concentrer sur cet aspect dans le cours, nous allons
// utiliser une précision double par défaut.
using Real = double;

// A few useful constants
#undef M_PI

#define M_PI         3.14159265358979323846f
#define INV_PI       0.31830988618379067154f
#define INV_TWOPI    0.15915494309189533577f
#define INV_FOURPI   0.07957747154594766788f
#define SQRT_TWO     1.41421356237309504880f
#define INV_SQRT_TWO 0.70710678118654752440f

#define votre_code_ici(txt)                                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        static bool been_here = false;                                                                                 \
        if (!been_here)                                                                                                \
        {                                                                                                              \
            been_here = true;                                                                                          \
            spdlog::error("{}() non implementee dans {}:{}.\n    msg: {}", __FUNCTION__, __FILE__, __LINE__,           \
                         txt);                                                                                         \
        }                                                                                                              \
    } while (0);

/*!
    Python-style range: iterates from min to max in range-based for loops

    To use:

    \code{.cpp}
    for(int i = 0; i < 100; i++) { ... }             // old way
    for(auto i : range(100))     { ... }             // new way

    for(int i = 10; i < 100; i+=2)  { ... }          // old way
    for(auto i : range(10, 100, 2)) { ... }          // new way

    for(float i = 3.5f; i > 1.5f; i-=0.01f) { ... } // old way
    for(auto i : range(3.5f, 1.5f, -0.01f)) { ... } // new way
    \endcode
*/
template <typename T>
class Range
{
public:
    /// Standard iterator support for #Range
    class Iterator
    {
    public:
        Iterator(T pos, T step) : m_pos(pos), m_step(step)
        {
        }

        bool operator!=(const Iterator &o) const
        {
            return (o.m_pos - m_pos) * m_step > T(0);
        }
        Iterator &operator++()
        {
            m_pos += m_step;
            return *this;
        }
        Iterator operator++(int)
        {
            Iterator copy(*this);
                     operator++();
            return copy;
        }
        T operator*() const
        {
            return m_pos;
        }

    private:
        T m_pos, m_step;
    };

    /// Construct an iterable range from \p start to \p end in increments of \p step
    Range(T start, T end, T step = T(1)) : m_start(start), m_end(end), m_step(step)
    {
    }

    Iterator begin() const
    {
        return Iterator(m_start, m_step);
    }
    Iterator end() const
    {
        return Iterator(m_end, m_step);
    }

private:
    T m_start, m_end, m_step;
};

/// Construct a Python-style range from a single parameter \p end
template <typename T>
Range<T> range(T end)
{
    return Range<T>(T(0), end, T(1));
}

template <typename T, typename S>
inline T lerp(T a, T b, S t)
{
    return T((S(1) - t) * a + t * b);
}

/// Always-positive modulo operation
template <typename T>
inline T mod(T a, T b)
{
    int n = (int)(a / b);
    a -= n * b;
    if (a < 0)
        a += b;
    return a;
}

template<typename T>
inline T clamp(T v, T a, T b) {
    return std::min(std::max(v, a), b);
}

/// Convert radians to degrees
inline Real rad2deg(Real value)
{
    return value * Real(180.0 / M_PI);
}

/// Convert degrees to radians
inline Real deg2rad(Real value)
{
    return value * Real(M_PI / 180.0);
}

class Exception : public std::runtime_error
{
public:
    /// Variadic template constructor to support fmt::format-style arguments
    template <typename... Args>
    Exception(const char *fmt, const Args &...args) : std::runtime_error(fmt::format(fmt, args...))
    {
        // spdlog::critical(fmt::format(fmt, args...));
        // fflush(stdout);
    }
};

/*
    Taken from Nori, a simple educational ray tracer

    Copyright (c) 2015 by Wenzel Jakob

    Nori is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Nori is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
inline double fresnel(double cos_theta_i, double eta_i, double eta_t)
{
    double etaI = eta_i, etaT = eta_t;

    if (eta_i == eta_t)
        return 0.0f;

    // Swap the indices of refraction if the interaction starts at the inside of the object
    if (cos_theta_i < 0.0f)
    {
        std::swap(etaI, etaT);
        cos_theta_i = -cos_theta_i;
    }

    // Using Snell's law, calculate the squared sine of the angle between the normal and the transmitted ray
    double eta = etaI / etaT, sinThetaTSqr = eta * eta * (1 - cos_theta_i * cos_theta_i);

    if (sinThetaTSqr > 1.0)
        return double(1.0); // Total internal reflection!

    double cosThetaT = std::sqrt(1.0f - sinThetaTSqr);

    double Rs = (etaI * cos_theta_i - etaT * cosThetaT) / (etaI * cos_theta_i + etaT * cosThetaT);
    double Rp = (etaT * cos_theta_i - etaI * cosThetaT) / (etaT * cos_theta_i + etaI * cosThetaT);

    return (Rs * Rs + Rp * Rp) / double(2.0);
}

// Fonction modulo, retourne toujours un nombre positif
inline double modulo(double a, double b) {
    auto r = std::fmod(a, b);
    return (r < 0) ? r+b : r;
}

extern thread_local size_t NUMBER_INTERSECTIONS;
extern thread_local size_t NUMBER_TRACED_RAYS;
