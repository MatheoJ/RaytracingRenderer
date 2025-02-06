#pragma once
#include <render/shapes/shape.h>
#include <render/scene.h>
#include <render/integrators/integrator.h>

class DirectIntegrator : public SamplerIntegrator {
private:
	enum class SamplingStrategy {
		EBSDF, // Utilisation bsdf.sample(...) -- Tache 1
		ENaive, // Tache 4
		EBSDFEval, // Tache 5
		EEmitter, // Devoir 4 (prochain devoir)
		EMIS // Devoir 4 (prochain devoir)
	};
	SamplingStrategy m_strategy;

public:
    DirectIntegrator(const json& param = json::object()) {
		std::string strategy = param.value("strategy", "bsdf");
		m_strategy = SamplingStrategy::EBSDF;
		if(strategy == "bsdf") {
			m_strategy = SamplingStrategy::EBSDF;
		} else if(strategy == "naive") {
			m_strategy = SamplingStrategy::ENaive;
		} else if(strategy == "bsdf_eval") {
			m_strategy = SamplingStrategy::EBSDFEval;
		} else if(strategy == "emitter") {
			m_strategy = SamplingStrategy::EEmitter;
		} else if(strategy == "mis") {
			m_strategy = SamplingStrategy::EMIS;
		} else {
			throw Exception("Invalid strategy (DirectIntegrator)");
		}
	}

