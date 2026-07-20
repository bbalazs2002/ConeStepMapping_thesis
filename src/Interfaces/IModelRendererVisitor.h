#pragma once

class Model;
class RayMarchedModel;
interface ISceneObject;

interface IModelRendererVisitor {
public:
    virtual void Visit(const Model& target)          = 0;
    virtual void Visit(const RayMarchedModel& target) = 0;

    // Called by SceneManager::Render() before iterating objects.
    // Default is a no-op so existing visitors without selection support compile unchanged.
    virtual void SetSelected(const ISceneObject* selected) {}

    virtual ~IModelRendererVisitor() = default;
};
