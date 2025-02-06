#pragma once

#include <render/materials/material.h>
#include <render/samplers/sampler.h>
#include <render/json.h>

class DiffuseEmit : public Material {
	Texture<Color4> radiance;

public:
	DiffuseEmit(const json& param = json::object()){
		radiance = create_texture_color(param, "radiance", Color4(1.0));
	}

    Color3 emission(const Vec3& wo, const Vec2& uv, const Vec3& p) const override {
		if(wo.z > 0.0) {
        	return eval(radiance, uv, p).xyz();
		} else {
			return {0.0, 0.0, 0.0};
		}
    }

	bool have_emission() const override {
        return true;
    }

	virtual Vec3 evaluate(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
        return Vec3(0.0);
    }

    virtual double pdf(const Vec3& wo, const Vec3& wi, const Vec2& uv, const Vec3& p) const override {
        return 0.0;
    }
};