#pragma once

#include "render/shapes/shape.h"
#include "render/shapes/shape_group.h"
#include "render/samplers/independent.h"

// Structure to cache the AABB for a given shape
// this structure is only used for BVH construction
struct CachedShapeAABB {
	AABB aabb; //< Cached AABB
	std::shared_ptr<Shape> shape; //< Ptr to the shape
	CachedShapeAABB(std::shared_ptr<Shape> s) {
		aabb = s->aabb();
		shape = s;
	}
	CachedShapeAABB() :
		aabb(AABB()),
		shape(nullptr)
	{}
};

class BVHNode {
public:
	/// The bounding box associated to the node
	AABB aabb; 

	// Structure describing the nodes type
	struct Leaf {
        // Index of the frist primitive
        size_t first_primitive;
        // Number of primitives handled this leaf
        size_t primitive_count;
    };
    struct Node {
		// Index of the left child node
		// right child is at: left+1
        size_t left;
    };

private:
	// Union here to be able to hold
	// either the information for a leaf
	// or a node
    union {
        Leaf leaf_data;
        Node node_data;
    };

	// Store the node type for checks
    enum Type {
        LeafType,
        NodeType,
    } type;

public:
	// Construct a leaf node
	// Compute the associated AABB from the shapes
    BVHNode(Leaf leaf, std::vector<CachedShapeAABB>& aabbs) : leaf_data(leaf), type(LeafType) {
		aabb = AABB();
		for(auto i = 0; i < std::size_t(prim_count()); i++) {
			aabb = merge_aabb(aabb, aabbs[i+first_prim()].aabb);
		}
	}

	// Change the BVHNode to node type
	void to_node(size_t left) {
		type = NodeType;
		node_data.left = left;
	}

	bool is_leaf() const {
		return type == LeafType;
	}

	size_t left() const {
		assert(type == NodeType);
		return node_data.left;
	}

	size_t prim_count() const {
		return leaf_data.primitive_count;
	}

	size_t first_prim() const {
		return leaf_data.first_primitive;
	}
};

class BVH: public ShapeGroup {

	std::vector<BVHNode> m_nodes;
public:	
	enum class AxisSelection {
		ERoundRobin, // Alternate between x, y, z
		ELongest	 // Take the longest axis
	};
	AxisSelection axis_selection;

	enum class Builder {
		EMedian,  // Split using the median using the selected axis
		ESpatial, // Split using the middle using the selected axis
		ESAH	  // Perform sweep SAH
	};
	Builder builder;

	BVH(const json& param = json::object()) {
		// Read axis split strategy
		// this strategy is ignored is we use SAH
		std::string axis_selection_str = param.value("axis_selection", "roundrobin");
		if(axis_selection_str == "roundrobin") {
			axis_selection = AxisSelection::ERoundRobin;
		} else if(axis_selection_str == "longest") {
			axis_selection = AxisSelection::ELongest;
		} else {
			spdlog::warn("Unknown axis_selection: {}, use Round robin instead.", axis_selection_str);
			axis_selection = AxisSelection::ERoundRobin;
		}
	
		// Builder BVH strategy
		std::string builder_str = param.value("builder", "sah");
		builder = Builder::ESAH;
		if(builder_str == "sah") {
			builder = Builder::ESAH;
		} else if(builder_str == "median") {
			builder = Builder::EMedian;
		} else if(builder_str == "spatial") {
			builder = Builder::ESpatial;
		} 
	}

	bool hit(const Ray& r, Intersection& its) const override {
		if (!m_nodes.empty()) {
			// Test de l'intersection du noeud racine (aabb)
			if(m_nodes[0].aabb.hit(r)) {
				// On copie le rayon pour qu'il soit plus facile a editer 
				// lors de la récursion
				Ray ray = r;
				return hit_bvh(ray, its, m_nodes[0]);
			} else {
				return false;
			}
		} else {
			return false;
		}
	}

