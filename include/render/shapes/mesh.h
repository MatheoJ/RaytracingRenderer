#pragma once

#include "render/texture.h"
#include "render/shapes/shape.h"

class Mesh : public ShapeWithMaterialAndTransform {
private:
	AABB m_aabb; // Precomputed AABB
public:
    Mesh(std::shared_ptr<Material> mat): ShapeWithMaterialAndTransform(mat, json::object()) {}
	Mesh(std::shared_ptr<Material> mat, const json& param);
    virtual ~Mesh() {}

    bool hit(const Ray& r, Intersection& its) const override {
        throw Exception("Hit not implemented on Mesh. Need to add the generated triangles inside parents");
    }
    virtual AABB aabb() const override {
        return m_aabb;
    }
    std::vector<std::shared_ptr<Shape>> children(std::shared_ptr<Shape> self) const override;

    bool has_uv() const {
        return !uvs.empty();
    }
    bool has_normal() const {
        return !normals.empty();
    }
    void computeTangent();

    bool inverse_normal = false;
    bool two_sided = false;
    std::vector<Vec3> positions; //< Vertex positions
    std::vector<Vec3> normals;   //< Vertex normals
    std::vector<Vec3> tangents;   //< Vertex normals
    std::vector<Vec2> uvs;       //< Vertex texture coordinates
    std::vector<Vec3i> face_positions_idx;  //< Indices for positions (for each faces)
    std::vector<Vec3i> face_normals_idx;    //< Indices for normals (for each faces)
    std::vector<Vec3i> face_uvs_idx;        //< Indices for texture coordinates (for each faces)
    Texture<Color4> m_normalMap;
};