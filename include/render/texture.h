#pragma once

#include <variant>

#include "render/vec.h"
#include "render/array2d.h"
#include "render/json.h"

/// Structure decrivant une texture constante
template <typename T>
struct ConstantTexture {
    T value; //< La valeur constante
};

/// Structure devriant une texture composee d'une
/// image 2D
template <typename T>
struct ImageTexture {
    Array2d<T> image; //< Image 2d contant la texture
    Vec2 uv_scale;    //< Le facteur d'echelle applique aux coordonnees UV (par default: [1, 1])
    Vec2 uv_offset;   //< La translation applique aux coordonnees UV (par default: [0, 0])
    double scale;     //< Mise a l'echelle des valeurs contenu de la texture (par default: 1.0)
};

/// Texture procedural sous forme de damier
template <typename T>
struct CheckerboardTexture2D {
    T color0; //< Couleur 0 du damier
    T color1; //< Couleur 1 du damier
    Vec2 uv_scale;  //< Le facteur d'echelle applique aux coordonnees UV (par default: [1, 1])
    Vec2 uv_offset;  //< La translation applique aux coordonnees UV (par default: [0, 0])
};

/// Texture procedural sous forme de damier
template <typename T>
struct CheckerboardTexture3D {
    T color0; //< Couleur 0 du damier
    T color1; //< Couleur 1 du damier
    Transform transform; //< Transformation de la texture
};

/// Type composite contenant les differents type de textures
/// ici on utilise les templates car les textures peuvent contenir
/// soit des couleurs (Color3) ou des valeurs reelles (double)
template <typename T>
using Texture = std::variant<ConstantTexture<T>, ImageTexture<T>, CheckerboardTexture2D<T>, CheckerboardTexture3D<T>>;

/// Structure permettant du "polymorphisme"
template <typename T>
struct eval_texture_op {
    T operator()(const ConstantTexture<T> &t) const;
    T operator()(const ImageTexture<T> &t) const;
    T operator()(const CheckerboardTexture2D<T> &t) const;
    T operator()(const CheckerboardTexture3D<T> &t) const;
    const Vec2 &uv;
    const Vec3 &p;
};

/// Evaluation d'une texture constante
template <typename T>
T eval_texture_op<T>::operator()(const ConstantTexture<T> &t) const {
    return t.value;
}

/// Evaluation d'une texture basee image (2D)
template <typename T>
T eval_texture_op<T>::operator()(const ImageTexture<T> &t) const {
    Vec2 uv_image = Vec2(modulo(uv.x * (t.image.width() - 1) * t.uv_scale.x + t.uv_offset.x, t.image.width() - 1),
                         modulo(uv.y * (t.image.height() - 1) * t.uv_scale.y + t.uv_offset.y, t.image.height() - 1));
    // uv_image = uv_image * t.uv_scale - t.uv_offset;
    int x = int(uv_image.x);
    int y = int(uv_image.y);

    T p_00 = t.image.at(x, y);
    T p_10 = t.image.at(x + 1, y);
    T p_01 = t.image.at(x, y + 1);
    T p_11 = t.image.at(x + 1, y + 1);

    T inter_down = lerp(p_00, p_10, uv_image.x - x);
    T inter_up = lerp(p_10, p_11, uv_image.x - x);
    T result = lerp(inter_down, inter_up, uv_image.y - y);
    return result * t.scale;
}

/// Texture procedural (damier)
template <typename T>
T eval_texture_op<T>::operator()(const CheckerboardTexture2D<T> &t) const {
    Vec2 uv_check = Vec2(uv.x * t.uv_scale.x + t.uv_offset.x,
                         uv.y * t.uv_scale.y + t.uv_offset.y);
    int a = floor(uv_check.x);
    int b = floor(uv_check.y);
    if ((a + b) % 2 == 0){
        return t.color0;
    }
    return t.color1;
}

template <typename T>
T eval_texture_op<T>::operator()(const CheckerboardTexture3D<T> &t) const {
    Vec3 pos = mul(t.transform.m, Vec4(p, 1.0)).xyz();
    int a = floor(pos.x);
    int b = floor(pos.y);
    int c = floor(pos.z);
    if ((a + b + c) % 2 == 0){
        return t.color0;
    }
    return t.color1;
}

/// Evaluation du type composite de texture
template <typename T>
T eval(const Texture<T> &texture, const Vec2 &uv, const Vec3& p) {
    // Documentation std::visit: https://en.cppreference.com/w/cpp/utility/variant/visit
    return std::visit(eval_texture_op<T>{uv, p}, texture);
}

// /// Structure permettant du "polymorphisme"
// struct eval_texture4_op {
//     Color4 operator()(const ConstantTexture<Color4> &t) const;
//     Color4 operator()(const ImageTexture<Color4> &t) const;
//     Color4 operator()(const CheckerboardTexture2D<Color4> &t) const;
//     Color4 operator()(const CheckerboardTexture3D<Color4> &t) const;
//     const Vec2 &uv;
//     const Vec3 &p;
// };

// /// Evaluation d'une texture constante
// Color4 eval_texture4_op::operator()(const ConstantTexture<Color4> &t) const {
//     return t.value;
// }

// /// Evaluation d'une texture basee image (2D)
// Color4 eval_texture4_op::operator()(const ImageTexture<Color4> &t) const {
//     Vec2 uv_image = Vec2(modulo(uv.x * (t.image.width() - 1) * t.uv_scale.x + t.uv_offset.x, t.image.width() - 1),
//                          modulo(uv.y * (t.image.height() - 1) * t.uv_scale.y + t.uv_offset.y, t.image.height() - 1));
//     // uv_image = uv_image * t.uv_scale - t.uv_offset;
//     int x = int(uv_image.x);
//     int y = int(uv_image.y);

