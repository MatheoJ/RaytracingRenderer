#pragma once

#include <render/fwd.h>
#include <render/common.h>
#include <render/json.h>
#include <render/shapes/shape.h>
#include <render/shapes/shape_group.h>
#include <render/materials/material.h>
#include <render/samplers/sampler.h>
#include <render/camera.h>
#include <render/image.h>
#include <render/integrators/integrator.h>
#include <render/texture.h>
#include <render/shapes/envmap.h>

#include <map>
#include <vector>
#include <nanothread/nanothread.h>

class Scene {
private:
    /// Root containing all the scene's shape
    std::shared_ptr<ShapeGroup> m_root;
    /// Map containing all named materials
    std::map<std::string, std::shared_ptr<Material>> m_materials;
    /// Camera
    std::shared_ptr<Camera> m_camera;
    /// Main sampler
    std::shared_ptr<Sampler> m_sampler;
    /// Main integrator
    std::shared_ptr<Integrator> m_integrator;
    /// Background color
    std::shared_ptr<EnvironmentMap> m_background = nullptr;
    /// Max depth
    int m_max_depth = 16;
    // EnvironmentMap m_envmap;

    /// Nombre de threads pour les calculs
    int m_nb_threads = -1; // -1 = auto

    bool m_use_instancing = false;

    bool m_has_air_density_shift = false;
private:
    Color3 trace(const Ray& r, Sampler& sampler, int depth) {
        /*TODO:
        Copier et adapter le code de la fonction "trace" dans "devoirs/devoir_1.cpp".
        La couleur de l'environement peut etre obtenu avec "m_background".

        Boucle de rendu recursif, ou les principale étapes sont:
        1) Si depth == 0, on termine le chemin en retournant la couleur noire
        2) On test l'intersection en utilisant sur tout les objects de la scène (m_root)
            - Si nous avons un intersection valide:
                * On construit le repère local en utilisant la normale de l'intersection et l'objet Frame
                * On essaye d'échantillioné le materiau (avec la bonne direction d'incidence dans les coordonnées locales)
                    - Si on a reussi: on creér un nouveau rayon et on retourne le weight due au sampling et la lumière incidente (en utilisant trace)
                    - Sinon on renvoye la valeure d'emission du matriaux 
            - Sinon (pas d'intersection), retourner la couleur du fond (m_background)
        */
        if (depth >= m_max_depth){
            return Color3(0.0, 0.0, 0.0);
        }
        Intersection its;
        if (m_root->hit(r, its)){
            Frame frame = Frame(its.n);
            Vec3 w_o = frame.to_local(-r.d);
            auto hit_result = its.material->sample(w_o, its.uv, its.p, sampler.next2d());
            if (hit_result){
                Vec3 w_i = frame.to_world(hit_result->wi);
                if (hit_result->wi.z < 0.0){
                    its.n.z = -its.n.z;
                }
                Vec3 o = its.p;
                Color3 color_r = trace(Ray(o, w_i), sampler, depth + 1);
                return hit_result->weight * color_r;
            }
            else{
                return its.material->emission(w_o, its.uv, its.p);
            }
        }
        return m_background->eval_direct(r.d);
    }

public:
    /// Create a new scene based on JSON file
    Scene(const json& param = json::object(), int nb_threads = -1);

    /// Method to render the scene with simple ray tracing
    Image render() {
        // auto resolution = m_camera->resolution();
        // Image im(resolution.x, resolution.y);
        // for(auto x: range(resolution.x)) {
        //     for(auto y: range(resolution.y)) {
        //         /*TODO:
        //         L'interieur de la boucle de: "task7_recursive_raytracing"
        //         Adapter ce code: m_sampler->samples() vous permet de savoir combien d'echantillion par pixel.

        //         Pour rappel, les principales étapes: 
        //         - Initialiser une couleure temporaire à noire.
        //         - Pour chaque échantillions (disponible: 
        //             * Génération d'un point 2D aléatoire dans le pixel (si filtre de reconstruction de type boite)
        //             * Génération du rayon passant par ce point avec la caméra
        //             * Appel de "trace" pour évaluer la lumière incidente. Accumuler dans la couleure temporaire
        //         - Calculer la moyenne de la couleur temporaire, sauvegardez la dans l'image pour le pixel donné
        //         */
        //         Color3 color = Color3(0.0);
        //         for (int i = 0; i < m_sampler->samples(); i++){
        //             double sample_x = m_sampler->next();
        //             double sample_y = m_sampler->next();
        //             auto ray = m_camera->generate_ray(Vec2(x + sample_x, y + sample_y), *m_sampler);
        //             color += trace(ray, *m_sampler, 0);
        //         }
        //         im(x, y) = color / m_sampler->samples();
        //     }
        // }

        return m_integrator->render(*this, *m_sampler);
        // votre_code_ici("Devoir 2: Remplacer le code par l'appel de integrator->render(scene, sampler)");
        // return im;
    }

