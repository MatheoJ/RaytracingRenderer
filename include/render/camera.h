#pragma once

#include <render/common.h>
#include <render/samplers/sampler.h>
#include <render/json.h>
#include <render/transform.h>

class Camera {
protected:
    Vec2i m_resolution;
    Transform m_transform;
public:
    Camera(const json& param = json::object()) {
        // By default, 512x512 image resolution
        m_resolution = param.value("resolution", Vec2i{512, 512});
        // By default identity as the transformation
        m_transform = param.value("transform", Transform());
    }

    /// Generate a ray passing through a given pixel coordinate
    /// These coordinates are between (0, 0) to (m_resolution.x, m_resolution.y)
    virtual Ray generate_ray(const Vec2& pos_img, Sampler& s) const = 0;

    /// Image resolution attached to the camera
    const Vec2i& resolution() const {
        return m_resolution;
    }
};

inline Vec2 random_in_unit_disk(Sampler& sampler) {
    while (true) {
        auto p = (2.0 * sampler.next2d() - 1.0);
        if (length2(p) >= 1) continue;
        return p;
    }
}

class CameraPerspective: public Camera {
private:
    /// Focal distance (where the plan is placed)
    double m_fdist; 
    /// Ouverture de la camera (uniquement pour le flou de profondeur)
    double m_lens_radius;
    // Les vecteurs de la camera (exprimer dans le repere monde)
    Vec3 m_origin; 
    Vec3 m_up_left_corner;
    Vec3 m_horizontal;
    Vec3 m_vertical;

    double m_viewport_height;
    double m_viewport_width;
public:
    CameraPerspective(const json& param = json::object()): Camera(param) {
        // La distance en "z" du plan image par rapport a l'origin
        m_fdist = param.value("fdist", 1.0);
        // Uniquement pour le flou de profondeur (optionel - point bonus)
        double aperture = param.value("aperture", 0.0); 
        m_lens_radius = aperture / 2.0;
        // FOV défini en y
        double vfov = param.value("vfov", 90.0);
        // Ratio de l'image define en entree (width/height)
        double aspect_ratio = m_resolution.x / double(m_resolution.y);

        /*
        TODO: Calculer la taille du plan image 
        en fonction de l'ouverture de la caméra. 

        Suivre les explications de la section 11 de "ray tracing in one weekend":
        https://raytracing.github.io/books/RayTracingInOneWeekend.html#positionablecamera

        Attention, vfov est en spécifié en dégrée. Utiliser deg2rad pour
        transformer cette valeur de radian avant d'utiliser des fonctions 
        trigonométriques.
        */
        const auto m_viewport_height = 2.0 * m_fdist * tan(deg2rad(vfov) / 2.0); // TODO: Changer ici
        const auto m_viewport_width = aspect_ratio * m_viewport_height;
        /* 
        TODO: Copier une partie de votre mise en oeuvre 
        effectuer dans la première tâche du devoir 1. 
       
        Utiliser "m_transform" pour transformer les vecteurs
        et points calculer lors de votre première tâche du devoir 1.
        */

        m_origin = Vec3(0.0, 0.0, 0.0); // Rappelez vous, dans l'espace local, l'origin de la camera est en (0,0,0)
        m_horizontal = Vec3(m_viewport_width, 0.0, 0.0); // Vecteur horizontal definissant le plan image
        m_vertical = Vec3(0.0, -m_viewport_height, 0.0);   // Vecteur vertial definissant le plan image
        // Le point 3D correspondant au point en haut a gauche sur le plan image
        Vec3 focal = Vec3(0.0, 0.0, m_fdist);
        m_up_left_corner = m_origin - focal - (m_horizontal * 0.5) - (m_vertical * 0.5);
    }

    /// Generate a ray passing through a given pixel coordinate
    /// These coordinates are between (0, 0) to (m_resolution.x, m_resolution.y)
    Ray generate_ray(const Vec2& pos_img, Sampler& s) const override {
        /*
        TODO: Copier une partie de votre mise en oeuvre 
        effectuer dans la première tâche du devoir 1. 

        Attention, pos_img est un vecteur 2D avec des valeurs entre (0, 0) et (m_resolution.x, m_resolution.y).
        */
        auto u = double(pos_img.x) / m_resolution.x;
        auto v = double(pos_img.y) / m_resolution.y;
        Vec3 ray_direction = (m_up_left_corner + v * m_vertical + u * m_horizontal) - m_origin;
        Vec3 ray_origin = m_origin;
        Ray ray = Ray(m_origin, normalize(ray_direction));
        return m_transform.ray(ray);
    }
};
