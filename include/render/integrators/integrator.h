#pragma once

#include <render/fwd.h>
#include <render/vec.h>
#include <render/image.h>
#include <render/json.h>
class Scene; // Forward declaration

/// Abstract class for integrating
class Integrator {
public:
    virtual ~Integrator() {}
    /// Generate an image by integrating the incomming light
    virtual Image render(const Scene &scene, Sampler& sampler) = 0;
};

/// Abstract class for integrator integrating
/// one pixel at a time
class SamplerIntegrator: public Integrator {
    /// Method if want to perform a preprocessing step
    virtual void preprocess(const Scene &scene, Sampler &sampler) {}
    /// Generate an image by integrating the incomming light
    Image render(const Scene &scene, Sampler& sampler) override;
    /// Method that estimate the incomming light for a given ray
    /// This method need to be implemented by childs
    /// This method is called inside the render function 
    virtual Color3 Li(Ray& ray, const Scene& scene, Sampler& sampler) = 0;
};

std::shared_ptr<Integrator> create_integrator(const json& param);