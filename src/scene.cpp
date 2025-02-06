#include <render/scene.h>
#include <render/samplers/independent.h>
#include <render/integrators/path.h>
#include <render/shapes/shape_group.h>
#include <render/shapes/bvh.h>
#include "render/materials/material.h"
#include "render/shapes/shape.h"

thread_local size_t NUMBER_INTERSECTIONS = 0;
thread_local size_t NUMBER_TRACED_RAYS = 0;

Scene::Scene(const json& param, int nb_threads) {
    m_nb_threads = nb_threads;

    // Read the scene to create the camera, materials and shapes

    // Create camera 
    if(param.contains("camera")) {
        m_camera = std::make_shared<CameraPerspective>(param["camera"]);
    } else {
        // Otherwise default camera
        m_camera = std::make_shared<CameraPerspective>();
    }

    // Create sampler
    if(param.contains("sampler")) {
        m_sampler = create_sampler(param["sampler"]);
    } else {
        m_sampler = std::make_shared<Independent>();
    }
    m_sampler->r.seed((uint64_t)5, (uint64_t)23423535);

    // Create integrator
    if (param.contains("integrator")) {
        m_integrator = create_integrator(param["integrator"]);
    }
    else {
        m_integrator = std::make_shared<PathIntegrator>();
    }

    //Set m_use_instancing to false by default
    if (param.contains("instancing")) {
        m_use_instancing = param["instancing"].get<bool>();
    }
    else {
        m_use_instancing = false;
    }

    // Create all the materials
    if(param.contains("materials")) {
        auto jmats = param["materials"];
        if(!jmats.is_array()) {
            throw Exception("Materials needs to be specified as an array\n\t{}", jmats.dump(4));
        } 
        for (auto &jmat : jmats) {
            if(!jmat.contains("name")) {
                throw Exception("Materials need to have a name\n\t{}", jmat.dump(4));
            }
            auto name = jmat["name"];
            m_materials[name] = create_material(jmat);
        }
    }

    // Create all shapes
    std::string accel_type = "linear";
    if (param.contains("accelerator")) {
        accel_type = param["accelerator"].value("type", "linear");
        spdlog::info("accel_type: {}", accel_type);
    }

    if(accel_type == "bvh") {
        m_root = std::make_shared<BVH>(param["accelerator"]);
    } else if(accel_type == "linear") {
        m_root = std::make_shared<ShapeGroup>();
    } else {
        spdlog::warn("Invalid accelerator type {}, use linear", accel_type);
        m_root = std::make_shared<ShapeGroup>();
    }

    if (param.contains("shapes")) {
        auto jshapes = param["shapes"];
        if (!jshapes.is_array()) {
            throw Exception("Shapes needs to be specified as an array\n\t{}", jshapes.dump(4));
        }

        std::unordered_map<std::string, std::shared_ptr<Shape>> mesh_cache;
        for (auto& jshape : jshapes) {
            std::shared_ptr<Material> mat; 
            if (!jshape.contains("material")) {
                spdlog::warn("Shape need to have a material\n\t{}", jshape.dump(4));
                mat = create_material(json{{"type", "diffuse"}});
            } else {
                std::string material_name = jshape["material"];
                if (m_materials.find(material_name) == m_materials.end()) {
                    throw Exception("Impossible to found a material named: {}", material_name);
                }
                mat = m_materials[material_name];
            }
            
            // Create main shape
            auto s = create_shape(mat, jshape, m_use_instancing);
            // Generate children (if any)
            auto shapes = s->children(s);
            if (shapes.empty()) {
                m_root->add_shape(s);
            } else {
                for (auto shape : shapes) {
                    m_root->add_shape(shape);
                }
            }
        }
    }

    if (param.contains("airDensityShift")) {

        m_has_air_density_shift = true;

        auto jairDensityShift = param["airDensityShift"];

        double max_height = jairDensityShift.value("max_height", 1000.0);
        Vec2 quad_half_size = jairDensityShift.value<Vec2>("quad_half_size", { 1., 1. }) / 2.0;
        Transform transform = jairDensityShift.value<Transform>("quadSize", Transform());

        auto jlayers = jairDensityShift["layers"];
        if (!jlayers.is_array()) {
            throw Exception("layers needs to be specified as an array\n\t{}", jlayers.dump(4));
        }

        int nb_layer = jlayers.size();

        double layer_height = max_height / (double)nb_layer;

        double previous_eta = 1.0;

        std::string matName = "airDensityShift";
        int layerCount = 0;

        json new_translate = { {"translate", {0, 0, 0}} };
        jairDensityShift["transform"].insert(jairDensityShift["transform"].begin(), new_translate);

        for (auto jlayer : jlayers)
        {
            double eta = jlayer.value("ratioEta", 1.0);
            std::string matName = fmt::format("airDensityShift_{}", layerCount);
            double height = max_height - layerCount * layer_height;

            if (jlayer.contains("height")) {
                height = jlayer["height"];
            }

            jairDensityShift["transform"][0]["translate"][2] = height;
            
            m_materials[matName] = create_airDensityMaterial(eta);
            previous_eta = eta;
            auto s = create_airDensityQuad(m_materials[matName], jairDensityShift);
            m_root->add_shape(s);

            layerCount++;
            //Log the layer
            spdlog::info("layer {} : eta = {}, height = {}", layerCount, eta, height);
            //Log the transform 
            spdlog::info("transform : {}", jairDensityShift["transform"].dump(4));
        }
    }

    m_root->build();
    spdlog::info("Defining background ...");
    m_background = std::make_shared<EnvironmentMap>(param, maxelem(root().aabb().diagonal()));
    //m_background_ptr = std::shared_ptr<EnvironmentMap>(&m_background);
    spdlog::info("Envmap built ...");
    // m_root->m_emitters.push_back(std::shared_ptr<EnvironmentMap>(&m_background));
    m_root->add_shape(m_background);
    spdlog::info("Envmap added ...");
}

