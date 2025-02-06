#include <render/samplers/sampler.h>
#include <render/samplers/independent.h>

std::shared_ptr<Sampler> create_sampler(const json& param) {
    if(param.count("type") == 0) {
        return std::make_shared<Independent>(param);
    }

    std::string type = param["type"];
    try {
        if (type == "independent") {
            return std::make_shared<Independent>(param);
        } else {
            throw Exception("Unknow shape type: {}", type);
        }
    }
    catch (const std::exception &e)
    {
        throw Exception("Cannot create a '{}' here:\n{}.\n\t{}", type, param.dump(4), e.what());
    }

}