#pragma once

#include <render/materials/material.h>
#include <render/texture.h>
#include <render/samplers/sampler.h>
#include <render/json.h>

class Diffuse : public Material {
	Texture<Color4> m_albedo;

public:
	Diffuse(const json& param = json::object()){
		m_albedo = create_texture_color(param, "albedo", Color4(0.8, 0.8, 0.8, 1.0));
	}

	virtual std::optional<SampledDirection> sample(const Vec3& wo, const Vec2& uv, const Vec3& p, const Vec2& sample) const override {
		/*TODO:
		Mettre en œuvre un rebond sur une surface diffuse. Les principales étapes sont:
		1) Vérifier si on arrive sous la surface, si c'est le cas, ne pas retourner d'échantillon.
		2) Générer un point sur la *surface* d'une sphère unitaire (avec l'aide de `random_in_unit_sphere()`).
		3) Déplacer le point avec la normale (ici, dans l'espace local, la normale est [0, 0, 1]).
		4) Retourner un échantillon avec la direction normalisée au point déplacé.
		*/
		Vec2 s_copy = sample;
		Color4 albedo = eval(m_albedo, uv, p);
		if (albedo.w < 0.01)
			return std::optional<SampledDirection>{{{1.0, 1.0, 1.0}, -wo, true}};
		if (s_copy.x < albedo.w){
			s_copy.x = s_copy.x / albedo.w;
			Vec3 wi = normalize(sample_hemisphere_cos(s_copy));
			return std::optional<SampledDirection>{{albedo.xyz(), wi}};
		}
		s_copy.x = s_copy.x / (1 - (albedo.w - 1e-7));
		Vec3 wi = -normalize(sample_hemisphere_cos(s_copy));
		return std::optional<SampledDirection>{{albedo.xyz(), -wi, true}};
	}

    virtual Vec3 evaluate(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
		if (wi.z > 0.0 /* & wo.z > 0.0 */){
        	return eval(m_albedo, uv, p).xyz() * wi.z * M_1_PI;
		}
		return Vec3(0.0);
    }

    virtual double pdf(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
		return pdf_hemisphere_cos(wi);
    }

    virtual bool have_delta(const Vec2& uv, const Vec3& p) const override {
        return false; // Pas d'interactions spéculaires
    }
};