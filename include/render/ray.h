/*
   Based on the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2021 by Wojciech Jarosz
*/

#pragma once

#include "render/vec.h"

#include <limits.h>

/*! \addtogroup group_math
    @{
*/

/*!
    Simple ray segment data structure.
    Along with the ray origin and direction, this data structure additionally stores the segment interval [\ref mint,
    \ref maxt], which may include positive/negative infinity.
*/

struct Ray
{
public:
    /// Ray epsilon to avoid self intersection
    static constexpr Real eps = 0.0001;

    // Ray information
    Vec3 o;      ///< ray origin
    Vec3 d;      ///< ray direction
    double tmin; ///< ray minimal distance
    double tmax; ///< ray maximum distance

    /// Construct an empty ray 
    Ray():
        o(Vec3(0,0,0)),
        d(Vec3(0,0,0)),
        tmin(eps),
        tmax(std::numeric_limits<Real>::max())
    {}

    /// Construct a new ray with only origin and direction
    Ray(const Vec3 &origin,
        const Vec3 &direction): o(origin),
                            d(direction),
                            tmin(eps),
                            tmax(std::numeric_limits<Real>::max())

    {}

    // Construct a new ray with origin, direction and max distance
    Ray(const Vec3 &origin,
        const Vec3 &direction,
        const Real tmax) : o(origin),
                            d(direction),
                            tmin(eps),
                            tmax(tmax)
    {
    }

    /// Construct a new ray
    Ray(const Vec3 &origin,
        const Vec3 &direction,
        const Real tmin,
        const Real tmax) : o(origin),
                           d(direction),
                           tmin(tmin),
                           tmax(tmax)
    {
    }

    /// Copy new ray with different segment
    Ray(const Ray &ray, Real tmin, Real tmax) : o(ray.o), d(ray.d), tmin(tmin), tmax(tmax)
    {
    }

    Vec3 operator()(Real t) const
    {
        return o + t * d;
    }
};