json create_sphere_scene() {
    std::string scene = R"({
    "camera": {
        "transform": {
            "o": [0, 0, 4]
        },
        "vfov": 45,
        "resolution": [640, 480]
    },
    "background": [
        1, 1, 1
    ],
    "sampler": {
        "type": "independent",
        "samples": 1
    },
    "materials": [
        {
            "name": "mat_sphere",
            "type": "diffuse",
            "albedo": [0.6, 0.4, 0.4]
        },
        {
            "name": "mat_plane",
            "type": "diffuse",
            "albedo": [0.75, 0.75, 0.75]
        }
    ],
    "shapes": [
        {
            "type": "sphere",
            "radius": 1,
            "material": "mat_sphere"
        }, 
        {
            "type": "quad",
            "transform": {
                "o": [
                    0, -1, 0
                ],
                "x": [
                    1, 0, 0
                ],
                "y": [
                    0, 0, -1
                ],
                "z": [0, 1, 0]
            },
            "size": 100,
            "material": "mat_plane"
        }
        ]
    })";
    return json::parse(scene);

}

/// Create the scene used in the class
json create_three_sphere_class() {
    std::string scene = R"({
        "camera": {
            "transform": {
                "o": [0.7, 0, 0]
            },
            "vfov": 90,
            "resolution": [640, 480]
        },
        "background": [
            0, 0, 0
        ],
        "sampler": {
            "type": "independent",
            "samples": 1
        },
        "materials" : [
            {
                "name": "glass",
                "type": "dielectric",
                "eta_int": 1.5
            },
            {
                "name": "wall",
                "type": "diffuse"
            }, 
            {
                "name": "light",
                "type": "diffuse_light"
            },
            {
                "name": "red ball",
                "type": "metal", 
                "roughness": 0.1,
                "ks" : [0.9, 0.1, 0.1]
            },
            {
                "name" : "blue ball",
                "type" : "diffuse", 
                "albedo" : [0.1, 0.1, 0.9]
            }
        ],
        "shapes" : [
            {
                "type": "quad",
                "size": [100.0, 100.0], 
                "transform": [
                    {"angle": -90, "axis": [1, 0, 0]}, 
                    {"translate": [0.0, -1.0, 0.0]}
                ],
                "material": "wall"
            }, 
            {
                "type": "quad",
                "size": [100.0, 100.0], 
                "transform": [
                    {"translate": [0.0, 0.0, -10.0]}
                ],
                "material": "wall"
            }, 
            {
                "type": "quad",
                "size": [20.0, 20.0], 
                "transform": [
                    {"angle": 90, "axis": [1, 0, 0]}, 
                    {"translate": [0.0, 10.0, 0.0]}
                ],
                "material": "light"
            }, 
            {
                "type" : "sphere",
                "transform" : {
                    "o" : [0.0, 0.0, -2.0]
                },
                "material" : "red ball"
            },
            {
                "type" : "sphere",
                "transform" : {
                    "o" : [1.8, -0.2, -2.2]
                },
                "radius" : 0.8,
                "material" : "glass"
            },
            {
                "type" : "sphere",
                "transform" : {
                    "o" : [-1.5, -0.5, -1.5]
                },
                "radius" : 0.5,
                "material" : "blue ball"
            }
        ]
    })";

    return json::parse(scene); 
}