	// Fonction récursive pour calculer l'intersection ou non d'un noeud.
	// Le noeud est représenter par l'indice (node_idx)
	bool hit_bvh(Ray& r, Intersection& its, const BVHNode& node) const {
		if (node.is_leaf())
		{
			bool found = false;
			for (auto i = 0; i < std::size_t(node.prim_count()); i++ ) {
				if(m_shapesAABB[i+node.first_prim()].hit(r)) {
					if(m_shapes[i+node.first_prim()]->hit(r, its)) {
						r.tmax = its.t;
						found = true;
					}
				}
			}
			return found;
		}
		else
		{
			auto id1 = node.left();
			auto id2 = node.left() + 1;
			const BVHNode* n1 = &m_nodes[node.left()];
			const BVHNode* n2 = &m_nodes[node.left() + 1];
			double d1, d2;
			bool hit1 = n1->aabb.hit(r, d1);
			bool hit2 = n2->aabb.hit(r, d2);
			d1 = hit1 ? d1 : std::numeric_limits<double>::max();
			d2 = hit2 ? d2 : std::numeric_limits<double>::max();
			if(d1 > d2) {
				std::swap(d1, d2);
				std::swap(id1, id2);
			}
			bool have_hit = false;
			if(d1 < r.tmax) {
				have_hit |= hit_bvh( r, its, m_nodes[id1]);
			}
			if(d2 < r.tmax) {
				have_hit |= hit_bvh( r, its, m_nodes[id2]);
			}
			return have_hit;
		}
	}

	AABB aabb() const override {
		if (!m_nodes.empty()) {
			return m_nodes[0].aabb;
		}
		else {
			return AABB();
		}
	}

	void build() override {
		if(!m_nodes.empty()) {
			spdlog::warn("Try to build several time!");
			return;
		}

		std::vector<CachedShapeAABB> cached_aabbs(m_shapes.size());
		for(size_t id = 0; id < m_shapes.size(); id++) {
			cached_aabbs[id] = CachedShapeAABB(m_shapes[id]); 
		}

		// Create root
		BVHNode root = BVHNode({0, cached_aabbs.size()}, cached_aabbs);

		// Add root node inside the hierachy
		m_nodes.push_back(root);

		// Subdivide recursively the root node ()
		subdivide_node(0, cached_aabbs, 0);

		// Copy back the shape ptr
		// indeed, during the build, we might have sorted the shapes
		// thus we need to store the correct order inside the AABB
		std::vector<std::shared_ptr<Shape>> new_shapes_order(m_shapes.size());
		for(auto idx = 0; idx < m_shapes.size(); idx++) {
			new_shapes_order[idx] = cached_aabbs[idx].shape;
		}
		m_shapes = new_shapes_order;
		m_shapesAABB.clear();
		for(auto idx = 0; idx < m_shapes.size(); idx++) {
			m_shapesAABB.push_back(m_shapes[idx]->aabb());
		}
	}

