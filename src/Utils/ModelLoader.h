#pragma once

#include <memory>
#include <vector>
#include <filesystem>

#include "Log.h"
#include "GLUtils.hpp"

#include <tiny_obj_loader.h>

template <typename VertexT = Vertex>
struct ModelLoaderMatMesh {
    MeshObject<VertexT> mesh;
    int materialID;
};

template <typename VertexT = Vertex>
struct ModelLoaderData {
    std::vector<tinyobj::material_t> materials;
    std::vector<ModelLoaderMatMesh<VertexT>> matMesh;
};

class ModelLoader {
private:

    struct ModelLoaderRawData {
        std::vector<tinyobj::material_t> materials;
        std::unordered_map<int, std::vector<Vertex>> verticesPerMaterial;
        std::unordered_map<int, std::vector<GLuint>> indicesPerMaterial;
    };

    template <typename VertexT = Vertex>
    static inline void InternalLoader(const std::filesystem::path& objPath, ModelLoaderRawData& modelData, const std::filesystem::path& mtlSearchPath = "./") {

        // --- Clear model data ---
        modelData.materials.clear();
        modelData.indicesPerMaterial.clear();
        modelData.verticesPerMaterial.clear();

        // --- Generate and check .obj file path ---
        std::filesystem::path objFile = RelToAbsPath(objPath);
        if (!std::filesystem::exists(objFile)) {
            return;
        }

        // --- Get .mtl search path ---
        std::filesystem::path absMtlSearchPath = ResolveMTLSearchPath(objFile, mtlSearchPath);

        // Configure tinyObjLoader
        tinyobj::ObjReaderConfig config;
        config.triangulate = true;
        config.mtl_search_path = absMtlSearchPath.string();

        // --- Read model from file ---
        LOG("Reading .obj file: ", objFile.generic_string());
        tinyobj::ObjReader reader;
        if (!reader.ParseFromFile(objFile.generic_string(), config)) {
            LOG_ERROR("TinyObjLoader error: ", reader.Error());
            throw std::runtime_error("Failed to load OBJ file.");
        }
        const auto& attrib = reader.GetAttrib();
        const auto& shapes = reader.GetShapes();
        const auto& materials = reader.GetMaterials();

        // --- Load materials ---
        LOG("Materials found: ", materials.size());

        modelData.materials = materials;

        // --- Create Meshes ---
        for (const auto& shape : shapes) {

            size_t index_offset = 0;
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                int fv = shape.mesh.num_face_vertices[f];
                int mat_id = shape.mesh.material_ids.empty() ? -1 : shape.mesh.material_ids[f];

                if (mat_id < 0 || mat_id >= (int)modelData.materials.size()) mat_id = 0;

                for (size_t v = 0; v < fv; v++) {
                    Vertex vert{};
                    tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                    // position
                    vert.position = glm::vec3(
                        attrib.vertices[3 * idx.vertex_index + 0],
                        attrib.vertices[3 * idx.vertex_index + 1],
                        attrib.vertices[3 * idx.vertex_index + 2]
                    );

                    // normal
                    if (idx.normal_index >= 0) {
                        vert.normal = glm::vec3(
                            attrib.normals[3 * idx.normal_index + 0],
                            attrib.normals[3 * idx.normal_index + 1],
                            attrib.normals[3 * idx.normal_index + 2]
                        );
                    }

                    // texture coordinates
                    if (idx.texcoord_index >= 0) {
                        vert.texcoord = glm::vec2(
                            attrib.texcoords[2 * idx.texcoord_index + 0],
                            attrib.texcoords[2 * idx.texcoord_index + 1]
                        );
                    }

                    auto& vList = modelData.verticesPerMaterial[mat_id];
                    vList.push_back(vert);
                    modelData.indicesPerMaterial[mat_id].push_back(static_cast<GLuint>(vList.size() - 1));
                }
                index_offset += fv;
            }
        }
    }