	Color3 Li(Ray& r, const Scene& scene, Sampler& sampler) {
		// votre code ici:
		// - intersection dans la scène pour trouver le premier
		// point d'intersection
		// - vérifier si l'intersection est valide, sinon couleur de fond
		// - vérifier si ce point d'intersection est sur une source de lumière
		// si oui, retourner la couleur de la source de lumière
		// - sinon, calculer l'éclairage direct (voir les différentes stratégies)
		Color3 throughput(1.0);
		int depth = 0;
		Ray current_ray = Ray(r.o, r.d);
		auto its = Intersection();
		if (scene.hit(current_ray, its)){
			Frame frame = Frame(its.n);
			Vec3 w_o = frame.to_local(-current_ray.d);
			if (its.material->have_emission()){
				return throughput * its.material->emission(w_o, its.uv, its.p);
			}
			if(m_strategy == SamplingStrategy::EBSDF) {
				if(auto hit_result = its.material->sample(w_o, its.uv, its.p, sampler.next2d())) {
					throughput *= hit_result->weight;
					Vec3 w_i = frame.to_world(hit_result->wi);
					if (hit_result->wi.z < 0.0){
						its.n.z = -its.n.z;
					}
					Vec3 o = its.p;
					current_ray = Ray(o, w_i);
					if (scene.hit(current_ray, its)){
						frame = Frame(its.n);
						w_o = frame.to_local(-current_ray.d);
						return throughput * its.material->emission(w_o, its.uv, its.p);
					}
					else{
						return throughput * scene.background(current_ray.d);
					}
				}
				else{
					return throughput * its.material->emission(w_o, its.uv, its.p);
				}
			} 
			else if (m_strategy == SamplingStrategy::ENaive) {
				Vec3 w_i;
				if (its.material->have_delta(its.uv, its.p)){
					auto hit_result = its.material->sample(w_o, its.uv, its.p, sampler.next2d());
					w_i = frame.to_world(hit_result->wi);
					throughput *= hit_result->weight;
				}
				else{
					w_i = sample_hemisphere(sampler.next2d());
					throughput *= its.material->evaluate(w_o, w_i, its.uv, its.p) / pdf_hemisphere(w_i);
					w_i = frame.to_world(w_i);
				}
				
				if (w_i.z < 0.0){
					its.n.z = -its.n.z;
				}
				Vec3 o = its.p;
				current_ray = Ray(o, w_i);
				if (scene.hit(current_ray, its)){
					frame = Frame(its.n);
					w_o = frame.to_local(-current_ray.d);
					return throughput * its.material->emission(w_o, its.uv, its.p);
				}
				else{
					return throughput * scene.background(current_ray.d);
				}
			} else if (m_strategy == SamplingStrategy::EBSDFEval) {
				if(auto hit_result = its.material->sample(w_o, its.uv, its.p, sampler.next2d())) {
					if (its.material->have_delta(its.uv, its.p)){
						throughput *= hit_result->weight;
					}
					else{
						throughput *= its.material->evaluate(w_o, hit_result->wi, its.uv, its.p) / its.material->pdf(w_o, hit_result->wi, its.uv, its.p);
					}
					Vec3 w_i = frame.to_world(hit_result->wi);
					if (hit_result->wi.z < 0.0){
						its.n.z = -its.n.z;
					}
					Vec3 o = its.p;
					current_ray = Ray(o, w_i);
					if (scene.hit(current_ray, its)){
						frame = Frame(its.n);
						w_o = frame.to_local(-current_ray.d);
						return throughput * its.material->emission(w_o, its.uv, its.p);
					}
					else{
						return throughput * scene.background(current_ray.d);
					}
				}
				else{
					return throughput * its.material->emission(w_o, its.uv, its.p);
				}
			} else if(m_strategy == SamplingStrategy::EEmitter) {
				if (auto hit_result = its.material->sample(w_o, its.uv, its.p, sampler.next2d())) {
					if (its.material->have_delta(its.uv, its.p)) {
						throughput *= hit_result->weight;

						if (scene.hit(current_ray, its)) {
							frame = Frame(its.n);
							w_o = frame.to_local(-current_ray.d);
							return throughput * its.material->emission(w_o, its.uv, its.p);
						}
						else {
							return throughput * scene.background(current_ray.d);
						}
					}
					else {
						std::tuple< EmitterSample, const Shape& > DirectSample = scene.sample_direct(its.p, sampler.next2d());
						const EmitterSample emitterSample = std::get<0>(DirectSample);
						const Shape& emitterShape = std::get<1>(DirectSample);
						const Vec3 wi = normalize(emitterSample.y - its.p);

						if (scene.visible(emitterSample.y, its.p)) {
							frame = Frame(its.n);
							throughput *= its.material->evaluate(w_o, normalize(frame.to_local(wi)), its.uv, its.p);
							frame = Frame(emitterSample.n);
							const Vec3 w_o_light = normalize(wi);

							Vec3 Le = emitterShape.evaluate_emission(w_o_light, emitterSample.uv, emitterSample.y);

							const Real pdf = emitterSample.pdf;
							if(pdf == 0.0) {
								Le = Color3(0.0); // scene.background(wi);
							}
							else{
								throughput /= pdf;
							}
							return throughput * Le;
						}					
						return Color3(0.0); //throughput * scene.background(wi);
					}
				}
				else {
					return throughput * its.material->emission(w_o, its.uv, its.p);
				}				
			} else if(m_strategy == SamplingStrategy::EMIS) {
				Vec3 contrib = Vec3(0.0);
				if (auto hit_result = its.material->sample(w_o, its.uv, its.p, sampler.next2d())) {
					std::tuple< EmitterSample, const Shape& > DirectSample = scene.sample_direct(its.p, sampler.next2d());
					const EmitterSample emitterSample = std::get<0>(DirectSample);
					const Shape& emitterShape = std::get<1>(DirectSample);
					const Vec3 wi = normalize(emitterSample.y - its.p);
					const bool is_delta = its.material->have_delta(its.uv, its.p);

					if (scene.visible(emitterSample.y, its.p) && !is_delta) {
						throughput *= its.material->evaluate(w_o, normalize(frame.to_local(wi)), its.uv, its.p);
						const Real pdf_bsdf1 = its.material->pdf(w_o, normalize(frame.to_local(wi)), its.uv, its.p);
						Frame frame_emmiter = Frame(emitterSample.n);
						const Vec3 w_o_light = normalize(frame_emmiter.to_local(-wi));

						Vec3 Le = emitterShape.evaluate_emission(w_o_light, emitterSample.uv, emitterSample.y);

						const Real pdf_emitter1 = emitterSample.pdf;
						throughput /= pdf_emitter1;
						const Real mis_w = pdf_emitter1 / (pdf_bsdf1 + pdf_emitter1);
						contrib += mis_w * throughput * Le;
					}

					Vec3 w_i = normalize(frame.to_world(hit_result->wi));
					if (hit_result->wi.z < 0.0){
						its.n.z = -its.n.z;
					}
					Vec3 o = its.p;
					current_ray = Ray(o, w_i);
					const Real pdf_bsdf2 = its.material->pdf(w_o, hit_result->wi, its.uv, its.p);
					if (scene.hit(current_ray, its)){
						Real mis_w = 1.0;
						if (!is_delta){
							const Real pdf_emitter2 = scene.pdf_direct(*its.shape, o, its.p, its.n);
							mis_w = pdf_bsdf2 / (pdf_bsdf2 + pdf_emitter2);
						}
						frame = Frame(its.n);
						w_o = frame.to_local(-current_ray.d);
						contrib += mis_w * hit_result->weight * its.material->emission(w_o, its.uv, its.p);
					}
					return contrib;
				}
				else {
					return throughput * its.material->emission(w_o, its.uv, its.p);
				}
			}
		}
		return throughput * scene.background(current_ray.d);
	}
};