#pragma once

#include <render/texture.h>
#include <render/materials/material.h>
#include <render/samplers/sampler.h>
#include <render/json.h>

class Blend: public Material {
    Texture <double> alphaTexture;
    std::shared_ptr<Material> m_materialA;
    std::shared_ptr<Material> m_materialB;

public:
    Blend(const json& json = json::object()) {
        alphaTexture = create_texture_float(json, "alpha", 0.5);
        m_materialA = create_material(json["matA"]);
        m_materialB = create_material(json["matB"]);
    }

    std::optional<SampledDirection> sample(const Vec3& wo, const Vec2& uv, const Vec3& p, const Vec2& sample) const override {
        Vec2 s_copy = sample;
        double alpha = eval(alphaTexture, uv, p);
        Vec3 wi;
        if (s_copy.x < alpha){
            s_copy.x = s_copy.x / alpha;
            if (auto sd =  m_materialA->sample(wo, uv, p, s_copy)){
                wi = sd.value().wi;
                if (m_materialA->have_delta(uv, p)){
                    return sd;
                }
            }
            else{
                return std::nullopt;
            }
            const Vec3 evalMat = evaluate(wo, wi, uv, p);
            const double pdfMat = pdf(wo, wi, uv, p);
            return std::optional<SampledDirection>{{evalMat / pdfMat, wi}};
        }
        s_copy.x = (s_copy.x - alpha) / (1 - alpha);
        if (auto sd =  m_materialB->sample(wo, uv, p, s_copy)){
            wi = sd.value().wi;
            if (m_materialB->have_delta(uv, p)){
                return sd;
            }
        }
        else{
            return std::nullopt;
        }
        const Vec3 evalMat = evaluate(wo, wi, uv, p);
        const double pdfMat = pdf(wo, wi, uv, p);
        return std::optional<SampledDirection>{{evalMat / pdfMat, wi}};
    }

    virtual Vec3 evaluate(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
        double alpha = eval(alphaTexture, uv, p);
        return alpha * m_materialA->evaluate(wo, wi, uv, p) + (1 - alpha) * m_materialB->evaluate(wo, wi, uv, p);
    }

    virtual double pdf(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
        double alpha = eval(alphaTexture, uv, p);
        return alpha * m_materialA->pdf(wo, wi, uv, p) + (1 - alpha) * m_materialB->pdf(wo, wi, uv, p);
    }

    virtual bool have_delta(const Vec2& uv, const Vec3& p) const override {
        return m_materialA->have_delta(uv, p) || m_materialB->have_delta(uv, p);
    }


};