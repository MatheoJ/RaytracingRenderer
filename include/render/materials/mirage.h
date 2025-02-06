#pragma once

#include <render/materials/material.h>
#include <render/samplers/sampler.h>
#include <render/json.h>

class Mirage: public Material {
protected:
    double eta_ext;
    double eta_int;
public:
    Mirage(const json& json = json::object()) {
        eta_ext = json.value("eta_ext", 1.0);
        eta_int = json.value("eta_int", 1.001);
    }

    std::optional<SampledDirection> sample(const Vec3& wo, const Vec2& uv, const Vec3& p, const Vec2& sample) const override {
        /*TODO:
        Les principales étapes sont:
        1) On copie eta_int et eta_ext dans eta_i et eta_t. On echange eta_i et eta_t is on arrive sous la surface
        2) On calcul la valeure de sin(theta_i) avec abs(cos(theta_i))  
        3) On calcul sin(theta_t) avec sin(theta_i) en utilisant les loi de Snell 
        4) On vérifie que la taille de sin(theta_t) est inférieure à 1
            - Si la norme du vecteur est supérieure à 1, on a une réfraction totale.
            Dans ce cas, on génère une réflexion direction comme les matériaux parfaitement spéculaires.
        5) Sinon, on calcule le Fresnel avec la formule de Schlick.
        6) On génére un nombre aléatoire (avec s.next()) entre [0, 1]
            - si ce nombre est inférieur au Fresnel, on effectue une réflexion spéculaire
            - sinon, on effectue une réfraction (voir cours comment calculer ce vecteur)
               
        */
        double eta_i, eta_t;
        eta_i = eta_ext;
        eta_t = eta_int;
        const Vec3 w_o = normalize(wo);
        Vec3 n = Vec3(0.0, 0.0, 1.0);

        if (wo.z < 0.0){
            eta_i = eta_int;
            eta_t = eta_ext;
            n = -n;
        }

        const double cosθ_i = w_o.z * n.z;
        const double sinθ_i = sqrt(1 - cosθ_i * cosθ_i);
        const double sinθ_t = eta_i * sinθ_i / eta_t;

        if (sinθ_t > 1.0){
            Vec3 refl = Vec3(-w_o.x, -w_o.y, w_o.z);
            return std::optional<SampledDirection>{{{1.0, 1.0, 1.0}, refl}};
        }
        const double F_0s = (eta_i - eta_t) / (eta_i + eta_t);
        const double F_0s2 = F_0s * F_0s;
        const double Fr = F_0s2 + (1 - F_0s2) * pow(1 - cosθ_i, 5);

        const double cosθ_t = sqrt(1 - sinθ_t * sinθ_t);
        const double ratio = eta_i / eta_t;
        Vec3 w_r = normalize(-ratio * w_o + (ratio * cosθ_i - cosθ_t) * n);
        
        return std::optional<SampledDirection>{{{1.0, 1.0, 1.0}, -w_o}};
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
    
    virtual bool is_air_density_change() const override {
		return true;
	}

};