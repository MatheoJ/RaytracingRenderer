
#include <render/json.h>
#include <render/materials/material.h>
#include <render/materials/diffuse.h>
#include <render/materials/vMF_diffuse.h>
#include <render/materials/metal.h>
#include <render/materials/dielectric.h>
#include <render/materials/diffuse_emitter.h>
#include <render/materials/diffuse_transparent.h>
#include <render/materials/blend.h>
#include <render/materials/fresnel_blend.h>
#include <render/materials/phong.h>
#include <render/materials/airDensityShift.h>
#include <render/materials/mirage.h>


std::shared_ptr<Material> create_material(const json& param) {
    if(param.count("type") == 0) {
        throw Exception("Need to specify 'type' variable to create the material.\n{}.", param.dump(4));
    }

    std::string type = param["type"];
    try {
        if (type == "diffuse") {
            return std::make_shared<Diffuse>(param);
        } else if(type == "diffuse_transparent") {
            return std::make_shared<DiffuseTransparent>(param);
        } else if(type == "vMF_diffuse") {
            return std::make_shared<vMF_diffuse>(param);
        } else if(type == "mirage") {
            return std::make_shared<Mirage>(param);
        } else if(type == "metal") {
            return std::make_shared<Metal>(param);
        } else if(type == "dielectric") {
            return std::make_shared<Dielectric>(param);
        } else if(type == "diffuse_light") {
            return std::make_shared<DiffuseEmit>(param);
        }else if(type == "blend") {
            return std::make_shared<Blend>(param);
        }else if(type == "fresnel_blend") {
            return std::make_shared<FresnelBlend>(param);
        }else if(type == "phong") {
            return std::make_shared<Phong>(param);
        }else {
            throw Exception("Unknow BSDF type: {}", type);
        }
    }
    catch (const std::exception &e)
    {
        throw Exception("Cannot create a '{}' here:\n{}.\n\t{}", type, param.dump(4), e.what());
    }
    
}


std::shared_ptr<Material> create_airDensityMaterial(double etaExt, double etaInt) {
    return std::make_shared<AirDensityShift>(etaExt, etaInt);
}

std::shared_ptr<Material> create_airDensityMaterial(double ratio) {
    return std::make_shared<AirDensityShift>(ratio);
}
