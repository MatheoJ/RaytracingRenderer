#pragma once
#include <render/shapes/shape.h>
#include <render/scene.h>
#include <render/integrators/integrator.h>

class PathIntegrator : public SamplerIntegrator {
private:
	int m_max_depth = 0;

public:
    PathIntegrator(const json& param = json::object()) {
		m_max_depth = param.value("max_depth", 16);
	}

	Color3 Li(Ray& r, const Scene& scene, Sampler& sampler) {
		/* Copier votre code scene::trace(...)
		Adapter votre code pour éviter la récursion 
		Vous pouvez accumuler la couleur des rebonds (du aux matériaux) dans une couleur. Par exemple:
		
		// Initalisation [...]
		Color3 throughput(1.0);

		// boucle de rendu rebondissant sur les surfaces
		while(...) { // Ou for loop. Condition sur la profondeur du chemin (depth < m_max_depth)
			// ... 
			if(auto res = its.material->sample(...)) {
				throughput *= res->weight;
				// ...
			} else {
				// ... 
                return throughput * its.material->emission(...);
			}
		}

		Cette variable vous permet ensuite de calculer la couleur totale.
		Exemple, si vous ne trouvez pas d'intersection: 

		return throughput * scene.background(...); 

        Ce code doit donner le meme resultat que votre code précédent.
		*/
		Color3 throughput(1.0);
		int depth = 0;
		Ray current_ray = Ray(r.o, r.d);
		while (depth < m_max_depth){
			auto its = Intersection();
        	if (scene.hit(current_ray, its)){
				depth++;
				Frame frame = Frame(its.n);
				Vec3 w_o = frame.to_local(-current_ray.d);
				if(auto hit_result = its.material->sample(w_o, its.uv, its.p, sampler.next2d())) {
					throughput *= hit_result->weight;
					Vec3 w_i = frame.to_world(hit_result->wi);
					if (hit_result->wi.z < 0.0){
						its.n.z = -its.n.z;
					}
					Vec3 o = its.p;
					current_ray = Ray(o, w_i);
				} else {
					return throughput * its.material->emission(w_o, its.uv, its.p);
				}
			}
			else{
				return throughput * scene.background(current_ray.d);
			}
		}
		return Color3(0.0);
	}
};