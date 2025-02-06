#include "render/shapes/mesh.h"
#include "render/shapes/triangle.h"
#include "render/filesystem.h"

#include <map>

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cpp file
#include <tiny_obj_loader.h>

Mesh::Mesh(std::shared_ptr<Material> mat, const json& param):
    ShapeWithMaterialAndTransform(mat, param) 
{
	// Code based on callback example
	// from TinyObjLoader and Darts render code
    std::string filename = FileResolver::resolve(param["filename"]).string();
	std::ifstream is(filename);
    if (is.fail())
        throw Exception("Unable to open OBJ file '{}'!", filename);

	struct UserData {
		Mesh* mesh;
	} data;
	data.mesh = this;
	
	tinyobj::callback_t cb;
    cb.vertex_cb = [](void *user_data, float x, float y, float z, float w)
    {
        UserData *data = reinterpret_cast<UserData *>(user_data);
        Mesh *    mesh = data->mesh;
        mesh->positions.push_back(mesh->m_transform.point(Vec3(x, y, z)));
        mesh->m_aabb.extends(mesh->positions.back());
    };

    cb.normal_cb = [](void *user_data, float x, float y, float z)
    {
        UserData *data = reinterpret_cast<UserData *>(user_data);
        Mesh *    mesh = data->mesh;
        mesh->normals.push_back(normalize(mesh->m_transform.normal(Vec3(x, y, z))));
    };

    cb.texcoord_cb = [](void *user_data, float x, float y, float z)
    {
        UserData *data = reinterpret_cast<UserData *>(user_data);
        Mesh *    mesh = data->mesh;
        mesh->uvs.push_back(Vec2(x, y));
    };

    cb.index_cb = [](void *user_data, tinyobj::index_t *indices, int num_indices)
    {
        // NOTE: the value of each index is raw value. For example, the application must manually adjust the index with
        // offset (e.g. v_indices.size()) when the value is negative (which means relative index). Also, the first index
        // starts with 1, not 0. See fixIndex() function in tiny_obj_loader.h for details. Also, 0 is set for the index
        // value which does not exist in .obj
        UserData *data = reinterpret_cast<UserData *>(user_data);
        Mesh *    mesh = data->mesh;

        // just create a naive triangle fan from the first vertex
        tinyobj::index_t idx0 = indices[0], idx1 = indices[1], idx2;

        tinyobj::fixIndex(idx0.vertex_index, int(mesh->positions.size()), &idx0.vertex_index);
        tinyobj::fixIndex(idx0.normal_index, int(mesh->normals.size()), &idx0.normal_index);
        tinyobj::fixIndex(idx0.texcoord_index, int(mesh->uvs.size()), &idx0.texcoord_index);

        tinyobj::fixIndex(idx1.vertex_index, int(mesh->positions.size()), &idx1.vertex_index);
        tinyobj::fixIndex(idx1.normal_index, int(mesh->normals.size()), &idx1.normal_index);
        tinyobj::fixIndex(idx1.texcoord_index, int(mesh->uvs.size()), &idx1.texcoord_index);
        for (int i = 2; i < num_indices; i++)
        {
            idx2 = indices[i];
            tinyobj::fixIndex(idx2.vertex_index, int(mesh->positions.size()), &idx2.vertex_index);
            tinyobj::fixIndex(idx2.normal_index, int(mesh->normals.size()), &idx2.normal_index);
            tinyobj::fixIndex(idx2.texcoord_index, int(mesh->uvs.size()), &idx2.texcoord_index);

            mesh->face_positions_idx.push_back({idx0.vertex_index, idx1.vertex_index, idx2.vertex_index});
            mesh->face_normals_idx.push_back({idx0.normal_index, idx1.normal_index, idx2.normal_index});
            mesh->face_uvs_idx.push_back({idx0.texcoord_index, idx1.texcoord_index, idx2.texcoord_index});

            idx1 = idx2;
        }
    };

	cb.usemtl_cb = [](void *user_data, const char *name, int material_idx)
    {
        UserData *data = reinterpret_cast<UserData *>(user_data);
        Mesh *    mesh = data->mesh;
	};
	cb.mtllib_cb = [](void *user_data, const tinyobj::material_t *materials, int num_materials)
    {
        UserData *data = reinterpret_cast<UserData *>(user_data);
        Mesh *    mesh = data->mesh;
    };


	//error and warning output from the load function
	std::string warn;
	std::string err;
	bool ret = tinyobj::LoadObjWithCallback(is, cb, &data, nullptr, &warn, &err);

	//make sure to output the warnings to the console, in case there are issues with the file
	if (!warn.empty()) {
		spdlog::warn("Warning from tinyobj for {}: {}", filename, warn);
	}
	//if we have any error, print it to the console, and break the mesh loading.
	//This happens if the file can't be found or is malformed
	if (!err.empty()) {
		throw Exception("Error from tinyobj for {} : {}", filename, err);
	}
	if(!ret) {
		throw Exception("tinyobj load obj failed: {}", filename);
	}

    bool smooth = param.value("smooth", true);
    if (!smooth) {
        normals.clear();
        face_normals_idx.clear();
    }

    inverse_normal = param.value("inverse_normal", false);
    two_sided = param.value("two-sided", false);
    m_normalMap = create_texture_color(param, "normal", Color4(0.0, 0.0, 1.0, 0.0));

	spdlog::info("Loaded: {}", filename);
	spdlog::info(" - AABB (untransform): [{}, {}]", m_transform.inv().point(m_aabb.min), m_transform.inv().point(m_aabb.max));
    spdlog::info(" - AABB     : [{}, {}]", m_aabb.min, m_aabb.max);
	spdlog::info(" - centroid : {}", m_aabb.center());
	spdlog::info(" - #faces   : {}", face_positions_idx.size());
	spdlog::info(" - #vertices: {}", positions.size());	
    spdlog::info(" - has_normal: {}", has_normal());
    spdlog::info(" - has_uv    : {}", has_uv());

    if (has_uv() && has_normal()){
        spdlog::info("Pre-computing tangents");
        computeTangent();
    }

}

