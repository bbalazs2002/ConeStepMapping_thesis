#pragma once

#include <memory>
#include <vector>
#include <GL/glew.h>

#include "ModelBase.h"
#include "Headers/Model/Mesh.h"

class Model : public ModelBase {
public:
    explicit Model(std::string name = "unnamed model");

    GLuint GetProgramID()         const override { return m_programID; }
    void   SetProgram(GLuint id)               { m_programID = id; }

    GLuint GetSelectedProgramID() const          { return m_selectedProgramID; }
    void   SetSelectedProgram(GLuint id)         { m_selectedProgramID = id; }

    bool IsWireframe()        const { return m_wireframe; }
    void SetWireframe(bool wf)      { m_wireframe = wf; }

    void AddMesh(std::shared_ptr<Mesh> mesh) { m_meshes.push_back(std::move(mesh)); }
    const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return m_meshes; }

    void AcceptGUIVisitor(IGUIVisitor& v)              override;
    void AcceptRendererVisitor(IModelRendererVisitor& v) override;

protected:
    GLuint m_programID         = 0;
    GLuint m_selectedProgramID = 0;
    bool   m_wireframe         = false;

private:
    std::vector<std::shared_ptr<Mesh>> m_meshes;
};
