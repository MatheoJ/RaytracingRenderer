#pragma once

#include <render/shapes/shape.h>
// #include <corecrt_math_defines.h>

class Sphere : public ShapeWithMaterialAndTransform {
private:
    double m_radius;
    bool m_inverse_normal;
public:
    Sphere(std::shared_ptr<Material> mat, const json& param = json::object()):
        ShapeWithMaterialAndTransform(mat, param)
    {
        m_radius = param.value("radius", 1.0);
        m_inverse_normal = param.value("inverse_normal", false);
    }

    bool hit(const Ray& _r, Intersection& its) const override {
        NUMBER_INTERSECTIONS += 1;
        /*
        TODO: Cacluler l'intersection d'une sphère centrée en [0, 0, 0] et avec un rayon (m_radius).
        Regardez comment est calculer l'intersection d'un plan (disponible dans `include/render/shapes/quad.h`).
        Référez vous aux diapositives du cours ou "Ray tracing in one weekend" section 5
        https://raytracing.github.io/books/RayTracingInOneWeekend.html#addingasphere 

        En regle générale, les étapes sont:
        1) Transformation du rayon en coordonnée locale en utilisant m_transform
        2) Calcul de l'intersection dans ces coordonnées locale
            - Essayer de retourner faux le plus tot possible si il n'y a pas d'intersection
            - Si on est sur de l'intersection, calculer les information de l'intersection (position, distance, normale, ...)
        3) Transformation du point d'intersection en coordonnée monde.
        */
        Ray ray = m_transform.inv().ray(_r);
        double a = dot(ray.d, ray.d);
        double b = 2 * dot(ray.d, ray.o);
        double c = dot(ray.o, ray.o) - m_radius * m_radius;

        double delta = b * b - 4 * a * c;
        if (delta < 0.0){
            return false;
        }
        double determinant = sqrt(delta);
        double t = (-b - determinant) / (2 * a);
        if (!(t > ray.tmin && t < ray.tmax)) {
            t = (-b + determinant) / (2 * a);
            if (!(t > ray.tmin && t < ray.tmax)) {
                return false;       
            }
        }
        
        Vec3 hit_pos = ray.o + t * ray.d;
        Vec3 hit_nor = normalize(hit_pos);
        hit_nor = normalize(m_transform.normal(hit_nor));

        its.t = t;
        its.p = m_transform.point(hit_pos);
        its.n = hit_nor;
        its.material = m_material.get();
        
        const double inv_radius = 1.0 / m_radius;
        const double u = 1 + (INV_PI * atan2(hit_pos.y * inv_radius, hit_pos.x * inv_radius));
        const double v = 1 - INV_PI * acos(hit_pos.z * inv_radius);
        its.uv = {u / 2.0, v};
        its.shape = this;

        return true;
    }

    AABB aabb() const override {
        Vec3 lp{0,0,0};
        lp = m_transform.point(lp);
        
        double m0 = length(m_transform.vector(Vec3{ m_radius, 0, 0 }));
        double m1 = length(m_transform.vector(Vec3{ 0, m_radius, 0  }));
        double m2 = length(m_transform.vector(Vec3{ 0, 0, m_radius }));
        double m = std::max(m0, std::max(m1, m2));

        return AABB(
           lp - Vec3{m},
           lp + Vec3{m}
        );
    }

    /// Sample a position on the shape to compute direct lighting from the shading point x
    virtual std::tuple< EmitterSample, const Shape& > sample_direct(const Vec3& x, const Vec2& sample) const override {
        EmitterSample em = EmitterSample();

        const Vec3 sampleDirection = normalize(sample_spherical(sample));
        const Vec3 sampleSphere = m_radius * sampleDirection;

        em.y = m_transform.point(sampleSphere);
        em.n = normalize(m_transform.normal(sampleDirection));
        em.uv = sample;
        em.pdf = pdf_direct(*this, x, em.y, em.n);

        return { em, *this };
    }
    // La valeur de la densite de probabilite d'avoir echantillonne
    // le point "y" avec la normale "n" sur la source de lumiere
    // 
    virtual Real pdf_direct(const Shape& shape, const Vec3& x, const Vec3& y, const Vec3& n) const override {
     
        const double surface_area = 4.0 * M_PI * m_radius * m_radius;        

        const Vec3 diff = y - x;
        const Vec3 w = normalize(diff);
        const double cos_theta = std::abs(dot(n, -w)); 
        const double d2 = dot(diff, diff);       

        return d2 / (surface_area * cos_theta);
    }
};
