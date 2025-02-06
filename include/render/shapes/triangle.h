#pragma once

#include "render/shapes/mesh.h"

class Triangle : public Shape {
public:

    Triangle(std::shared_ptr<Material> mat,  const json& param) {
        if (!param.contains("positions") || !param.at("positions").is_array() || param.at("positions").size() != 3)
            throw Exception("required \"positions\" field should be an array of three Vec3s");

        auto mesh       = std::make_shared<Mesh>(mat);
        mesh->positions = {param["positions"][0], param["positions"][1], param["positions"][2]};
        mesh->face_positions_idx = {{0, 1, 2}};
        if (param.contains("normals") && param.at("normals").is_array())
        {
            if (param.at("normals").size() == 3)
            {
                mesh->normals = {param["normals"][0], param["normals"][1], param["normals"][2]};
                mesh->face_normals_idx = mesh->face_positions_idx;
            }
            else
                spdlog::warn("optional \"normals\" field should be an array of three Vec3s, skipping");
        }
        if (param.contains("uvs") && param.at("uvs").is_array())
        {
            if (param.at("uvs").size() == 3)
            {
                mesh->uvs = {param["uvs"][0], param["uvs"][1], param["uvs"][2]};
                mesh->face_uvs_idx  = mesh->face_positions_idx;
            }
            else
                spdlog::warn("optional \"uvs\" field should be an array of three Vec2s, skipping");
        }

        m_mesh = mesh;
        m_face_id = 0;
    }

    Triangle(std::shared_ptr<const Mesh> mesh, size_t face_id) :
        m_mesh(mesh),
        m_face_id(face_id)
    {
    }