	void subdivide_node(size_t node_idx, std::vector<CachedShapeAABB>& cached_aabbs, int depth) {
		BVHNode& node = m_nodes[node_idx];
		int axis = 0;
		if(axis_selection == BVH::AxisSelection::ERoundRobin) {
			axis = depth % 3;
		} else if(axis_selection == BVH::AxisSelection::ELongest) {
			Vec3 diag = node.aabb.diagonal();
			if(diag.x > diag.y && diag.x > diag.z) {
				axis = 0;
			} else if(diag.y > diag.z) {
				axis = 1;
			} else {
				axis = 2;
			}
		}

		if (node.prim_count() <= 2) {
			// Do not subdivide
			return;
		}

		// Transform to node
		size_t nb_prim = node.prim_count();
		size_t first_prim = node.first_prim();
		node.to_node(m_nodes.size());

		uint32_t offset = 0;
		if(builder == BVH::Builder::EMedian) {
			offset = std::max(uint32_t(nb_prim * 0.5), uint32_t(1));
			std::nth_element(cached_aabbs.begin() + first_prim, 
							 cached_aabbs.begin() + first_prim + offset,
							 cached_aabbs.begin() + first_prim + nb_prim, [&](const auto& a, const auto& b) {
				return a.aabb.center()[axis] < b.aabb.center()[axis];
			});
		} else if(builder == BVH::Builder::ESpatial) {
			// std::sort(cached_aabbs.begin() + first_prim, cached_aabbs.begin() + first_prim + nb_prim, [&](const auto& a, const auto& b) {
			// 	return a.aabb.center()[axis] < b.aabb.center()[axis];
			// });
			// double pos = node.aabb.center()[axis];
			// offset = 0;
			// while(offset < nb_prim && cached_aabbs[first_prim + offset].aabb.center()[axis] < pos) {
			// 	offset += 1;
			// }
			auto mid = std::partition(cached_aabbs.begin() + first_prim, cached_aabbs.begin() + first_prim + nb_prim, [&](const auto& a) {
				return a.aabb.center()[axis] < node.aabb.center()[axis];
			});
			offset = std::distance(cached_aabbs.begin() + first_prim, mid);
			if(offset == 0) {
				auto min_prim = std::min_element(cached_aabbs.begin() + first_prim, cached_aabbs.begin() + first_prim + nb_prim, 
					[&](const auto& a, const auto& b) {
					return a.aabb.center()[axis] < b.aabb.center()[axis];
				});
				std::swap(*(cached_aabbs.begin() + first_prim), *min_prim);
				offset = std::max(offset, uint32_t(1));
			}
			
		} else if(builder == BVH::Builder::ESAH) {
			// Sweeping
			int bestPos = 0; 
			double bestCost = 1e30f;
			int bestAxis = 3;
			{
				std::vector<double> scores(nb_prim-1, 0.0);
				for (int o = 0; o < 3; o++) {
					std::sort(cached_aabbs.begin() + first_prim, cached_aabbs.begin() + first_prim + nb_prim, [&](const auto& a, const auto& b) {
						return a.aabb.center()[o] < b.aabb.center()[o];
					});
					AABB tmp;
					for (auto id = 0; id < nb_prim-1; id++) {
						auto id_left = nb_prim - id - 1;
						tmp = merge_aabb(tmp, cached_aabbs[id_left + first_prim].aabb);
						scores[id_left-1] = tmp.area() * (id + 1);
					}
					// Reset aabb
					tmp = AABB();
					for (auto id = 0; id < nb_prim-1; id++) {
						tmp = merge_aabb(tmp, cached_aabbs[id + first_prim].aabb);
						scores[id] += tmp.area() * (id + 1);
						if (scores[id] < bestCost) {
							bestPos = id + 1;
							bestAxis = o;
							bestCost = scores[id];
						}
					}
				}
			}

			std::sort(cached_aabbs.begin() + first_prim, cached_aabbs.begin() + first_prim + nb_prim, [&](const auto& a, const auto& b) {
				return a.aabb.center()[bestAxis] < b.aabb.center()[bestAxis];
			});
			offset = bestPos;
			if(offset == nb_prim || offset == 0) {
				// Try mid
				spdlog::warn("SAH gives degenerate node: {} / {} [{}]", offset, nb_prim, bestCost);
				double pos = node.aabb.center()[bestAxis];
				offset = 0;
				while(offset < nb_prim && cached_aabbs[first_prim + offset].aabb.center()[bestAxis] < pos) {
					offset += 1;
				}
				if(offset == 0 || offset == nb_prim) {
					// Revert to mid
					offset = std::max(uint32_t(nb_prim * 0.5), uint32_t(1));
				}
			}
		}

		BVHNode left = BVHNode({first_prim, offset}, cached_aabbs);
		BVHNode right = BVHNode({first_prim + offset, nb_prim - offset}, cached_aabbs);

		size_t id_left = m_nodes.size();
		m_nodes.push_back(left); 
		m_nodes.push_back(right);

		subdivide_node(id_left, cached_aabbs, depth + 1);
		subdivide_node(id_left + 1, cached_aabbs, depth + 1);
	}
};