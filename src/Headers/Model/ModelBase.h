#pragma once

#include <memory>
#include <string>
#include <GL/glew.h>

#include "Interfaces/ISceneObject.h"
#include "Interfaces/IModelRendererVisitable.h"
#include "Interfaces/IUpdatable.h"
#include "Headers/Transform/Transform.h"

class ModelBase : public ISceneObject,
                  public IModelRendererVisitable,
                  public IUpdatable,
                  public std::enable_shared_from_this<ModelBase>
{
public:
    explicit ModelBase(std::string name = "unnamed");
    virtual ~ModelBase() = default;

    void Update(const SUpdateInfo& info) override;  // IUpdatable

    virtual GLuint GetProgramID() const = 0;

    const std::string& GetName()    const override { return m_name; }
    void               SetName(std::string n)      { m_name = std::move(n); }

    bool IsShown()            const { return m_show; }
    void SetShow(bool show)         { m_show = show; }

    int  GetDrawMode()        const { return m_drawMode; }
    void SetDrawMode(int mode)      { m_drawMode = mode; }

    Transform&       GetTransform()       { return m_transform; }
    const Transform& GetTransform() const { return m_transform; }

protected:
    std::string m_name;
    bool        m_show     = true;
    int         m_drawMode = GL_TRIANGLES;
    Transform   m_transform;
};