// Create the scene from peter shirley, book 1
json create_peter_shirley_scene(bool depth_of_field) {
    pcg32 rng = pcg32();

    json j;

    // Compose the camera
    j["camera"] = {{"transform", {{"from", {13, 3, 2}}, {"to", {0, 0, 0}}, {"up", {0, 0, 1}}}},
                   {"vfov", 20},
                   {"fdist", depth_of_field ? 10.0 : 1},
                   {"aperture", depth_of_field ? 0.1 : 0.0},
                   {"resolution", {600, 400}}};

    // compose the image properties
    j["sampler"]["samples"] = 1;
    j["sampler"]["type"] = "independent";
    j["background"]         = {1, 1, 1};

    // ground plane
    j["materials"] += {
        {"name", "ground"},
        {"type", "diffuse"}, 
        {"albedo", {0.5, 0.5, 0.5}}
    };
    j["shapes"] +=
        {{"type", "quad"},
         {"size", {25, 25}},
         {"transform",
          {{"o", {0.0, 0.0, 0.0}}, {"x", {1.0, 0.0, 0.0}}, {"y", {0.0, -1.0, 0.0}}, {"z", {0.0, 0.0, 1.0}}}},
         {"material", "ground"}};

    j["materials"] += {{"name", "glass"}, {"type", "dielectric"}, {"eta_int", 1.5}};
    int mat_id = 0;
    for (int a = -11; a < 11; a++)
    {
        for (int b = -11; b < 11; b++)
        {
            float choose_mat = rng.nextFloat();
            float r1 = rng.nextFloat();
            float r2 = rng.nextFloat();
            Vec3 center(a + 0.9f * r1, b + 0.9f * r2, 0.2f);
            if (length(center - Vec3(4.0f, 0.0f, 0.2f)) > 0.9f)
            {
                std::string mat_name = fmt::format("material_{}", mat_id);
                mat_id += 1;
                json sphere = {{"type", "sphere"}, {"radius", 0.2f}, {"transform", {{"translate", center}}},
                    {"material", mat_name}
                };

                if (choose_mat < 0.8)
                { // diffuse
                    float r1 = rng.nextFloat();
                    float r2 = rng.nextFloat();
                    float r3 = rng.nextFloat();
                    float r4 = rng.nextFloat();
                    float r5 = rng.nextFloat();
                    float r6 = rng.nextFloat();
                    Color3 albedo(r1*r2, r3*r4, r5*r6);
                    j["materials"] += {
                        {"name", mat_name},
                        {"type", "diffuse"}, 
                        {"albedo", albedo}
                    };
                }
                else if (choose_mat < 0.95)
                { // metal
                    float r1 = rng.nextFloat();
                    float r2 = rng.nextFloat();
                    float r3 = rng.nextFloat();
                    float r4 = rng.nextFloat();
                    Color3 albedo(0.5f * (1 + r1), 0.5f * (1.0f + r2), 0.5f * (1.0f + r3));
                    float   rough      = 0.5f * r4;
                    j["materials"] += {{"name", mat_name}, {"type", "metal"}, {"ks", albedo}, {"roughness", rough}};
                }
                else
                { // glass
                    sphere["material"] = "glass";
                }

                j["shapes"] += sphere;
            }
        }
    }


    j["shapes"] += {{"type", "sphere"},
                      {"radius", 1.f},
                      {"transform", {{"translate", {0, 1, 0}}}},
                      {"material", "glass"}};
    j["materials"] += {{"name", "big_mat_1"}, {"type", "diffuse"}, {"albedo", {0.4, 0.2, 0.1}}};
    j["shapes"] += {{"type", "sphere"},
                      {"radius", 1.f},
                      {"transform", {{"translate", {-4, 1, 0}}}},
                      {"material", "big_mat_1" }};
    j["materials"] += {{"name", "big_mat_2"}, {"type", "metal"}, {"ks", {0.7, 0.6, 0.5}}, {"roughness", 0.0}};
    j["shapes"] += {{"type", "sphere"},
                      {"radius", 1.f},
                      {"transform", {{"translate", {4, 1, 0}}}},
                      {"material", "big_mat_2"}};

    return j;
}

json create_example_scene(int id) {
    switch(id) {
        case 0:
            return create_sphere_scene();
        case 1:
            return create_peter_shirley_scene(false);
        case 2:
            return create_peter_shirley_scene(true);
        case 3:
            return create_three_sphere_class();
        default:
            throw Exception("Wrong example scene id ({}). Need to be between 0 to 3");
    }
}