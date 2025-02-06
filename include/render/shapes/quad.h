#pragma once
#include <render/shapes/shape.h>
#include <iostream>

class Quad : public ShapeWithMaterialAndTransform {
private:
    /// Half size of the quad
    Vec2 m_half_size;
    bool m_inverse_normal = false;
    
public:
    Quad(std::shared_ptr<Material> mat, const json& param = json::object()) :
        ShapeWithMaterialAndTransform(mat, param) {
        m_half_size = param.value<Vec2>("size", { 1., 1. }) / 2.0; 
        m_inverse_normal = param.value("inverse_normal", false);
    }

    bool hit(const Ray& _r, Intersection& its) const override {
        NUMBER_INTERSECTIONS += 1;
        
        // First we transform the ray defined in world space
        // to the local space
        Ray ray = m_transform.inv().ray(_r);
        
        // If the ray direction is parallel to the plane
        // no intersection can happens
        if (ray.d.z == 0.0) {
            return false; // No intersection
        }

        // Compute the intersection distance
        double t = -ray.o.z / ray.d.z;
        // If the intersection distance is outside 
        // the ray bounds, there is no intersection
        if (t < ray.tmin || ray.tmax < t) {
            return false; // no intersection
        }

        // Use the distance to compute the intersection position
        // this position is defined in local space
        auto p = ray(t);
        // Check this the x and y component of this intersection point
        // is interior of the quad
        if (std::abs(p.x) > m_half_size.x || std::abs(p.y) > m_half_size.y) {
            return false; // no intersection, outside the quad
        }

        // Trick: force the point to be on the plane
        // This operation is not mandatory but help to 
        // get a more precise intersection point
        p.z = 0; 

        // Finally, we transform all intersection
        // information from local space to world space
        its.t = t; // Distance to not change 
        its.p = m_transform.point(p); // Transform the intersection point
        its.n = m_transform.normal({0,0,1}); // Transform the local normal
        if (m_inverse_normal) {
            its.n = -its.n;
        }
        its.material = this->m_material.get(); // Give the material of the intersection point
        its.uv = (p.xy() / (2 * m_half_size)) + 0.5;
        its.shape = this; 
        // We found an intersection!
        return true;
    }

    virtual AABB aabb() const override {
        return AABB(
            m_transform.point(Vec3(-m_half_size.x, -m_half_size.y, 0) - Vec3(1e-4)),
            m_transform.point(Vec3(m_half_size.x, m_half_size.y, 0) + Vec3(1e-4))
        );
    }

    /// Sample a position on the shape to compute direct lighting from the shading point x
    virtual std::tuple< EmitterSample, const Shape& > sample_direct(const Vec3& x, const Vec2& sample) const override {
        EmitterSample em = EmitterSample();
        const Vec3 sampleRect = Vec3(2 * (sample - 0.5) * m_half_size, 0.0);
        
        em.y = m_transform.point(sampleRect);
        em.n = normalize(m_transform.normal({0.0, 0.0, 1.0}));
        em.uv = sample;
        em.pdf = pdf_direct(*this, x, em.y, em.n);
        return { em, *this };
    }
    // La valeur de la densite de probabilite d'avoir echantillonne
    // le point "y" avec la normale "n" sur la source de lumiere
    // 
    virtual Real pdf_direct(const Shape& shape, const Vec3& x, const Vec3& y, const Vec3& n) const override {
        const Vec3 diff = y - x;
        const double d2 = dot(diff, diff);
        const double area = 4 * m_half_size.x * m_half_size.y;
        const double cos_theta = std::abs(dot(normalize(n), normalize(diff)));
        if(cos_theta == 0)
            return 0.0;

        return d2 / (area * cos_theta);
    }
};