//     Color4 p_00 = t.image.at(x, y);
//     Color4 p_10 = t.image.at(x + 1, y);
//     Color4 p_01 = t.image.at(x, y + 1);
//     Color4 p_11 = t.image.at(x + 1, y + 1);

//     Color4 inter_down = lerp(p_00, p_10, uv_image.x - x);
//     Color4 inter_up = lerp(p_10, p_11, uv_image.x - x);
//     Color4 result = lerp(inter_down, inter_up, uv_image.y - y);
//     return result * t.scale;
// }

// /// Texture procedural (damier)
// Color4 eval_texture4_op::operator()(const CheckerboardTexture2D<Color4> &t) const {
//     Vec2 uv_check = Vec2(uv.x * t.uv_scale.x + t.uv_offset.x,
//                          uv.y * t.uv_scale.y + t.uv_offset.y);
//     int a = floor(uv_check.x);
//     int b = floor(uv_check.y);
//     if ((a + b) % 2 == 0){
//         return t.color0;
//     }
//     return t.color1;
// }

// Color4 eval_texture4_op::operator()(const CheckerboardTexture3D<Color4> &t) const {
//     Vec3 pos = mul(t.transform.m, Vec4(p, 1.0)).xyz();
//     int a = floor(pos.x);
//     int b = floor(pos.y);
//     int c = floor(pos.z);
//     if ((a + b + c) % 2 == 0){
//         return t.color0;
//     }
//     return t.color1;
// }

// /// Evaluation du type composite de texture
// Color4 eval_color4(const Texture<Color4> &texture, const Vec2 &uv, const Vec3& p) {
//     // Documentation std::visit: https://en.cppreference.com/w/cpp/utility/variant/visit
//     return std::visit(eval_texture4_op{uv, p}, texture);
// }

template <typename T>
struct is_imageop {
    bool operator()(const ConstantTexture<T> &t) const;
    bool operator()(const ImageTexture<T> &t) const;
    bool operator()(const CheckerboardTexture2D<T> &t) const;
    bool operator()(const CheckerboardTexture3D<T> &t) const;
};

/// Evaluation d'une texture constante
template <typename T>
bool is_imageop<T>::operator()(const ConstantTexture<T> &t) const {
    return false;
}

/// Evaluation d'une texture basee image (2D)
template <typename T>
bool is_imageop<T>::operator()(const ImageTexture<T> &t) const {
    return true;
}

/// Texture procedural (damier)
template <typename T>
bool is_imageop<T>::operator()(const CheckerboardTexture2D<T> &t) const {
    return false;
}

template <typename T>
bool is_imageop<T>::operator()(const CheckerboardTexture3D<T> &t) const {
    return false;
}

/// Evaluation du type composite de texture
template <typename T>
bool is_image(const Texture<T> &texture) {
    // Documentation std::visit: https://en.cppreference.com/w/cpp/utility/variant/visit
    return std::visit(is_imageop<T>{}, texture);
}

template <typename T>
struct get_imageop {
    Array2d<T> operator()(const ConstantTexture<T> &t) const;
    Array2d<T> operator()(const ImageTexture<T> &t) const;
    Array2d<T> operator()(const CheckerboardTexture2D<T> &t) const;
    Array2d<T> operator()(const CheckerboardTexture3D<T> &t) const;
};

/// Evaluation d'une texture constante
template <typename T>
Array2d<T> get_imageop<T>::operator()(const ConstantTexture<T> &t) const {
    return {};
}

/// Evaluation d'une texture basee image (2D)
template <typename T>
Array2d<T> get_imageop<T>::operator()(const ImageTexture<T> &t) const {
    return t.image;
}

/// Texture procedural (damier)
template <typename T>
Array2d<T> get_imageop<T>::operator()(const CheckerboardTexture2D<T> &t) const {
    return {};
}

template <typename T>
Array2d<T> get_imageop<T>::operator()(const CheckerboardTexture3D<T> &t) const {
    return {};
}

/// Evaluation du type composite de texture
template <typename T>
Array2d<T> get_image(const Texture<T> &texture) {
    // Documentation std::visit: https://en.cppreference.com/w/cpp/utility/variant/visit
    return std::visit(get_imageop<T>{}, texture);
}

template <typename T>
struct get_scaleop {
    Real operator()(const ConstantTexture<T> &t) const;
    Real operator()(const ImageTexture<T> &t) const;
    Real operator()(const CheckerboardTexture2D<T> &t) const;
    Real operator()(const CheckerboardTexture3D<T> &t) const;
};

/// Evaluation d'une texture constante
template <typename T>
Real get_scaleop<T>::operator()(const ConstantTexture<T> &t) const {
    return 1.0;
}

/// Evaluation d'une texture basee image (2D)
template <typename T>
Real get_scaleop<T>::operator()(const ImageTexture<T> &t) const {
    return t.scale;
}

/// Texture procedural (damier)
template <typename T>
Real get_scaleop<T>::operator()(const CheckerboardTexture2D<T> &t) const {
    return 1.0;
}

template <typename T>
Real get_scaleop<T>::operator()(const CheckerboardTexture3D<T> &t) const {
    return 1.0;
}

/// Evaluation du type composite de texture
template <typename T>
Real get_scale(const Texture<T> &texture) {
    // Documentation std::visit: https://en.cppreference.com/w/cpp/utility/variant/visit
    return std::visit(get_scaleop<T>{}, texture);
}

/// Methode pour creer une texture contenant une couleur
Texture<Color4> create_texture_color(const json& param, const std::string& name, const Color4& default_value);
/// Methode pour creer une texture contenant une valeur reelle
Texture<double> create_texture_float(const json& param, const std::string& name, const double default_value);
