// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

#include "MyApp.h"

#include "Headers/Model/RayMarchedModel.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

// Builds the startup demo scene: a 3x7 grid of ray-marched shapes (plane,
// sphere, cube) x (7 PBR color/height texture pairs), a standalone bowl
// demonstrating the apex method's concave-surface fallback, and a wood/
// geometries ground plane beneath everything.
void MyApp::CreateDemoScene()
{
    static const char* kShapeFolders[3] = { "Plane", "Sphere", "Cube" };
    static const char* kTextureNames[7] = {
        "Leather", "Pavement", "Brick", "Wood", "Stone", "Earth", "Asteroid"
    };
    static const char* kHeightmaps[7] = {
        "Assets/HMaps/leather.jpg",
        "Assets/HMaps/pavement.jpg",
        "Assets/HMaps/brick.jpg",
        "Assets/HMaps/wood.jpg",
        "Assets/HMaps/stone.jpg",
        "Assets/HMaps/Earth-heightmap-small.png",
        "Assets/HMaps/asteroid.jpg",
    };

    constexpr float kColSpacing = 6.0f;
    constexpr float kRowSpacing = 6.0f;
    // Stable range the models were tuned against is roughly 0.04-0.5;
    // 0.08 keeps the PBR heightmaps (fairly high-frequency detail)
    // comfortably inside it. Adjust live per-model via the Models panel
    // if a particular texture still looks too flat/too extreme.
    constexpr float kGridNormalMult = 0.08f;

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 7; ++col) {
            std::string objPath = std::string("Assets/models/") + kShapeFolders[row] + "_"
                + kTextureNames[col] + "/" + kShapeFolders[row] + ".obj";

            auto model = CreateRayMarchedModelFromOBJ(objPath);
            if (!model) continue;

            auto heightmap = m_textureManager.GetOrLoad(kHeightmaps[col], false);
            model->SetHeightmap(heightmap, m_conemapGenerator.get(), kHeightmaps[col]);
            model->SetNormalMult(kGridNormalMult);

            float x = (col - 3) * kColSpacing;
            float z = -row * kRowSpacing;
            model->GetTransform().SetLocation(glm::vec3(x, 0.f, z));

            // The Earth equirectangular texture comes out upside-down on the
            // sphere and cube's own UV convention (not on the plane, which
            // uses a simple planar mapping) -- flip those two 180 degrees
            // around Z.
            bool isEarth = (kTextureNames[col] == std::string("Earth"));
            if (isEarth && (row == 1 || row == 2)) {
                model->GetTransform().SetRotation(
                    glm::angleAxis(glm::radians(180.0f), glm::vec3(0.f, 0.f, 1.f)));
            }

            model->SetName(std::string(kShapeFolders[row]) + " - " + kTextureNames[col]);

            m_sceneManager.Add(std::move(model));
        }
    }

    // --- Bowl: concave surface, in front of the grid. Demonstrates the
    // apex method's accepted degenerate case: normals converge above a
    // concave dip instead of diverging, the k/wB/wC guard rejects the
    // ill-conditioned fit, and the construction falls back to the
    // original single-corner affine mapping (visibly less accurate --
    // this is the intended, documented demo of that fallback). ---
    {
        auto bowl = CreateRayMarchedModelFromOBJ("Assets/models/bowl.obj");
        if (bowl) {
            auto heightmap = m_textureManager.GetOrLoad("Assets/HMaps/hemisphere.png", false);
            bowl->SetHeightmap(heightmap, m_conemapGenerator.get(), "Assets/HMaps/hemisphere.png");
            // Deliberately high -- makes the degenerate/fallback case clearly visible.
            bowl->SetNormalMult(0.76f);
            bowl->GetTransform().SetLocation(glm::vec3(0.f, 0.f, kRowSpacing));
            bowl->SetName("Bowl (concave fallback demo)");
            m_sceneManager.Add(std::move(bowl));
        }
    }

    // --- Ground: large surface beneath the whole scene, wood color texture
    // with the geometric heightmap pattern (instead of the desert heightmap) ---
    {
        auto ground = CreateRayMarchedModelFromOBJ("Assets/models/Plane_Wood/Plane.obj");
        if (ground) {
            auto heightmap = m_textureManager.GetOrLoad("Assets/HMaps/heightmap-geometries.png", false);
            ground->SetHeightmap(heightmap, m_conemapGenerator.get(), "Assets/HMaps/heightmap-geometries.png");
            ground->SetNormalMult(5.f);
            ground->GetTransform().SetLocation(glm::vec3(0.f, -8.f, -3.f));
            ground->GetTransform().SetScale(glm::vec3(9.f, 1.f, 9.f));

            ground->SetName("Ground (wood / geometries)");
            m_sceneManager.Add(std::move(ground));
        }
    }
}