public:

    static inline std::filesystem::path RelToAbsPath(const std::filesystem::path& path) {
        return path.is_absolute() ?
            path :
            std::filesystem::path(PROJECT_ROOT) / path;
    }
    static inline std::filesystem::path ResolveMTLSearchPath(const std::filesystem::path& objPath, const std::filesystem::path& mtlSearchPath)
    {
        // If mtlSearchPath is absolute, return
        if (mtlSearchPath.is_absolute())
            return mtlSearchPath;

        // Compute based on objPath
        std::filesystem::path objDir = RelToAbsPath(objPath.parent_path());
        std::filesystem::path resolved = std::filesystem::weakly_canonical(objDir / mtlSearchPath);

        return resolved;
    }
    static inline std::filesystem::path ResolveTexturePath(const std::string& texPath, const std::filesystem::path& texFolder) {
        // normal texture path
        std::string pathName = texPath;
        std::replace(pathName.begin(), pathName.end(), '/', '\\');
        std::filesystem::path path = pathName;
        if (!path.is_absolute()) {
            path = RelToAbsPath(texFolder) / pathName;
        }
        return path;
    }

    template <typename VertexT = Vertex>
    static ModelLoaderData<VertexT> LoadFromOBJ(
        const std::filesystem::path& objPath,
        const std::filesystem::path& mtlSearchPath = "./",
        std::function<std::vector<VertexT>(const std::vector<Vertex>&)> transformFunc = nullptr
    ) {
        // 1. Step: Load raw data from disk into intermediate storage
        ModelLoaderRawData modelData;
        InternalLoader(RelToAbsPath(objPath), modelData, mtlSearchPath);

        // 2. Step: Process data
        ModelLoaderData<VertexT> retVal;

        // Iterate through vertex groups organized by material ID
        for (auto& [mat_id, verts] : modelData.verticesPerMaterial) {
            auto& inds = modelData.indicesPerMaterial[mat_id];

            // Apply vertex transformation if necessary
            if constexpr (std::is_same_v<VertexT, Vertex>) {
                if (transformFunc) {
                    // Identity type with custom logic (e.g., normal merging on standard Vertex)
                    retVal.matMesh.push_back({
                        {
                            transformFunc(verts),
                            std::move(inds)
                        },
                        mat_id
                    });
                }
                else {
                    // Default case: direct move of standard vertices
                    retVal.matMesh.push_back({
                        {
                            std::move(verts),
                            std::move(inds)
                        },
                        mat_id
                    });
                }
            }
            else {
                // Custom Vertex Type: transformation function is mandatory
                if (transformFunc) {
                    retVal.matMesh.push_back({
                        {
                            transformFunc(verts),
                            std::move(inds)
                        },
                        mat_id
                    });
                }
                else {
                    LOG_ERROR("[ModelLoader] Custom Vertex Type requested but no transformFunc provided.");
                    throw std::runtime_error("ModelLoader: Custom Vertex Type requested but no transformFunc provided.");
                }
            }
        }

        // 3. Step: Finalize and transfer ownership of shared resources
        retVal.materials = std::move(modelData.materials);

        return retVal;
    }

    // Merges normals for vertices at the same position to enable seamless displacement
    static std::vector<VertexMergedNorm> MergeNormals(const std::vector<Vertex>& inputVertices) {
        // Custom comparator to identify vertices at the exact same spatial location
        struct VectorComparator {
            bool operator()(const glm::vec3& lhs, const glm::vec3& rhs) const {
                if (lhs.x != rhs.x) return lhs.x < rhs.x;
                if (lhs.y != rhs.y) return lhs.y < rhs.y;
                return lhs.z < rhs.z;
            }
        };

        // 1. PHASE: Pass through all vertices to sum up normals for each position
        // We don't care about UVs or indices here, only where the vertices are in 3D space.
        std::map<glm::vec3, glm::vec3, VectorComparator> positionToSummedNormal;

        for (const auto& v : inputVertices) {
            positionToSummedNormal[v.position] += v.normal;
        }

        // 2. PHASE: Create the new vertex array with the merged normal attribute
        // The size of the output vector is EXACTLY the same as the input.
        std::vector<VertexMergedNorm> outputVertices;
        outputVertices.reserve(inputVertices.size());

        for (const auto& v : inputVertices) {
            // Look up the pre-calculated sum for THIS vertex's position
            glm::vec3 merged = positionToSummedNormal[v.position];

            // Normalize the sum to get a proper direction vector
            if (glm::length(merged) > 0.0001f) {
                merged = glm::normalize(merged);
            }
            else {
                merged = v.normal; // Fallback if something went wrong
            }

            // Keep original data intact, only add the new merged normal
            outputVertices.push_back({
                v.position,      // Original position
                v.normal,        // Original "face" or "per-vertex" normal
                merged,          // The new "smooth" normal shared across boundaries
                v.texcoord       // Original UV - UNTOUCHED
                });
        }

        return outputVertices;
    }
};