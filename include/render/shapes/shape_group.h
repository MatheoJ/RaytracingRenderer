#pragma once

#include <render/shapes/shape.h>

class ShapeGroup: public Shape {
protected:
    std::vector<std::shared_ptr<Shape>> m_shapes;
    std::vector<AABB> m_shapesAABB;
    
public:
    std::vector<std::shared_ptr<Shape>> m_emitters;
    ShapeGroup(): 
        Shape()
    {
        // Nothing (for now?)   
    }

    bool hit(const Ray& r, Intersection& its) const override {
        Ray rtmp = r;
        bool hit = false;
        for(auto i = 0; i < m_shapes.size(); i++) {
            if(m_shapesAABB[i].hit(rtmp)) {
                if (m_shapes[i]->hit(rtmp, its)) {
                    rtmp.tmax = its.t;
                    hit = true;
                }
            }
        }
        return hit;
    }

    /// Add a shape to the group
    void add_shape(std::shared_ptr<Shape> s) {
        m_shapes.push_back(s);
        if((s->material() != nullptr && s->material()->have_emission()) || s->is_envmap()) {
			m_emitters.push_back(s);
		}
    }

    void build() override {
        m_shapesAABB.resize(m_shapes.size());
        for(auto i = 0; i < m_shapes.size(); i++) {
            m_shapesAABB[i] = m_shapes[i]->aabb();
        }
    }

    AABB aabb() const override {
        AABB total_aabb;
        for (const auto& s : m_shapes) {
            total_aabb = merge_aabb(total_aabb, s->aabb());
        }
        return total_aabb;
    }

    virtual std::tuple< EmitterSample, const Shape& > sample_direct(const Vec3& x, const Vec2&
        sample) const override{

        auto id = std::min(size_t(sample.x * m_emitters.size()), m_emitters.size() - 1);
        auto sample_p = sample;
        sample_p.x = (sample_p.x * m_emitters.size()) - id;
        auto [ps, shape] = m_emitters[id]->sample_direct(x, sample_p);
        ps.pdf *= 1.0 / m_emitters.size();
        return { ps, shape };
    }
    Real pdf_direct(const Shape& shape, const Vec3& x, const Vec3& y, const Vec3& n) const override{
        int nb_lights = (int)m_emitters.size();
        if (nb_lights == 0) {
            return 0.0;
        }
        if (shape.material() == nullptr || !shape.material()->have_emission()) {
            return 0;
        }
        return shape.pdf_direct(shape, x, y, n) / nb_lights;
    }


};
