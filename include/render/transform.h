#pragma once

#include "render/vec.h"
#include "render/ray.h"

struct Transform {
public:
    Mat4 m;
    Mat4 m_inv;

    /// Create identity transformation
    Transform():
        m(Identity),
        m_inv(Identity) {
    }

    /// Create a transformation based on the 4x4 matrix provided
    Transform(const Mat4& m_): 
        m(m_),
        m_inv(inverse(m_)) {
    }

    /// Create new transformation based on a given matrix
    /// and its inverse
    Transform(const Mat4& m, const Mat4& m_inv):
        m(m), m_inv(m_inv) 
    {}

    /// Return the inverse transformation
    Transform inv() const {
        return Transform(m_inv, m);
    }

    /// Concatenate with another transformation
    Transform operator*(const Transform &t) const
    {
        return Transform(mul(m, t.m), mul(t.m_inv, m_inv));
    }

    /// Apply the homogenous transformation to a 3D direction vector
    Vec3 vector(const Vec3& v) const {
        /*
        TODO: Vous devez utiliser les coordonnées homogènes pour 
        appliquer la transformation à un vecteur.  

        La multiplication d'un vecteur et d'une matrice s'effectue avec
        la fonction "mul(Mat4, Vec4)".

        Dans le cours, nous avons vu comment exprimer des vecteurs dans les coordonnées
        homogènes. Vous pouvez utiliser la méthode ".xyz()" pour extraire les 3 composantes 
        du vecteur. 
        */
        // votre_code_ici("Devoir 1: transformation pour un vecteur");
        Vec4 vec4 = Vec4{v.x, v.y, v.z, 0.0};
        Vec4 result = mul(m, vec4);
        return Vec3(result.xyz());
    }

    /// Apply the homogenous transformation to a 3D normal vector
    Vec3 normal(const Vec3& n) const {
        /*
        TODO: Mettre en oeuvre la transformation d'un vecteur normal.
        Nous avons vu en cours que cela demande le calcul d'une matrice
        particulière. Vous avez accès à l'inverse avec m_inv. Pour calculer
        la transposé vous avez la fonction "transpose(Mat4)".

        Comme pour les vecteurs, les normales ne subissent pas de translation.
        Dans le cours, nous avons vu comment exprimer des vecteurs dans les coordonnées
        homogènes. Vous pouvez utiliser la méthode ".xyz()" pour extraire les 3 composantes 
        du vecteur. 
        */
        Mat4 m_normal = transpose(m_inv);
        Vec4 n4 = Vec4{n.x, n.y, n.z, 0.0};
        Vec4 result = mul(m_normal, n4);
        return Vec3(result.xyz());
    }

    /// Apply the homogenous transformation to a 3D point
    Vec3 point(const Vec3& p) const {
        /*
        TODO: Vous devez utiliser les coordonnées homogènes pour 
        appliquer la transformation à un point.

        Attention contrairement aux vecteurs, nous devons diviser les 3 premières
        composantes par la quatrième (.w()).   
        */
        Vec4 p4 = Vec4{p.x, p.y, p.z, 1.0};
        Vec4 result = mul(m, p4);
        return Vec3(result.xyz());
    }

    /// Apply the homogenous transformation to a ray
    Ray ray(const Ray& r) const {
        /*
        TODO: Utiliser les methodes définie plus haut
        pour transformer l'origine (r.o) et la direction (r.d) 
        d'un rayon.
        */
       Vec3 o = point(r.o);
       Vec3 d = vector(r.d);
       return Ray(o, d, r.tmin, r.tmax);
    }
};