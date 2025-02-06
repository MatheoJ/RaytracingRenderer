#pragma once

#include "render/vec.h"
#include "render/fwd.h"
#include "render/common.h"
#include "render/json.h"
#include "render/transform.h"
#include "render/materials/material.h"
#include "render/aabb.h"
#include "shape.h"
#include <iostream>

class Instance : public Shape {
public:
    Instance(std::shared_ptr<Shape> base_shape, const Transform& transform, std::shared_ptr<Material> mat)
        : m_base_shape(base_shape), m_transform(transform), m_inverse_transform(transform.inv()), m_material(mat) {
    }

    bool hit(const Ray& r, Intersection& its) const override {
        // Transformer le rayon dans l'espace local de l'instance
        Ray local_ray = m_inverse_transform.ray(r);

        // Tester l'intersection avec le mesh de base
        if (m_base_shape->hit(local_ray, its)) {
            // Transformer les données d'intersection dans l'espace monde
            its.p = m_transform.point(its.p);
            its.n = normalize(m_transform.normal(its.n));
            its.t = length((its.p - r.o));
            its.material = m_material.get();


            return true;
        }
        return false;
    }

    AABB aabb() const override {
        // Transformer l'AABB du mesh de base dans l'espace monde
        return m_base_shape->aabb().transform(m_transform);
    }

    const Material* material() const override {
        return m_material.get();
    }

    std::vector<std::shared_ptr<Shape>> children(std::shared_ptr<Shape> self) const override {

        std::vector<std::shared_ptr<Shape>> childrenBaseShape = m_base_shape->children(m_base_shape);

        if (childrenBaseShape.size() > 0) {
			std::vector<std::shared_ptr<Shape>> children;
			for (auto child : childrenBaseShape) {
				children.push_back(std::make_shared<Instance>(child, m_transform, m_material));
			}
			return children;
		}
		return childrenBaseShape;
	}

private:
    std::shared_ptr<Shape> m_base_shape;
    Transform m_transform;
    Transform m_inverse_transform;
    std::shared_ptr<Material> m_material = nullptr;
};


