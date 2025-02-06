#pragma once

#include <limits.h>

#include <render/vec.h>
#include <render/ray.h>

struct AABB {
    // Les points formant la AABB
    Vec3 min; //< Point minimum
    Vec3 max; //< Point maximum

    /// Initialisation d'une AABB vide
    AABB() :
        min(std::numeric_limits<Real>::max()),
        max(std::numeric_limits<Real>::lowest())
    {
        /* Il existe plusieurs pour initialiser une AABB vide. 
        Ici nous avons choisi de mettre min à +infini et max à -infini.
        Ces valeurs donne un volume englobant invalide, mais qui peut être
        facilement initialiser avec la méthode extends.
        */
    }

    /// Construction volume englobant avec deux points
    AABB(const Vec3& p1, const Vec3& p2)
    {
        min = linalg::min(p1, p2);
        max = linalg::max(p1, p2);
    }

    /// Étendre le volume englobant pour inclure la position v
    void extends(const Vec3& v) {
        min = linalg::min(min, v);
        max = linalg::max(max, v);
    }

    /// Calcul de l'intersection avec le volume englobant
    /// http://psgraphics.blogspot.de/2016/02/new-simple-ray-box-test-from-andrew.html
    bool hit(const Ray& r, double& dist) const {
        // Initialise with ray segments
        auto tmax = r.tmax;
        auto tmin = r.tmin;

        // For the different dimensions
        for (int d = 0; d < 3; d++) {
            // Inverse ray distance (can be optimized if this information is cached inside the ray structure)
            auto invD = 1.0f / r.d[d];

            // Same formula showed in class
            auto t0 = (min[d] - r.o[d]) * invD;
            auto t1 = (max[d] - r.o[d]) * invD;

            // In case if the direction is inverse:
            // we will hit the plane "max" before "min"
            // so we will swap the two distance so t0 is always the minimum
            if (invD < 0.0f)
                std::swap(t0, t1);

            // Min/Max updates (std::min, std::max)
            tmin = t0 > tmin ? t0 : tmin;
            tmax = t1 < tmax ? t1 : tmax;

            if (tmax <= tmin)
                return false; // Misses the AABB (max distance)
        }

        // Set the distance only if we have an intersection!
        dist = tmin;
        return true;
    }

    /// Calcul de l'intersection avec le volume englobant (sans la distance)
    bool hit(const Ray& r) const {
        double dist;
        return hit(r, dist);
    }

    /// Centre de la boîte englobante
    Vec3 center() const {
        return (min + max) / 2.0;
    }

    /// Vecteur représentant la diagonale de la boîte englobante
    Vec3 diagonal() const {
        return max - min;
    }

    /// Aire de la boite englobante
    double area() const
    { 
        Vec3 e = max - min; // box extent
        return 2.0 * (e.x * e.y + e.y * e.z + e.z * e.x); 
    }

    AABB transform(const Transform& t) const {
        // Start with the transformed center
        Vec3 new_center = t.point(center());

        // Calculate the transformed x, y, z axes of the box
        Vec3 half_diag = diagonal() / 2.0;
        Vec3 new_half_diag;

        // For each axis, we need to account for scaling and rotation
        // by taking the absolute value of the transformed basis vectors
        for (int i = 0; i < 3; i++) {
            new_half_diag[i] =
                std::abs(t.m[0][i] * half_diag.x) +
                std::abs(t.m[1][i] * half_diag.y) +
                std::abs(t.m[2][i] * half_diag.z);
        }

        // Construct new AABB from center and half-diagonal
        return AABB(
            new_center - new_half_diag,
            new_center + new_half_diag
        );
    }

};

/// Fonction permettant le créer une nouvelle boite englobante
/// correspondant à la funsion des boîtes a et b. 
inline AABB merge_aabb(const AABB& a, const AABB& b) {
    return AABB(min(a.min, b.min), max(a.max, b.max));
}