    virtual bool hit(const Ray& r, Intersection& its) const override {
        NUMBER_INTERSECTIONS += 1;

        // Example pour recuprer les positions des sommets
        auto idx = m_mesh->face_positions_idx[m_face_id];
        Vec3 p1 = m_mesh->positions[idx.x];
        Vec3 p2 = m_mesh->positions[idx.y];
        Vec3 p3 = m_mesh->positions[idx.z];

        /* Les autres informations auxiliaires:
        normale: m_mesh->normal
        coordonnée de texture: m_mesh->uvs
        Vous avez aussi les méthodes m_mesh->has_uv() et m_mesh->has_normal()
        pour vérifier si ces informations auxiliaires existent ou non. 

        Enfin, attention: les indices pour ces informations auxiliaires peuvent être différents.
        Par exemple: m_mesh->face_normals_idx[m_face_id], renvoie les indices à utiliser pour les normales. 
        Si c'est une information auxiliaire n'est pas disponible, utilisez la valeur par défaut:
        - normals: utiliser la normale du plan contenant le triangle (attention à l'ordre des vecteurs lors du calcul avec le produit vectoriel).
        - uv (uniquement pour la fin du devoir 1): utiliser les coordonnées de textures vues en cours (0,0),(1,0),(1,1)

        m_mesh->material() pour récupérer le matériau lié au mesh.
        */
        
        // Code à portée éducative pas optimisé -> 3450ms sur 02_classic à 64 spp

        // const Vec3 b = r.o - p3;
        // const Mat3 A = Mat3(p1 - p3, p2 - p3, -r.d);
        // const Vec3 x = mul(inverse(A), b);
        // const Vec3 bary = Vec3(x.x, x.y, 1 - x.x - x.y);
        // if (sum(bary) > 1.0){
        //     return false;
        // }
        // if (maxelem(bary) > 1.0 || minelem(bary) < 0.0){
        //     return false;
        // }
        // const float t = x.z;
        
        // Code optimisé, source wikipedia -> 2950ms sur 02_classic à 64 spp
        constexpr float ε = 0.000001;
        const Vec3 e1 = p2 - p1;
        const Vec3 e2 = p3 - p1;
        const Vec3 r_cross_e2 = cross(r.d, e2);
        // Triple product
        const float det = dot(e1, r_cross_e2);
        if (det > -ε && det < ε)
            return false;
        
        const float inv_det = 1.0 / det;
        const Vec3 s = r.o - p1;
        const float β = inv_det * dot(s, r_cross_e2);
        if (β < 0 || β > 1)
            return false;
        
        const Vec3 s_cross_e1 = cross(s, e1);
        const float γ = inv_det * dot(r.d, s_cross_e1);
        if (γ < 0 || γ + β > 1)
            return false;

        const float t = inv_det * dot(e2, s_cross_e1);
        if (t > r.tmin && t < r.tmax){
            const float alpha = 1 - γ - β;
            Vec3 n;
            Vec3 tan1, tan2, tan3;
            Vec2 uv1, uv2, uv3;
            if (m_mesh->has_uv()){
                auto idx_uv = m_mesh->face_uvs_idx[m_face_id];
                uv1 = m_mesh->uvs[idx_uv.x];
                uv2 = m_mesh->uvs[idx_uv.y];
                uv3 = m_mesh->uvs[idx_uv.z];
            }
            else{
                uv1 = Vec2(0.0, 0.0);
                uv2 = Vec2(1.0, 0.0);
                uv3 = Vec2(1.0, 1.0);
            }
            its.uv = alpha * uv1 + β * uv2 + γ * uv3;
            its.p = r.o + t * r.d;
            if (!m_mesh->has_normal()){
                n = normalize(cross(e1, e2));
            }
            else{
                auto idx_n = m_mesh->face_normals_idx[m_face_id];
                const Vec3 n1 = m_mesh->normals[idx_n.x];
                const Vec3 n2 = m_mesh->normals[idx_n.y];
                const Vec3 n3 = m_mesh->normals[idx_n.z];
                // n = normalize(bary.x * n1 + bary.y * n2 + bary.z * n3);
                n = normalize(alpha * n1 + β * n2 + γ * n3);
            }
            if (!(m_mesh->has_normal() && m_mesh->has_uv())){
                its.tang = normalize(cross(e1, n));
            }
            else{
                auto idx_n = m_mesh->face_normals_idx[m_face_id];
                tan1 = m_mesh->tangents[idx_n.x];
                tan2 = m_mesh->tangents[idx_n.y];
                tan3 = m_mesh->tangents[idx_n.z];
                its.tang = normalize(alpha * tan1 + β * tan2 + γ * tan3);
            }
            if (is_image(m_mesh->m_normalMap)){
                const Real scale = get_scale(m_mesh->m_normalMap);
                const Vec3 bitang = cross(n, its.tang);
                Frame tbn = Frame(n);
                tbn.x = its.tang;
                tbn.y = bitang;
                tbn.z = n;
                n = tbn.to_world(eval(m_mesh->m_normalMap, its.uv, {0.0, 0.0, 0.0}).xyz());
                // TODO : hard codé berk
                n.z *= (1 + scale);
                n = normalize(n);
            }
            
            if (m_mesh->two_sided){
                if (dot(n, -r.d) < 0.0){
                    n = -n;
                }
            }
            its.t = t;
            // its.p = bary.x * p1 + bary.y * p2 + bary.z * p3;
            its.n = n;
            its.material = m_mesh->material();
            its.shape = this;
            return true;
        }
        return false;
    }

    virtual AABB aabb() const override {
                AABB aabb;
        auto idx = m_mesh->face_positions_idx[m_face_id];
        aabb.extends(m_mesh->positions[idx.x]);
        aabb.extends(m_mesh->positions[idx.y]);
        aabb.extends(m_mesh->positions[idx.z]);

        // Make sure that the AABB is enough
        // thick for all the dimensions
        auto diag = aabb.diagonal();
        for (int i = 0; i < 3; i++) {
            if (diag[i] < 2e-4) {
                aabb.min[i] -= 1e-4;
                aabb.max[i] += 1e-4;
            }
        }

        return aabb;
    }

