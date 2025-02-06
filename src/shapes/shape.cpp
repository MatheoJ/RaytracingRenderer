#include <render/shapes/shape.h>
#include <render/shapes/sphere.h>
#include <render/shapes/quad.h>
#include <render/shapes/mesh.h>
#include <render/shapes/triangle.h>
#include <render/shapes/instance.h>
#include "render/filesystem.h"


std::shared_ptr<Shape> create_shape(std::shared_ptr<Material> mat, const json& param, const bool m_use_instancing) {

    static std::unordered_map<std::string, std::shared_ptr<Mesh>> mesh_cache;


    if(param.count("type") == 0) {
        throw Exception("Need to specify 'type' variable to create the shape.\n{}.", param.dump(4));
    }

    std::string type = param["type"];
    try {
        if (type == "quad") {
            return std::make_shared<Quad>(mat, param);
        } else if (type == "sphere") {
            return std::make_shared<Sphere>(mat, param);
        } else if(type == "mesh") {
            std::string filename = FileResolver::resolve(param["filename"]).string();
            
            // Check if the mesh is already loaded
            std::shared_ptr<Mesh> mesh;
            if (m_use_instancing && mesh_cache.find(filename) != mesh_cache.end()) {
                // Mesh is already loaded
                mesh = mesh_cache[filename];
                Transform transform2 = param.value<Transform>("transform", Transform());

                spdlog::info("Creating instances of mesh: {}", filename);


                Transform transform = transform2 * mesh->transform().inv();
                return std::make_shared<Instance>(mesh, transform, mat);
            }
            else {
                mesh = std::make_shared<Mesh>(mat, param);
                mesh_cache[filename] = mesh;
                return mesh;
            }            
        } else if(type == "triangle") {
            return std::make_shared<Triangle>(mat, param);
        } else {
            throw Exception("Unknow shape type: {}", type);
        }
    }
    catch (const std::exception &e)
    {
        throw Exception("Cannot create a '{}' here:\n{}.\n\t{}", type, param.dump(4), e.what());
    }
    
}

std::shared_ptr<Shape> create_airDensityQuad(std::shared_ptr<Material> mat, const json& param) {
    return std::make_shared<Quad>(mat, param);
}

