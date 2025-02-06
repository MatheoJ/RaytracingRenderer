#pragma once
#include <render/shapes/shape.h>
#include <render/scene.h>
#include <render/integrators/integrator.h>

class PathMISIntegrator : public SamplerIntegrator {
private:
	int m_max_depth = 0;

public:
    PathMISIntegrator(const json& param = json::object()) {
		m_max_depth = param.value("max_depth", 16);
	}

	Color3 compute_direct(const Intersection& its, const Vec3& w_o, const Scene& scene, Sampler& sampler){
		if (its.material->have_emission())
			return Color3(0.0);
		Vec3 contrib = Vec3(0.0);
		Vec3 throughput = Vec3(1.0);
		Frame frame = Frame(its.n);
		std::tuple< EmitterSample, const Shape& > DirectSample = scene.sample_direct(its.p, sampler.next2d());
		const EmitterSample emitterSample = std::get<0>(DirectSample);
		const Shape& emitterShape = std::get<1>(DirectSample);
		const Vec3 wi = normalize(emitterSample.y - its.p);
		if (wi.z > 0.0){
			const bool is_delta = its.material->have_delta(its.uv, its.p);

			if (scene.visible(emitterSample.y, its.p) && !is_delta) {
				throughput *= its.material->evaluate(w_o, normalize(frame.to_local(wi)), its.uv, its.p);
				const Real pdf_bsdf1 = its.material->pdf(w_o, normalize(frame.to_local(wi)), its.uv, its.p);
				Frame frame_emiter = Frame(emitterSample.n);
				// Vec3 w_o_light = normalize(frame_emiter.to_local(-wi));
				Vec3 w_o_light = normalize(wi);

				Vec3 Le = emitterShape.evaluate_emission(w_o_light, emitterSample.uv, emitterSample.y);

				const Real pdf_emitter1 = emitterSample.pdf;
				throughput /= pdf_emitter1;
				const Real mis_w = pdf_emitter1 / (pdf_bsdf1 + pdf_emitter1);
				contrib += mis_w * throughput * Le;
			}
		}

			// Vec3 w_i = normalize(frame.to_world(hit_result->wi));
			// if (hit_result->wi.z < 0.0){
			// 	its.n.z = -its.n.z;
			// }
			// Vec3 o = its.p;
			// current_ray = Ray(o, w_i);
			// const Real pdf_bsdf2 = its.material->pdf(w_o, hit_result->wi, its.uv, its.p);
			// if (scene.hit(current_ray, its)){
			// 	Real mis_w = 1.0;
			// 	if (!is_delta){
			// 		const Real pdf_emitter2 = scene.pdf_direct(*its.shape, o, its.p, its.n);
			// 		mis_w = pdf_bsdf2 / (pdf_bsdf2 + pdf_emitter2);
			// 	}
			// 	frame = Frame(its.n);
			// 	w_o = frame.to_local(-current_ray.d);
			// 	contrib += mis_w * hit_result->weight * its.material->emission(w_o, its.uv, its.p);
			// }
		return contrib;
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
		Color3 contrib = Vec3(0.0);
		Real mis_w_bsdf = 1.0;
		bool is_delta = true;
		Real pdf_bsdf2 = 1.0;
		Intersection prev_its;
		while (depth < m_max_depth){
			auto its = Intersection();
			depth++;
			if (current_ray.d.x != current_ray.d.x){
				return contrib;
			}
        	if (scene.hit(current_ray, its)){
				Frame frame = Frame(its.n);
				Vec3 w_o = frame.to_local(-current_ray.d);
				if (its.material->is_air_density_change() && depth == 1){
					Vec3 origin = its.p;
					Vec3 dir = current_ray.d;
					Ray new_ray = Ray(origin, dir);
					auto ray_through = trace_mirage(new_ray, scene, sampler);
					current_ray = std::get<0>(ray_through);
					throughput *= std::get<1>(ray_through);
					contrib += std::get<2>(ray_through);
					depth++;
					continue;
				}

				if (!its.material->have_emission()){
					is_delta = its.material->have_delta(its.uv, its.p);
					if(auto hit_result = its.material->sample(w_o, its.uv, its.p, sampler.next2d())) {
						if (hit_result->transparent){
							depth--;
							is_delta = true;
						}
						else{
							pdf_bsdf2 = its.material->pdf(w_o, hit_result->wi, its.uv, its.p);
							// NEE
							if (depth < m_max_depth - 1 && (!hit_result->transparent))
								contrib += throughput * compute_direct(its, w_o, scene, sampler);
							throughput *= hit_result->weight;
						}
						Vec3 w_i = frame.to_world(hit_result->wi);
						if (hit_result->wi.z < 0.0){
							its.n.z = -its.n.z;
						}
						Vec3 o = its.p;
						current_ray = Ray(o, w_i);
					}
					else{
						if (depth < m_max_depth - 1 && (!hit_result->transparent))
							contrib += throughput * compute_direct(its, w_o, scene, sampler);
						return clamp(contrib, Color3(0.0), Color3(4.0));
					}
				}
				else {
					// We hit a light
					if (!is_delta){
						const Real pdf_emitter2 = scene.pdf_direct(*its.shape, current_ray.o, its.p, its.n);
						mis_w_bsdf = pdf_bsdf2 / (pdf_bsdf2 + pdf_emitter2);
					}
					else{
						mis_w_bsdf = 1.0;
					}
					Color3 result = contrib + mis_w_bsdf * throughput * its.material->emission(w_o, its.uv, its.p);
					return clamp(result, Color3(0.0), Color3(4.0));
				}
				prev_its = its;
			}
			else{
				if (!is_delta && depth > 1){
					// const Real pdf_emitter2 = scene.pdf_direct(*prev_its.shape, current_ray.o, prev_its.p, prev_its.n);
					const Real pdf_emitter2 = scene.pdf_envmap(current_ray.o, current_ray.d);
					mis_w_bsdf = pdf_bsdf2 / (pdf_bsdf2 + pdf_emitter2);
				}
				else{
					mis_w_bsdf = 1.0;
				}
				// if (current_ray.d.x != current_ray.d.x){
				// 	return contrib + Color3(0.0, 0.0, 1.0);
				// }
				Color3 result = contrib + mis_w_bsdf * throughput * scene.background(current_ray.d);
				return clamp(result, Color3(0.0), Color3(4.0));
			}
		}
		return contrib;
	}

