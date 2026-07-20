#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

#include "ModelBase.h"
#include "Headers/Model/Mesh.h"
#include "Headers/Texture/Texture.h"
#include "Interfaces/IRayMarchingTechnique.h"

class ConemapGenerator;

class RayMarchedModel : public ModelBase {
public:
    explicit RayMarchedModel(std::string name = "unnamed ray marched model");

    GLuint GetProgramID() const override;

    void SetTechnique(std::shared_ptr<IRayMarchingTechnique> t);
    // Generates the conemap from heightmap via gen and stores the result.
    // The heightmap itself is not retained — only the conemap is kept.
    // If gen == nullptr, m_conemap is left unchanged.
    void SetHeightmap(std::shared_ptr<Texture> heightmap, ConemapGenerator* gen);

    void AddMesh(std::shared_ptr<Mesh> mesh) { m_meshes.push_back(std::move(mesh)); }
    const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return m_meshes; }

    // (+,!) — no public setter; use SetHeightmap() / SetTechnique()
    const std::shared_ptr<Texture>&               GetConemap()   const { return m_conemap; }
    const std::shared_ptr<IRayMarchingTechnique>& GetTechnique() const { return m_technique; }

    GLuint GetSelectedProgramID() const     { return m_selectedProgramID; }
    void   SetSelectedProgram(GLuint id)    { m_selectedProgramID = id; }

    // (+,+) render parameters
    int       GetMaxSteps()             const { return m_maxSteps; }
    void      SetMaxSteps(int v)              { m_maxSteps = v; }

    float     GetEpsilon()              const { return m_epsilon; }
    void      SetEpsilon(float v)             { m_epsilon = v; }

    float     GetNormalMult()           const { return m_normalMult; }
    void      SetNormalMult(float v)          { m_normalMult = v; }

    glm::vec3 GetLightDir()             const { return m_lightDir; }
    void      SetLightDir(glm::vec3 v)        { m_lightDir = v; }

    bool      GetDiscardFragments()     const { return m_discardFragments; }
    void      SetDiscardFragments(bool v)     { m_discardFragments = v; }

    bool      GetDisplayNonConverged()  const { return m_displayNonConverged; }
    void      SetDisplayNonConverged(bool v)  { m_displayNonConverged = v; }

    bool      GetShowFlags()            const { return m_showFlags; }
    void      SetShowFlags(bool v)            { m_showFlags = v; }

    void AcceptGUIVisitor(IGUIVisitor& v)                override;
    void AcceptRendererVisitor(IModelRendererVisitor& v) override;

protected:
    int       m_maxSteps            = 64;
    float     m_epsilon             = 1.0f;
    float     m_normalMult          = 1.0f;
    glm::vec3 m_lightDir            = glm::vec3(0.0f, 5.0f, 0.0f);
    bool      m_discardFragments    = true;
    bool      m_displayNonConverged = false;
    bool      m_showFlags           = false;

private:
    GLuint m_selectedProgramID = 0;

    std::shared_ptr<Texture>               m_conemap;
    std::shared_ptr<IRayMarchingTechnique> m_technique;
    std::vector<std::shared_ptr<Mesh>>     m_meshes;
};
