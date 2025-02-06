#include <render/texture.h>
#include <render/common.h>
#include <render/image.h>
#include <render/filesystem.h>
#include <pcg32.h>

Texture<Color4> create_texture_color(const json& param, const std::string& name, const Color4& default_value) {
    if(!param.contains(name)) {
        // The parameter do not exist inside the json
        // return the default object (constant) with default value
        return ConstantTexture<Color4>{
            default_value
        };
    }
    
    const json& j = param[name];
    if (j.is_string()) {
        Image im;
        std::string filename = FileResolver::resolve(j.get<std::string>()).string();
        if (!im.load(filename)) {
            throw Exception("Impossible to open image: {}", filename);
        }
        im.flip_vertically();
        return ImageTexture<Color4>{
                im,
                Vec2(1.0),
                Vec2(0.0),
                1.0
        };
    }
    else if (j.is_array() || j.is_number_float()) {
        Color3 value = j.get<Color3>();
        return ConstantTexture<Color4>{
            {value, 1.0}
        };
    }


    std::string type = "constant";
    if(j.contains("type")) {
        type = j["type"];
    }

    try {
        if(type == "constant") {
            Color3 value = j.value("value", Color3(1.0));
            return ConstantTexture<Color4>{
                {value, 1.0}
            };
        } else if(type == "texture") {
            Image im;
            bool gamma = j.value("gamma", true);
            std::string filename = FileResolver::resolve(j["filename"]).string();
            if(!im.load(filename, gamma)) {
                throw Exception("Impossible to open image: {}", filename); 
            }
            spdlog::info("Load: {}", filename);
            Vec2 uv_scale = j.value("uv_scale", Vec2{1.0, 1.0});
            Vec2 uv_offset = j.value("uv_offset", Vec2{0.0, 0.0});
            double scale = j.value("scale", 1.0);

            bool flip = j.value("vflip", true);
            if(flip) {
                im.flip_vertically();
            }

            return ImageTexture<Color4>{
                im,
                uv_scale,
                uv_offset,
                scale
            };
        } else if(type == "checkerboard2d") {
            Color4 color1 = j.value("color1", Color4{0.8, 0.8, 0.8, 1.0});
            Color4 color2 = j.value("color2", Color4{0.2, 0.2, 0.2, 1.0});
            Vec2 scale = j.value("uv_scale", Vec2{1.0, 1.0});
            Vec2 offset = j.value("uv_offset", Vec2{0.0, 0.0});

            return CheckerboardTexture2D<Color4>{
                color1, 
                color2, 
                scale, 
                offset
            };
        } else if(type == "checkerboard3d") {
            Color4 color1 = j.value("color1", Color4{0.8, 0.8, 0.8, 1.0});
            Color4 color2 = j.value("color2", Color4{0.2, 0.2, 0.2, 1.0});
            Transform transform = j.value("transform", Transform());

            return CheckerboardTexture3D<Color4>{
                color1, 
                color2, 
                transform
            };
        } else {
            throw Exception("Invalid texture type '{}' here:\n{}", type, j.dump(4));
        }
    }
    catch (const std::exception &e)
    {
        throw Exception("Cannot create a '{}' here:\n{}.\n\t{}", type, param.dump(4), e.what());
    }
}

Texture<double> create_texture_float(const json& param, const std::string& name, const double default_value) {
    if(!param.contains(name)) {
        // The parameter do not exist inside the json
        // return the default object (constant) with default value
        return ConstantTexture<double>{
            default_value
        };
    }

    const json& j = param[name];   
    if (j.is_string()) {
        Image im;
        std::string filename = FileResolver::resolve(j.get<std::string>()).string();
        if (!im.load(filename)) {
            throw Exception("Impossible to open image: {}", filename);
        }
        im.flip_vertically();
        Array2d<double> im_float(im.sizeX(), im.sizeY());
        for (auto x : range(im.width()))
            for (auto y : range(im.height()))
                im_float.at(x, y) = luminance(im.at(x, y));

        return ImageTexture <double > {
            im_float,
            Vec2(1.0),
            Vec2(0.0),
            1.0
        };
    }
    else if (j.is_number_float()) {
        double value = j.get<double>();
        return ConstantTexture<double>{
            value
        };
    }

    std::string type = "constant";
    if(j.contains("type")) {
        type = j["type"];
    }

    try {
        if(type == "constant") {
            double value = j.get<double>();
            return ConstantTexture<double>{
                value
            };
        } else if(type == "texture") {
            Image im;
            std::string filename = FileResolver::resolve(j["filename"]).string();
            if(!im.load(filename)) {
                throw Exception("Impossible to open image: {}", filename); 
            }

            bool flip = j.value("vflip", true);
            if(flip) {
                im.flip_vertically();
            }

            Array2d<double> im_float(im.sizeX(), im.sizeY());
            for (auto x : range(im.width()))
                for (auto y : range(im.height()))
                    im_float.at(x, y) = luminance(im.at(x, y));

            Vec2 uv_scale = j.value("uv_scale", Vec2{ 1.0, 1.0 });
            Vec2 uv_offset = j.value("uv_offset", Vec2{ 0.0, 0.0 });
            double scale = j.value("scale", 1.0);

            return ImageTexture<double>{
                im_float,
                uv_scale,
                uv_offset,
                scale
            };
        } else if(type == "checkerboard") {
            double color1 = j.value("color1", 0.8);
            double color2 = j.value("color2", 0.2);
            Vec2 scale = j.value("scale", Vec2{1.0, 1.0});
            Vec2 offset = j.value("offset", Vec2{0.0, 0.0});

            return CheckerboardTexture2D<double>{
                color1, 
                color2, 
                scale, 
                offset
            };
        } else if(type == "checkerboard3d") {
            double color1 = j.value("color1", 0.8);
            double color2 = j.value("color2", 0.2);
            Transform transform = j.value("transform", Transform());

            return CheckerboardTexture3D<double>{
                color1, 
                color2, 
                transform
            };
        } else {
            throw Exception("Invalid texture type '{}' here:\n{}", type, j.dump(4));
        }
    }
    catch (const std::exception &e)
    {
        throw Exception("Cannot create a '{}' here:\n{}.\n\t{}", type, param.dump(4), e.what());
    }
}