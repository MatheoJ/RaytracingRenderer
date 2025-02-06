#pragma once

#include <render/integrators/integrator.h>
#include <render/scene.h>

class UVIntegrator : public SamplerIntegrator {
public:
	UVIntegrator(const json& param = json::object()) {
	}

	Color3 Li(Ray& ray, const Scene& scene, Sampler& sampler) {
		auto intersection = Intersection();
		if (scene.hit(ray, intersection)) {
			return {modulo(intersection.uv.x, 1.0), modulo(intersection.uv.y, 1.0), 0.0};
		}
		return Color3(0.0);
	
	}
};