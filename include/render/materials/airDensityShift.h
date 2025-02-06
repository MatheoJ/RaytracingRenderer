#pragma once

#include <render/materials/material.h>
#include <render/samplers/sampler.h>
#include <render/json.h>
#include <iostream>

class AirDensityShift: public Material {

private :
	double eta_ratio;
    Color3 ks = Color3(1.0, 1.0, 1.0);

public:
    AirDensityShift(const double eta_e, const double eta_i) {
        eta_ratio = eta_e / eta_i;
    }

    AirDensityShift(const double ratio) {
        eta_ratio = ratio;
    }

    AirDensityShift(const json& json) {
		eta_ratio = json.at("eta_ratio").get<double>();
	}

    std::optional<SampledDirection> sample(const Vec3& wo, const Vec2& uv, const Vec3& p, const Vec2& sample) const override {
        double eta = eta_ratio;
        const Vec3 w_o = normalize(wo);
        Vec3 n = Vec3(0.0, 0.0, 1.0);

        if (wo.z < 0.0) {
            eta = 1.0 / eta;
            n = -n;
        }

        const double cosθ_i = w_o.z * n.z;
        const double sinθ_i = sqrt(1 - cosθ_i * cosθ_i);
        double sinθ_t = eta * sinθ_i;

        const double jitter_strength = 0.01; // Adjust this to control the magnitude of jitter
        Vec3 jitter(
            (sample.x - 0.5) * jitter_strength, // Map sample.x from [0, 1] to [-0.005, 0.005]
            (sample.y - 0.5) * jitter_strength + std::fmod(p.x, jitter_strength) +  jitter_strength , // Map sample.y from [0, 1] to [-0.005, 0.005]
            0.0 // Apply jitter only in the x and y directions for now
        );

        if (sinθ_t > 1.0) {
            Vec3 refl = Vec3(-w_o.x, -w_o.y, w_o.z);

            const double nonReflectChanceBorder =0.002;
            const double nonReflectChance = 0.01;
            if( sample.x* nonReflectChanceBorder +1.0> sinθ_t || sample.y< nonReflectChance)
            {
                sinθ_t = 1.0;
            }
            else
            {
                return std::optional<SampledDirection>{{ks, normalize(refl+ jitter)}};
            }
        }

        const double cosθ_t = sqrt(1 - sinθ_t * sinθ_t);
        Vec3 w_r = normalize(-eta * w_o + (eta * cosθ_i - cosθ_t) * n + jitter);

        return std::optional<SampledDirection>{{ks, w_r}};
    }

    virtual bool is_air_density_change() const override{
        return true;
    }

    virtual Vec3 evaluate(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
        return Vec3(0.0);
    }

    virtual double pdf(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
        return 0.0;
    }

    virtual bool have_delta(const Vec2& uv, const Vec3& p) const override {
        return true; // Pas d'interactions spéculaires
    }
};