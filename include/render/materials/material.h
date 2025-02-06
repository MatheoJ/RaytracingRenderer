#pragma once

#include <optional>

#include <render/fwd.h>
#include <render/vec.h>
#include <render/samplers/sampler.h>

// structure representing a sampled direction by the material
struct SampledDirection {
    Color3 weight; //< The "color" of the material
    Vec3 wi;       //< omega_i (direction where to sample the next ray), expressed in local coordinate
    bool transparent = false;
};

class Material {
public:
    virtual std::optional<SampledDirection> sample(const Vec3& wo, const Vec2& uv, const Vec3& p, const Vec2& sample) const {
        // By default, sampling the material will fails.
        // Otherwise return
        // - weight: fr(wo, wi) * cos(theta) / pdf(wi | wo)
        // - direction: wi ~ pdf(wi | wo)
        return std::nullopt;
    }
    // nouvelle méthode pour évaluer la valeur du matériau f_r(d_in, d_out)
    // multiplé par le cosinus -- Mettre en œuvre pour tache 4
    virtual Vec3 evaluate(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const {
        votre_code_ici("Vous n'avez pas mis en œuvre evaluate() pour une des BSDFs")
        return Vec3();
    }
    // nouvelle méthode pour évaluer la valeur de la PDF p(wi | wo)
    // testée seulement dans la tache 4
    virtual double pdf(const Vec3& wo, const Vec3& wi, const Vec2& uv,
    const Vec3& p) const {
        votre_code_ici("Vous n'avez pas mis en œuvre evaluate() pour une des BSDFs")
        return 0.0;
    }
    // nouvelle méthode pour signifier si le matériau contient une composante discrète
    virtual bool have_delta(const Vec2& uv, const Vec3& p) const {
        return false; // Pas d'interactions spéculaires
    }

    // Virtual function if the material emit light
    virtual Color3 emission(const Vec3& wo, const Vec2& uv, const Vec3& p) const {
        return {0,0,0};
    }
    // nouvelle methode pour signifier si un materiau emet de la lumiere
    virtual bool have_emission() const {
        return false;
    }

    virtual bool is_air_density_change() const {
		return false;
	}

};

// Utility function to sample a random point
// uniformly inside a unit sphere
inline Vec3 random_in_unit_sphere(Sampler& s)
{
	Vec3 p;
	do
	{
		auto a = s.next();
		auto b = s.next();
		auto c = s.next();
		p = 2.0f * Vec3(a, b, c) - Vec3(1);
	} while (length2(p) >= 1.0f);

	return p;
}

// function to create material (implemented inside src/materials/material.cpp)
std::shared_ptr<Material> create_material(const json& param);

std::shared_ptr<Material> create_airDensityMaterial(double etaExt, double etaInt);

std::shared_ptr<Material> create_airDensityMaterial(double ratio);
