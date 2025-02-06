#pragma once
#include <render/materials/material.h>
#include <render/samplers/sampler.h>
#include <render/json.h>
#include <render/texture.h>

class Metal: public Material {
    Texture<Color4> ks;
    Texture<double> roughness;

public:
    Metal(const json& json = json::object()) {
        ks = create_texture_color(json, "ks", Color4(1.0));
        roughness = create_texture_float(json, "roughness", 0.0);
    }

    std::optional<SampledDirection> sample(const Vec3& wo,  const Vec2& uv, const Vec3&p, const Vec2& sample) const override {
        if(wo.z < 0.0) {
            return std::nullopt;
        }

        double r = eval(roughness, uv, p);
        if(r == 0.0) {
            return std::optional<SampledDirection> {
                {
                    eval(ks, uv, p).xyz(),
                    {-wo.x, -wo.y, wo.z}
                }
            };
        } else {
            double e = (2.0 / (r*r)) - 2.0;
            double sinAlpha = std::sqrt(1-std::pow(sample.y, 2/(e + 1)));
            double cosAlpha = std::pow(sample.y, 1/(e + 1));
            double phi = (2.0f * M_PI) * sample.x;
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
            return std::optional<SampledDirection> {
                {
                    eval(ks, uv, p).xyz(),
                    normalize(d)
                }
            };  

            // Vec3 d = sample_spherical(sample)*r;
            // d = d + Vec3{-wo.x, -wo.y, wo.z};
            // if(d.z < 0.0) {
            //     return std::nullopt;
            // } else {
            //   return std::optional<SampledDirection> {
            //     {
            //         eval(ks, uv),
            //         normalize(d)
            //     }
            //     };  
            // }
        }
    }

    virtual bool have_delta(const Vec2& uv, const Vec3& p) const override {
        double r = eval(roughness, uv, p);
        return r == 0.0;
    }

    virtual double pdf(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
        double r = eval(roughness, uv, p);
        if(r == 0.0) {
            return 0.0;
        } else {
            if (wo.z <= 0.0 || wi.z <= 0.0) {
                // Below the surface
                return 0.0;
            }

            Vec3 reflect = linalg::normalize(Vec3{ -wo.x, -wo.y, wo.z });
            double cosAlpha = std::max(dot(normalize(wi), reflect), 0.0);
            double e = (2.0 / (r*r)) - 2.0;
            return ((e + 1) / (2.0 * M_PI)) * std::pow(cosAlpha, e);
        }
    }

    virtual Color3 evaluate(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
        double r = eval(roughness, uv, p);
        if(r == 0.0) {
            return Color3();
        } else {
            if (wo.z <= 0.0 || wi.z <= 0.0) {
                // Below the surface
                return Color3();
            }

            Vec3 reflect = linalg::normalize(Vec3{ -wo.x, -wo.y, wo.z });
            double cosAlpha = std::max(dot(normalize(wi), reflect), 0.0);
            double e = (2.0 / (r*r)) - 2.0;
            return ((e + 1) / (2.0 * M_PI)) * std::pow(cosAlpha, e) *  eval(ks, uv, p).xyz();
        }
    }
};