std::vector<std::shared_ptr<Shape>> Mesh::children(std::shared_ptr<Shape> self) const {
    auto mesh = std::dynamic_pointer_cast<Mesh>(self);

    std::vector<std::shared_ptr<Shape>> shapes = {};
    for (size_t face_id = 0; face_id < face_positions_idx.size(); face_id++) {
        shapes.emplace_back(
            std::make_shared<Triangle>(
                mesh,
                face_id
                )
        );
    }
    return shapes;
}

void Mesh::computeTangent() {
    // Calculate tangent using https://marti.works/posts/post-calculating-tangents-for-your-mesh/post/
    size_t vtxCount = normals.size();
    tangents.reserve(vtxCount);
    std::vector<Vec4> result;
    std::vector<Vec3> tanA(vtxCount, Vec3(0,0,0));
    std::vector<Vec3> tanB(vtxCount, Vec3(0,0,0));
    // (1)
    size_t indexCount = face_positions_idx.size();
    for (size_t i = 0; i < indexCount; ++i) {
        if (has_uv()){
            auto idx_pos = face_positions_idx[i];
            Vec3 pos0 = positions[idx_pos.x];
            Vec3 pos1 = positions[idx_pos.y];
            Vec3 pos2 = positions[idx_pos.z];
            auto idx_tex = face_uvs_idx[i];
            Vec2 tex0 = uvs[idx_tex.x];
            Vec2 tex1 = uvs[idx_tex.y];
            Vec2 tex2 = uvs[idx_tex.z];
            
            Vec3 edge1 = pos1 - pos0;
            Vec3 edge2 = pos2 - pos0;
            Vec2 uv1 = tex1 - tex0;
            Vec2 uv2 = tex2 - tex0;
            float r = 1.0f / (uv1.x * uv2.y - uv1.y * uv2.x);
            Vec3 tangent(
                ((edge1.x * uv2.y) - (edge2.x * uv1.y)) * r,
                ((edge1.y * uv2.y) - (edge2.y * uv1.y)) * r,
                ((edge1.z * uv2.y) - (edge2.z * uv1.y)) * r
            );
            Vec3 bitangent(
                ((edge1.x * uv2.x) - (edge2.x * uv1.x)) * r,
                ((edge1.y * uv2.x) - (edge2.y * uv1.x)) * r,
                ((edge1.z * uv2.x) - (edge2.z * uv1.x)) * r
            );
            auto idx_tang = face_normals_idx[i];
            tanA[idx_tang.x] += tangent;
            tanA[idx_tang.y] += tangent;
            tanA[idx_tang.z] += tangent;
            tanB[idx_tang.x] += bitangent;
            tanB[idx_tang.y] += bitangent;
            tanB[idx_tang.z] += bitangent;
        }
    }
    // (2)
    for (size_t i = 0; i < vtxCount; i++) {
        Vec3 n = normals[i];
        Vec3 t0 = tanA[i];
        Vec3 t1 = tanB[i];
        Vec3 t = t0 - (n * dot(n, t0));
        t = normalize(t);
        Vec3 c = cross(n, t0);
        // float w = (dot(c, t1) < 0) ? -1.0f : 1.0f;
        if (dot(c, t1) < 0){
            tangents[i] = m_transform.normal({-t.x, -t.y, -t.z});
        }
        else{
            tangents[i] = m_transform.normal({t.x, t.y, t.z});
        }
        // tangents[i] = Vec4(1.0, 0.0, 0.0, 0.0);
    }
}