	std::tuple<Ray, Color3, Color3> trace_mirage(Ray& r, const Scene& scene, Sampler& sampler){
		bool inside = true;
		const Real STEP = 0.2;
		const Real Z_STEP = 0.008;
		const Real clarity = 5;
		Real z = get_height(r.o, scene);
		Real n_i = get_refrac(z);
		Real n_t;
		Real z_new;
		Real sin_phi_i = normalize(r.d).z;
		Intersection its;
		const Real h_init = get_height(r.o, scene);
		int i = 0;
		while (inside && i < 1000){
			const Real step_size = ((sampler.next() - 0.5) / clarity) + STEP;
			// z_new = z + step_size;
			// n_t = get_refrac(z_new);
			// const Real phi_t = asin(n_i * sin_phi_i / n_t);
			const Real t_max = step_size;
			const Real height = get_height(r.o, scene) + 1e-5;
			const Real h = h_init / height;
			// spdlog::info("{}", h);
			const Vec3 d_new = normalize(Vec3{r.d.x, r.d.y, r.d.z + Z_STEP * h});
			r.d = d_new;
			r.tmax = step_size;
			if (scene.hit(r, its)){
				r.tmax = std::numeric_limits<Real>::max();
				r.o = its.p;
				if (its.material->is_air_density_change()){
					return {r, Color3(1.0), Color3(0.0)};
				}
				else{
					Frame frame = Frame(its.n);
					Vec3 w_o = frame.to_local(-r.d);
					Color3 throughput = Color3(1.0);
					Color3 contrib = Color3(0.0);
					if(auto hit_result = its.material->sample(w_o, its.uv, its.p, sampler.next2d())) {
						contrib += throughput * compute_direct(its, w_o, scene, sampler);
						throughput = hit_result->weight;
						Vec3 w_i = frame.to_world(hit_result->wi);
						if (hit_result->wi.z < 0.0){
							its.n.z = -its.n.z;
						}
						r.d = w_i;
					}
					return {r, throughput, contrib};
				}
			}
			r.o = r.d * t_max + r.o;
			// z = z_new;
			// n_i = n_t;
			// sin_phi_i = sin(phi_t);
			i++;
			inside = height < 30.0;
		}
		r.tmax = std::numeric_limits<Real>::max();
		return {r, Color3(1.0, 1.0, 1.0), Color3(0.0)};
	}

	Real get_height(const Vec3& p, const Scene& scene) const{
		Ray ray = {p, {0.0, 0.0, -1.0}};
		auto its = Intersection();
		if (scene.hit(ray, its)){
			return its.t;
		}
		return 0.0;
	}

	Real get_refrac(const Real& z) const{
		return 1.0 + z * 0.1;
	}
};