#pragma once

#include "render/vec.h"
#include "render/fwd.h"
#include "render/common.h"
#include "render/json.h"
#include "render/transform.h"
#include "render/materials/material.h"
#include "render/aabb.h"


struct EmitterSample {
    Vec3 y; // Position on the light source
    Vec3 n; // Normal associed with p (if applicable) 
    // if not applicable n = normalize(x-p)
    Vec2 uv;  // UV coordinates associated with p
    // if not applicable uv = Vec2(0.5)
    Real pdf; // probability density (in solid angle)

    EmitterSample() {
        y = Vec3();
        n = Vec3();
        uv = Vec2(0.5);
        pdf = 0.0;
    }
};

/// Abstract class for shapes
class Shape {
public:
    Shape() {}

    /// Compute the intersection with a ray
    /// return true if the intersection happens.
    /// If there is an intersection, the intersection object (its)
    /// get updated.
    virtual bool hit(const Ray& r, Intersection& its) const = 0;

    /// Compute the bouding box for the given shape
    virtual AABB aabb() const = 0;

    /// For shape that need to build hierachies
    /// by default do nothings
    virtual void build() {}

    /// Shapes that need to be produced
    /// and added to the group (ex. triangles generated by a mesh)
    virtual std::vector<std::shared_ptr<Shape>> children(std::shared_ptr<Shape> self) const {
        return {}; // By default, the shape do not produce other shapes
    }

    /// Sample a position on the shape to compute direct lighting from the shading point x
    virtual std::tuple< EmitterSample, const Shape& > 
            sample_direct(const Vec3& x, const Vec2& sample) const {
        votre_code_ici("Vous n'avez pas mise en oeuvre sample_direct() pour une des formes");
        return { EmitterSample(), *this };
    }
    // La valeur de la densite de probabilite d'avoir echantillonne
    // le point "y" avec la normale "n" sur la source de lumiere
    // 
    virtual Real pdf_direct(const Shape& shape, const Vec3& x, const Vec3& y, const Vec3& n) 
        const {
        votre_code_ici("Vous n'avez pas mis en oeuvre pdf_direct() pour une des formes");
        return Real(0.0);
    }

    // Permet de connaitre le matériau (et savoir si source de lumière)
    virtual const Material* material() const {
        votre_code_ici("Vous n'avez pas mise en oeuvre material() pour une des formes");
        return nullptr;
    }

    virtual Color3 evaluate_emission(const Vec3& wo, const Vec2& uv, const Vec3& p) const{
        votre_code_ici("Vous n'avez pas mise en oeuvre evaluate_emission() pour une des formes");
        return Color3(0.0);
    }

    virtual bool is_envmap() const {
        return false;
    }

};

/// Shape with material (virtual class)
class ShapeWithMaterialAndTransform: public Shape {
public:
    /// Construct a shape with a particular transform (in json) and a specified material
    ShapeWithMaterialAndTransform(std::shared_ptr<Material> material, const json& param) :
        m_material(material),
        m_transform(param.value<Transform>("transform", Transform())) {
    }

    /// Get material
    const Material* material() const override {
        return m_material.get();
    }

    /// Get transform
    const Transform& transform() const {
		return m_transform;
	}

    virtual Color3 evaluate_emission(const Vec3& wo, const Vec2& uv, const Vec3& p) const override{
        return m_material->emission(wo, uv, p);
    }

protected:
    /// Material associated to the shape
    std::shared_ptr<Material> m_material = nullptr;
    /// Transform applied to the shape
    Transform m_transform;
};


std::shared_ptr<Shape> create_shape(std::shared_ptr<Material> mat, const json& param, const bool use_instancing);
std::shared_ptr<Shape> create_airDensityQuad(std::shared_ptr<Material> mat, const json& param);

/// Object describing an intersection
struct Intersection {
    /// Intersection distance
    double t;
    /// Intersection point
    Vec3 p;
    /// Surface normal
    Vec3 n;
    Vec3 tang;
    /// UV
    Vec2 uv;
    /// Material at the intersection point
    const Material* material = nullptr;
    /// Shape at the intersection point
    const Shape* shape = nullptr;
};