
#include <spdlog/spdlog.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <chrono>
#pragma warning(disable:4996)
#include <argparse/argparse.hpp>

#include "render/common.h"
#include "render/vec.h"
#include "render/ray.h"
#include "render/image.h"
#include "render/transform.h"
#include "render/json.h"
#include "render/shapes/shape.h"
#include "render/materials/material.h"
#include "render/samplers/sampler.h"
#include "render/materials/diffuse.h"
#include "render/scene.h"
#include "render/filesystem.h"

int main(int argc, char *argv[])
{
    argparse::ArgumentParser program("Render - MTI 870");
    program.add_argument("-i", "--input")
      .help("fichier de scene")
      .default_value<std::string>("example_scene1");
    program.add_argument("-o", "--output")
      .help("image de sortie")
      .default_value<std::string>("out.png");
    program.add_argument("-t", "--threads")
        .help("nombre de threads (-1 = auto)")
        .scan<'i', int>()
        .default_value<int>(-1);
    program.add_argument("-n", "--spp")
      .help("nombre d'echantilions")
      .scan<'i', int>()
      .default_value<int>(0);
    program.add_argument("-s", "--scale")
      .help("scale rendering image")
      .scan<'f', double>()
      .default_value<double>(1.0);
    program.add_argument("-a", "--add")
      .help("additional json (to change some rendering options)")
      .default_value<std::string>("");
    program.add_argument("-e", "--envmap")
      .help("path to envmap")
      .default_value<std::string>("");
    // Parsing des arguments
    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error &err)
    {
        spdlog::critical("Erreur pendant la lecture des arguments de la ligne de commande");
        spdlog::critical("Raison: {}", err.what());
        std::cerr << program << "\n";
        return EXIT_FAILURE;
    }

   try 
    {
        std::shared_ptr<Scene> scene = nullptr;

        using milli = std::chrono::milliseconds;


        // Open file
        auto input = program.get<std::string>("--input");
        int  scene_number = 0;
        json j;
        if (sscanf(input.c_str(), "example_scene%d", &scene_number) == 1) {
            spdlog::info("Use example scene: {}", scene_number);
            j = create_example_scene(scene_number);
        } else {
            spdlog::info("Read scene file: {}", input);
            std::ifstream stream(input, std::ifstream::in);
            if (!stream.good())
                throw Exception("Cannot open file: {}.", input);

            // Add scene directory to the filesystem
            FileResolver::append(std::filesystem::path(input).parent_path());

            // Parsing the file
            spdlog::info("Parsing ...");
            stream >> j;
        }

        auto addition_json = program.get<std::string>("--add");
        if(addition_json != "") {
            spdlog::info("Read additional json file: {}", addition_json);
            std::ifstream stream(addition_json, std::ifstream::in);
            if (!stream.good())
                throw Exception("Cannot open file: {}.", addition_json);

            // Parsing the file
            spdlog::info("Parsing ...");
            json a_j;
            stream >> a_j;

            j.merge_patch(a_j);
        }

        if (j.contains("paths")) {
            auto scene_dir = std::filesystem::path(input).parent_path();
            for (std::string p : j["paths"]) {
                FileResolver::append(scene_dir / std::filesystem::path(p));
            }
        }

        double scale = program.get<double>("--scale");
        if(scale != 1.0) {
            if(j["camera"].contains("resolution")) {
                Vec2 res = j["camera"]["resolution"];
                j["camera"]["resolution"] = res * scale;
                spdlog::info("Change resolution: {} ... ", res * scale);
            }            
        }

        spdlog::info("Filesystem: ");
        for (auto path : FileResolver::paths()) {
            spdlog::info("\t- {}", path);
        }


        int nb_threads = program.get<int>("--threads");
        if (nb_threads == 0) {
            throw Exception("Impossible to have 0 threads to perform the rendering process");
        }
        nb_threads = std::max(nb_threads, -1);

        spdlog::info("Create scene ...");
        auto start = std::chrono::high_resolution_clock::now();
        scene = std::make_shared<Scene>(j, nb_threads);
        auto finish = std::chrono::high_resolution_clock::now();
        spdlog::info("Loading time: {} ms", std::chrono::duration_cast<milli>(finish - start).count());

        int spp = program.get<int>("--spp");
        if(spp > 0) {
            // Change SPP count
            spdlog::info("Change spp number: {}", spp);
            scene->set_sample_count(spp);
        }

        start = std::chrono::high_resolution_clock::now();
        spdlog::info("Rendering ...", input);
        auto res = scene->render();

        finish = std::chrono::high_resolution_clock::now();
        spdlog::info("Rendering time: {} ms", std::chrono::duration_cast<milli>(finish - start).count());

        // Save output image
        auto output = program.get<std::string>("--output");
        spdlog::info("Save output image: {}", output);
        res.save(output);
    }
    catch (const std::exception &e)
    {
         spdlog::critical("{}", e.what());
         exit(EXIT_FAILURE);
    }
}