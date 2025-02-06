#include <render/integrators/integrator.h>
#include <render/integrators/normal.h>
#include <render/integrators/tangent.h>
#include <render/integrators/path.h>
#include <render/integrators/path_mis.h>
#include <render/integrators/uv.h>
#include <render/integrators/direct.h>
#include <render/progress.h>

#include <nanothread/nanothread.h>


Image SamplerIntegrator::render(const Scene& scene, Sampler& sampler) {
    auto resolution = scene.camera().resolution();

    // Structure nous permettant de définir 
    // une tache de travail (faire le calcul d'une sous partie de l'image)
    struct Work {
        Image im;       //< Sous image correspondant à la charge de travail
        Vec2i offset;   //< Defini l'offset dans l'image complete
        Vec2i size;     //< Defini la taille de la sous image
        std::shared_ptr<Sampler> sampler; //< Defini un sampler par unité de travail
    };

    // Code qui va generer toutes les unités de travail
    // en fonction de la taille de l'image
    spdlog::info("Generate work units ... ");
    std::vector<Work> works = {};
    auto BLOCK_SIZE = 32;
    for (int x = 0; x < resolution.x; x += BLOCK_SIZE) {
        for(int y = 0; y < resolution.y; y+= BLOCK_SIZE) {
            Vec2i size = {
                std::min(resolution.x, x + BLOCK_SIZE) - x,
                std::min(resolution.y, y + BLOCK_SIZE) - y
            };
            works.push_back(
                Work{
                    Image(size.x, size.y),
                    {x, y},
                    size,
                    sampler.clone()
                }
            );
        }
    }
    spdlog::info("Work unit: {}", works.size());

    // La barre de progression
    ProgressBar monitor(works.size());

    // La facon de collecter les statistiques des differents processus de rendu
    std::atomic<size_t> TOTAL_NUMBER_INTERSECTIONS = 0;
    std::atomic<size_t> TOTAL_NUMBER_RAY_GENERATED = 0;
    
    // Le calcul parallel qui utilise les taches de travail
    // défini plus haut
    Pool* pool = pool_create(scene.nb_threads());
    nanothread::parallel_for(
        nanothread::blocked_range<uint32_t>(/* begin = */ 0, /* end = */ works.size(), /* block_size = */ 1),
        [&](nanothread::blocked_range<uint32_t> range_nt) {
        
        // Initialisation des statistiques
        NUMBER_INTERSECTIONS = 0;
        NUMBER_TRACED_RAYS = 0;

        // Pour toutes les blocs de travail alloués
        for (auto i = range_nt.begin(); i != range_nt.end(); ++i) {
            Work& wk = works[i];
            // Calculer l'image correspondant au bloc de travail courant
            for (int x : range(wk.size.x)) {
                for (int y : range(wk.size.y)) {
                    // Estimateur de Monte Carlo
                    Color3 avg;
                    for (auto s : range(wk.sampler->samples())) {
                        Vec2 pos_img = Vec2(x + wk.offset.x, y + wk.offset.y) + wk.sampler->next2d();
                        Ray ray = scene.camera().generate_ray(pos_img, *wk.sampler);
                        // Appel Li pour connaitre la luminance incidente au rayon
                        avg += Li(ray, scene, *wk.sampler);
                    }
                    wk.im.at(x, y) = {avg / wk.sampler->samples(), 1.0};
                }
            }
            // Mise a jour de la progression du rendu (progress bar)
            ++monitor;
        }

        // Mise a jour des statistiques de rendu
        TOTAL_NUMBER_INTERSECTIONS += NUMBER_INTERSECTIONS;
        TOTAL_NUMBER_RAY_GENERATED += NUMBER_TRACED_RAYS;
    }, pool
    );

    // Specifier que l'on a fini le rendu
    monitor.set_done();

    // Libération mémoire
    pool_destroy(pool);

    // Affichage des statisques de rendu
    spdlog::info("Stats: ");
    spdlog::info(" - #intersections: {}", TOTAL_NUMBER_INTERSECTIONS);
    spdlog::info(" - #rays(traced) : {}", TOTAL_NUMBER_RAY_GENERATED);
    spdlog::info(" - #intersections/#ray(traced): {}", TOTAL_NUMBER_INTERSECTIONS / double(TOTAL_NUMBER_RAY_GENERATED));

    // Fusion des différentes unité de travail
    // dans une seule et unique image
    spdlog::info("Merging... ");
    Image im(resolution.x, resolution.y);
    for (auto wk : works) {
        for (auto x : range(wk.size.x)) {
            for (auto y : range(wk.size.y)) {
                im.at(x + wk.offset.x, y + wk.offset.y) = wk.im.at(x, y);
            }
        }
    }
    return im;
}

std::shared_ptr<Integrator> create_integrator(const json& param) {
    if (param.count("type") == 0) {
        throw Exception("Need to specify 'type' variable to create the integrator.\n{}.", param.dump(4));
    }

    std::string type = param["type"];
    try {
        if (type == "normal") {
            return std::make_shared<NormalIntegrator>(param);
        }else if (type == "tangent") {
            return std::make_shared<TangentIntegrator>(param);
        }
        else if (type == "path") {
            return std::make_shared<PathIntegrator>(param);
        }
        else if (type == "path_mis") {
            return std::make_shared<PathMISIntegrator>(param);
        }
        else if (type == "uv") {
            return std::make_shared<UVIntegrator>(param);
        }
        else if (type == "direct") {
            return std::make_shared<DirectIntegrator>(param);
        }
        else {
            throw Exception("Unknow Integrator type: {}", type);
        }
    }
    catch (const std::exception& e)
    {
        throw Exception("Cannot create a '{}' here:\n{}.\n\t{}", type, param.dump(4), e.what());
    }
}