    virtual const Material* material() const override {
        return m_mesh->material();
    }

    /// Sample a position on the shape to compute direct lighting from the shading point x
    virtual std::tuple< EmitterSample, const Shape& > sample_direct(const Vec3& x, const Vec2& sample) const override {
        EmitterSample em = EmitterSample();
        auto idx = m_mesh->face_positions_idx[m_face_id];
        Vec3 p1 = m_mesh->positions[idx.x];
        Vec3 p2 = m_mesh->positions[idx.y];
        Vec3 p3 = m_mesh->positions[idx.z];

        // Sample Position
        constexpr float ε = 0.000001;
        const Vec3 e1 = p2 - p1;
        const Vec3 e2 = p3 - p1;
        if (sample.x + sample.y > 1){
            em.y = p1 + e1 * (1 - sample.x) + e2 * (1 - sample.y);
        }
        else{
            em.y = p1 + e1 * sample.x + e2 * sample.y;
        }

        // Compute barycentric coords using Cramer's rule https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
        const Vec3 e3 = em.y - p1;
        const double d00 = dot(e1, e1);
        const double d01 = dot(e1, e2);
        const double d11 = dot(e2, e2);
        const double d20 = dot(e3, e1);
        const double d21 = dot(e3, e2);
        const double denom = d00 * d11 - d01 * d01;
        const double alpha = (d11 * d20 - d01 * d21) / denom;
        const double β = (d00 * d21 - d01 * d20) / denom;
        const double γ = 1.0 - alpha - β;

        // Normal
        if (!m_mesh->has_normal()){
            em.n = normalize(cross(e1, e2));
        }
        else{
            auto idx_n = m_mesh->face_normals_idx[m_face_id];
            const Vec3 n1 = m_mesh->normals[idx_n.x];
            const Vec3 n2 = m_mesh->normals[idx_n.y];
            const Vec3 n3 = m_mesh->normals[idx_n.z];
            // n = normalize(bary.x * n1 + bary.y * n2 + bary.z * n3);
            em.n = normalize(alpha * n1 + β * n2 + γ * n3);
        }

        // UV
        Vec2 uv1, uv2, uv3;
        if (m_mesh->has_uv()){
            auto idx_uv = m_mesh->face_uvs_idx[m_face_id];
            uv1 = m_mesh->uvs[idx_uv.x];
            uv2 = m_mesh->uvs[idx_uv.y];
            uv3 = m_mesh->uvs[idx_uv.z];
        }
        else{
            uv1 = Vec2(0.0, 0.0);
            uv2 = Vec2(1.0, 0.0);
            uv3 = Vec2(1.0, 1.0);
        }
        em.uv = alpha * uv1 + β * uv2 + γ * uv3;
        em.pdf = pdf_direct(*this, x, em.y, em.n);
        return { em, *this };
    }
    // La valeur de la densite de probabilite d'avoir echantillonne
    // le point "y" avec la normale "n" sur la source de lumiere
    // 
    virtual Real pdf_direct(const Shape& shape, const Vec3& x, const Vec3& y, const Vec3& n) const override {
        const Vec3 diff = x - y;
        const double d2 = dot(diff, diff);

        auto idx = m_mesh->face_positions_idx[m_face_id];
        Vec3 p1 = m_mesh->positions[idx.x];
        Vec3 p2 = m_mesh->positions[idx.y];
        Vec3 p3 = m_mesh->positions[idx.z];
        const Vec3 e1 = p2 - p1;
        const Vec3 e2 = p3 - p1;
        const double area = length(cross(e1, e2)) / 2;

        const double cos_theta = std::abs(dot(n, normalize(x - y)));
        return d2 / (area * cos_theta + 1e-9);
    }


private:
    std::shared_ptr<const Mesh> m_mesh;
    size_t m_face_id;
};