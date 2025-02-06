#pragma once

#include <render/integrators/integrator.h>
#include <render/scene.h>

class NormalIntegrator : public SamplerIntegrator {
public:
	NormalIntegrator(const json& param = json::object()) {
	}

	Color3 Li(Ray& ray, const Scene& scene, Sampler& sampler) {
		/* Couleur de la normale si une intersection :
		(n + 1.0) * 0.5
		
		Sinon retourner la couleur noire.
		*/ 
		auto intersection = Intersection();
		if (scene.hit(ray, intersection)) {
			return (intersection.n + 1.0) * 0.5;
		}
		return Color3(0.0);
	
	}
};