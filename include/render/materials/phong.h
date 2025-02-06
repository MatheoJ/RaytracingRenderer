#pragma once

#include <render/texture.h>
#include <render/materials/material.h>
#include <render/samplers/sampler.h>
#include <render/json.h>

class Phong: public Material {
    Texture<Color4> ks;
    Texture<Color4> kd;
    Texture<double> n;
public:
    Phong(const json& json = json::object()) {
        ks = create_texture_color(json, "ks", Color4(0.8));
        kd = create_texture_color(json, "kd", Color4(0.2));
        n = create_texture_float(json, "exponent", 30.0);
    }

    std::optional<SampledDirection> sample(const Vec3& wo, const Vec2& uv, const Vec3& p, const Vec2& sample) const override {
        Vec2 s_copy = sample;
		Color4 albedo = eval(kd, uv, p);
		if (s_copy.x < albedo.w){
			s_copy.x = s_copy.x / albedo.w;
            if(wo.z < 0.0) {
                return std::nullopt;
            }

            double kd_val = luminance(eval(kd, uv, p));
            double ks_val = luminance(eval(ks, uv, p));
            double prob_phong = ks_val / (kd_val + ks_val);

            Vec2 s = s_copy;
            if(prob_phong < s.x) {
                s.x = (s.x - prob_phong) / (1.0 - prob_phong);
                // Diffuse
                auto d = sample_hemisphere_cos(s);
                return std::optional<SampledDirection> {
                    {
                        evaluate(wo, d, uv, p) / pdf(wo, d, uv, p),
                        d
                    }
                };
            } else {
                s.x /= prob_phong;

                double e = eval(n, uv, p);
                double sinAlpha = std::sqrt(1-std::pow(s.y, 2/(e + 1)));
                double cosAlpha = std::pow(s.y, 1/(e + 1));
                double phi = (2.0f * M_PI) * s.x;
                Vec3 localDir = Vec3(
                    sinAlpha * std::cos(phi),
                    sinAlpha * std::sin(phi),
                    cosAlpha
                );


                Vec3 r = linalg::normalize(Vec3{-wo.x, -wo.y, wo.z});
                Vec3 d = Frame(r).to_world(localDir);
                if (d.z < 0.0) {
                    return std::nullopt;
                }

                // Do not forget the cosine
                auto weight = ((e + 2) / (e + 1)) * eval(ks, uv, p).xyz() * d.z;
                return std::optional<SampledDirection>{{
                    evaluate(wo, d, uv, p) / pdf(wo, d, uv, p),
                    d
                }};
            }
        }
        return std::optional<SampledDirection>{{{1.0, 1.0, 1.0}, -wo}};
    }

    virtual Vec3 evaluate(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
         if (wo.z <= 0.0 || wi.z <= 0.0) {
            // Below the surface
            return Color3();
        }

        Vec3 r = linalg::normalize( Vec3{ -wo.x, -wo.y, wo.z });
        double cosAlpha = std::max(dot(normalize(wi), r), 0.0);
        double e = eval(n, uv, p);
        auto c = eval(ks, uv, p).xyz();

        auto phong = ((e + 2) / (2.0 * M_PI)) * std::pow(cosAlpha, e) * c * wi.z;

        return phong + (eval(kd, uv, p).xyz() / M_PI) * wi.z;
    }

    virtual double pdf(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
        if (wo.z <= 0.0 || wi.z <= 0.0) {
            // Below the surface
            return 0.0;
        }

        Vec3 r = linalg::normalize(Vec3{ -wo.x, -wo.y, wo.z });
        double cosAlpha = std::max(dot(normalize(wi), r), 0.0);
        double e = eval(n, uv, p);

        double kd_val = luminance(eval(kd, uv, p));
        double ks_val = luminance(eval(ks, uv, p));
        double prob_phong = ks_val / (kd_val + ks_val);

        double phong = ((e + 1) / (2.0 * M_PI)) * std::pow(cosAlpha, e);
        double diffuse = wi.z / M_PI;

        return prob_phong * phong + (1 - prob_phong) * diffuse;
    }

    virtual bool have_delta(const Vec2& uv, const Vec3& p) const override {
        return false;
    }
};