    /// Compute the intersection with the scene
    /// return true if there is an valid intersection
    bool hit(Ray& r, Intersection& its) const {
        NUMBER_TRACED_RAYS+=1;

        bool result = m_root->hit(r, its);

        while(m_has_air_density_shift && result && its.material->is_air_density_change()){         
        
            r = curve_ray(r, its, m_sampler->next2d());
        	result = m_root->hit(r, its);
        }
        return result;
    }

    bool hitStraight(Ray& r, Intersection& its) const {
        NUMBER_TRACED_RAYS += 1;
        bool result = m_root->hit(r, its);
        return result;
    }


    Ray curve_ray(Ray& r, Intersection& its, const Vec2 sample) const {
        Frame frame = Frame(its.n);
        Vec3 w_o = frame.to_local(-r.d);
        auto sampled_dir = its.material->sample(w_o, its.uv, its.p, m_sampler->next2d());
        Vec3 w_i = frame.to_world(sampled_dir->wi);
        Vec3 o = its.p;
        return Ray(o, w_i);
	}
    
    bool visible(const Vec3& p0, const Vec3& p1) const {
        bool is_transparent = true;
        bool is_air_density_change = true;
        bool hit_res = true;
        Vec3 point0 = p0;
        Vec3 point1 = p1;
        while (is_transparent || is_air_density_change){
            // Caclul de la direction entre p1 et p0
            Vec3 d = point1 - point0;
            double dist = linalg::length(d);
            d /= dist;

            // Prise en compte de tmin et tmax
            dist -= Ray::eps * 2.0;
            
            // Verification si on intersecte un objet entre p0 et p1
            Ray r(point0, d, dist);
            Intersection _its;
            hit_res = hitStraight(r, _its);
            if (!hit_res)
                return true;
            if (auto hit_stats = _its.material->sample(d, _its.uv, _its.p, {0.2, 0.5})){
                is_transparent = hit_stats->transparent;
                point0 = _its.p;
            }else
                is_transparent = false;
            is_air_density_change = _its.material->is_air_density_change();
            if(is_air_density_change)
            {
				point0 = _its.p;			
            }
            is_air_density_change = _its.material->is_air_density_change();
            if(is_air_density_change)
            {
				point0 = _its.p;
            }
        }
        return !hit_res;
    }

    virtual std::tuple< EmitterSample, const Shape& >
        sample_direct(const Vec3& x, const Vec2& sample) const {
        return m_root->sample_direct(x, sample);
    }

    virtual Real pdf_direct(const Shape& shape, const Vec3& x, const Vec3& y, const Vec3& n)
        const {
        return m_root->pdf_direct(shape, x, y, n);
    }



    /// Setters/Getters
    const Camera& camera() const {
        return *m_camera;
    }
    void set_camera(std::shared_ptr<Camera> c) {
        m_camera = c;
    }
    void add_material(const std::string& name, std::shared_ptr<Material> m) {
        if(m_materials.find(name) != m_materials.end()) {
            spdlog::warn("Material {} is added multiple time", name);
        }
        m_materials[name] = m;
    }
    std::shared_ptr<Material> material(const std::string& name) const {
        auto el = m_materials.find(name);
        if(el == m_materials.end()) {
            throw Exception("Impossible to found material: {}", name);
        }
        return el->second;
    }
    void add_shape(std::shared_ptr<Shape> s) {
        m_root->add_shape(s);
    }
    const ShapeGroup& root() const {
        return *m_root;
    }
    void set_sample_count(std::size_t spp) {
        m_sampler->set_samples(spp);
    }

    std::shared_ptr<Sampler> sampler() {
        return m_sampler;
    }
    Color3 background(const Vec3& d) const {
        return m_background->eval_direct(d);
    }
    void set_nb_threads(int nb_threads) {
        m_nb_threads = nb_threads;
    }
    int nb_threads() const {
        return m_nb_threads;
    }

    Real pdf_envmap(const Vec3& o, const Vec3& d) const{
        return m_background->pdf(o, d);
    }
};


json create_example_scene(int id);