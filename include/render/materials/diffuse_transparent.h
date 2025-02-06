#pragma once

#include <render/materials/material.h>
#include <render/texture.h>
#include <render/samplers/sampler.h>
#include <render/json.h>

class DiffuseTransparent : public Material {
	Texture<Color4> m_albedo;
	Texture<Real> m_alpha;

public:
	DiffuseTransparent(const json& param = json::object()){
		m_albedo = create_texture_color(param, "albedo", Color4(0.8, 0.8, 0.8, 1.0));
		m_alpha = create_texture_float(param, "alpha", 0.5);
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
		Real alpha = eval(m_alpha, uv, p);
		if (albedo.w < 0.01)
			return std::optional<SampledDirection>{{{1.0, 1.0, 1.0}, -wo, true}};
		Vec3 wi;
		if (s_copy.x < alpha){
			s_copy.x = s_copy.x / alpha;
			wi = normalize(sample_hemisphere_cos(s_copy));
		}
		s_copy.x = s_copy.x / (1 - (alpha - 1e-7));
		wi = -normalize(sample_hemisphere_cos(s_copy));
		return std::optional<SampledDirection>{{albedo.xyz(), wi}};
	}

    virtual Vec3 evaluate(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
        return eval(m_albedo, uv, p).xyz() * std::abs(wi.z) * M_1_PI;
    }

    virtual double pdf(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
		return pdf_hemisphere_cos(wi);
    }

    virtual bool have_delta(const Vec2& uv, const Vec3& p) const override {
        return false; // Pas d'interactions spéculaires
    }
};