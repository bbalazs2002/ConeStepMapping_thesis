#pragma once

#include <GL/glew.h>

#include "Interfaces/IModelRendererVisitor.h"

interface ICamera;
class ModelBase;
interface ISceneObject;
struct RayMarchDebugState;

class OpenGLRendererVisitor : public IModelRendererVisitor {
public:
    explicit OpenGLRendererVisitor(const ICamera& cam);
    ~OpenGLRendererVisitor() = default;
    OpenGLRendererVisitor(const OpenGLRendererVisitor&)            = delete;
    OpenGLRendererVisitor& operator=(const OpenGLRendererVisitor&) = delete;

    void SetSelected  (const ISceneObject* selected) override { m_selected = selected; }
    void SetDebugState(RayMarchDebugState* state)             { m_debugState = state; }

    void Visit(const Model& target)           override;
    void Visit(const RayMarchedModel& target) override;

private:
    void VisitModelBase(const ModelBase& target);

    const ICamera&       m_camera;
    const ISceneObject*  m_selected   = nullptr;
    RayMarchDebugState*  m_debugState = nullptr;
};