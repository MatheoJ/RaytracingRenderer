#pragma once

#include <render/common.h>
#include <render/texture.h>
#include <render/materials/material.h>
#include <render/samplers/sampler.h>
#include <render/json.h>

class FresnelBlend: public Material {
    double eta_int;
    std::shared_ptr<Material> m_materialA;
    std::shared_ptr<Material> m_materialB;

public:
    FresnelBlend(const json& json = json::object()) {
        eta_int = json.value("eta", 1.5);
        m_materialA = create_material(json["matA"]);
        m_materialB = create_material(json["matB"]);
    }

    std::optional<SampledDirection> sample(const Vec3& wo, const Vec2& uv, const Vec3& p, const Vec2& sample) const override {
        double eta_i, eta_t;
        eta_i = 1.0;
        eta_t = eta_int;
        const Vec3 w_o = normalize(wo);
        Vec3 n = Vec3(0.0, 0.0, 1.0);

        if (wo.z < 0.0){
            eta_i = eta_int;
            eta_t = 1.0;
            n = -n;
        }
        const double cosθ_i = w_o.z * n.z;
        double Fr = fresnel(cosθ_i, eta_i, eta_t);

        Vec2 s_copy = sample;
        Vec3 wi;
        if (s_copy.x < Fr){
            s_copy.x = s_copy.x / Fr;
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
        s_copy.x = (s_copy.x - Fr) / (1 - Fr);
        if (auto sd =  m_materialB->sample(wo, uv, p, s_copy)){
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

    virtual Vec3 evaluate(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
        if (have_delta(uv, p)){
            return Vec3(0.0);
        }
        double eta_i, eta_t;
        eta_i = 1.0;
        eta_t = eta_int;
        const Vec3 w_o = normalize(wo);
        Vec3 n = Vec3(0.0, 0.0, 1.0);

        if (wo.z < 0.0){
            eta_i = eta_int;
            eta_t = 1.0;
            n = -n;
        }
        const double cosθ_i = w_o.z * n.z;
        double Fr = fresnel(cosθ_i, eta_i, eta_t);
        return Fr * m_materialA->evaluate(wo, wi, uv, p) + (1 - Fr) * m_materialB->evaluate(wo, wi, uv, p);
    }

    virtual double pdf(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
        if (have_delta(uv, p)){
            return 0.0;
        }
        double eta_i, eta_t;
        eta_i = 1.0;
        eta_t = eta_int;
        const Vec3 w_o = normalize(wo);
        Vec3 n = Vec3(0.0, 0.0, 1.0);

        if (wo.z < 0.0){
            eta_i = eta_int;
            eta_t = 1.0;
            n = -n;
        }
        const double cosθ_i = w_o.z * n.z;
        double Fr = fresnel(cosθ_i, eta_i, eta_t);
        return Fr * m_materialA->pdf(wo, wi, uv, p) + (1 - Fr) * m_materialB->pdf(wo, wi, uv, p);
    }

    virtual bool have_delta(const Vec2& uv, const Vec3& p) const override {
        return m_materialA->have_delta(uv, p) || m_materialB->have_delta(uv, p);
    }

};