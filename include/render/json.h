/*
   Based on the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2021 by Wojciech Jarosz
*/
#pragma once

#include <render/transform.h>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4706)
#endif
#include <nlohmann/json.hpp>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

/*! \addtogroup group_parser
    @{
*/

using json = nlohmann::json; ///< Bring nlohmann::json into scope

// The below functions allow us to parse matrices, vectors, and other common darts types from a #json object
namespace linalg
{

/// parse a Mat44<T> from json
template<typename T>
void from_json(const json &j, mat<T, 4, 4> &m)
{
    if (j.count("from") || j.count("at") || j.count("to") || j.count("up"))
    {
        vec<T, 3> from(0, 0, 1), at(0, 0, 0), up(0, 1, 0);
        from = j.value("from", from);
        at   = j.value("at", at) + j.value("to", at);
        up   = j.value("up", up);
        vec<T, 3> dir   = normalize(from - at);
        vec<T, 3> left  = normalize(cross(up, dir));
        vec<T, 3> newUp = normalize(cross(dir, left));

        m = mat<T, 4, 4>(vec<T, 4>(left, 0.f), vec<T, 4>(newUp, 0.f), vec<T, 4>(dir, 0.f), vec<T, 4>(from, 1.f));
    }
    else if (j.count("o") || j.count("x") || j.count("y") || j.count("z"))
    {
        vec<T, 3> o(0, 0, 0), x(1, 0, 0), y(0, 1, 0), z(0, 0, 1);
        o = j.value("o", o);
        x = j.value("x", x);
        y = j.value("y", y);
        z = j.value("z", z);
        m = mat<T, 4, 4>(vec<T, 4>(x, 0.f), vec<T, 4>(y, 0.f), vec<T, 4>(z, 0.f), vec<T, 4>(o, 1.f));
    }
    else if (j.count("translate"))
    {
        vec<T, 3> translate = j.value("translate", vec<T, 3>(0.f));
        m = translation_matrix(translate);
    }
    else if (j.count("scale"))
    {
        m = scaling_matrix(j.value("scale", vec<T, 3>(1)));
    }
    else if (j.count("rotation"))
    {
        // YXZ
        vec<T, 3> r = j.value("rotation", vec<T, 3>(0, 0, 0));
        r = r*M_PI/180.0f;
        T c[] = {std::cos(r.x), std::cos(r.y), std::cos(r.z)};
        T s[] = {std::sin(r.x), std::sin(r.y), std::sin(r.z)};

        m = transpose(mat<T, 4, 4>(
            {c[1]*c[2] - s[1]*s[0]*s[2],   -c[1]*s[2] - s[1]*s[0]*c[2], -s[1]*c[0], 0.0f},
            {c[0]*s[2],                     c[0]*c[2],      -s[0], 0.0f},
            {s[1]*c[2] + c[1]*s[0]*s[2],   -s[1]*s[2] + c[1]*s[0]*c[2],  c[1]*c[0], 0.0f},
            {0.0f,                          0.0f,       0.0f, 1.0f}
        ));
    }
    else if (j.count("axis") || j.count("angle"))
    {
        vec<T, 3> axis = j.value("axis", vec<T, 3>(1, 0, 0));
        T angle = deg2rad(j.value<T>("angle", 0.f));
        m = rotation_matrix(rotation_quat(normalize(axis), angle));
    }
    else if (j.count("matrix"))
    {
        json jm = j.at("matrix");
        if (jm.size() == 1)
        {
            m = mat<T, 4, 4>(jm.get<T>());
            spdlog::warn("Incorrect array size when trying to parse a Matrix. "
                         "Expecting 4 x 4 = 16 values but only found a single scalar. "
                         "Creating a 4 x 4 scaling matrix with '{}'s along the diagonal.",
                         jm.get<float>());
            return;
        }
        else if (16 != jm.size())
        {
            throw Exception("Incorrect array size when trying to parse a Matrix. "
                                 "Expecting 4 x 4 = 16 values but found {}, here:\n{}.",
                                 jm.size(), jm.dump(4));
        }

        // jm.size() == 16
        for (auto row : range(4))
            for (auto col : range(4))
                jm.at(row * 4 + col).get_to(m[col][row]);
    }
    else
        throw Exception("Unrecognized 'transform' command:\n{}.", j.dump(4));
}

/// parse a Vec<N,T> from json
template <class T, int N>
inline void from_json(const json &j, vec<T, N> &v)
{
    if (j.is_object())
        throw Exception("Can't parse a Vec{}. Expecting a json array, but got a json object.", N);

    if (j.size() == 1)
    {
        if (j.is_array())
            spdlog::info("Incorrect array size when trying to parse a Vec3. "
                         "Expecting {} values but only found 1. "
                         "Creating a Vec of all '{}'s.",
                         N, j.get<T>());
        v = vec<T, N>(j.get<T>());
        return;
    }
    else if (N != j.size())
    {
        throw Exception("Incorrect array size when trying to parse a Vec. "
                             "Expecting {} values but found {} here:\n{}",
                             N, (int)j.size(), j.dump(4));
    }

    // j.size() == N
    for (auto i : range((int)j.size()))
        j.at(i).get_to(v[i]);
}

// /// Serialize a Mat44<T> to json
// template <class T>
// inline void to_json(json &j, const mat<T, 4, 4> &v)
// {
//     j.at("matrix") = vector<T>(reinterpret_cast<const T *>(&v.x), reinterpret_cast<const T *>(&v.x) + 16);
// }

// /// Serialize a Vec3<N,T> to json
// template <class T, int N>
// inline void to_json(json &j, const vec<T, N> &v)
// {
//     j = vector<T>(&(v[0]), &(v[0]) + N);
// }

} // namespace linalg

/// Parse a Transform from json
inline void from_json(const json &j, Transform &v)
{
    Mat4 m = v.m;
    if (j.is_array())
    {
        // multiple transformation commands listed in order
        for (auto &element : j)
            m = mul(element.get<Mat4>(), m);
    }
    else if (j.is_object())
    {
        // a single transformation
        j.get_to(m);
    }
    else
        throw Exception("'transform' must be either an array or an object here:\n{}", j.dump(4));

    v = Transform(m);
}

/// Serialize a Transform to json
// inline void to_json(json &j, const Transform &t)
// {
//     to_json(j, t.m);
// }

/*!
    \file
    \brief Serialization and deserialization of various darts types to/from JSON
*/

/*! @}*/