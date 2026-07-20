#pragma once

#include <vector>
#include <memory>

interface ISceneObject;
interface IUpdatable;
interface IModelRendererVisitable;
interface IModelRendererVisitor;
interface IGUIVisitor;
struct SUpdateInfo;

class SceneManager {
public:
    ~SceneManager(); // defined in .cpp where ISceneObject is complete

    void Add       (std::shared_ptr<ISceneObject> s);
    void Remove    (const std::shared_ptr<ISceneObject>& s); // also clears m_selected if the removed object was selected
    void Update    (const SUpdateInfo& info);
    void Render    (IModelRendererVisitor& v);               // syncs m_selected into v before iterating
    void RenderGUI (IGUIVisitor& v);
    void Clear     ();

    const std::vector<std::shared_ptr<ISceneObject>>& GetSceneObjects() const;

    void                SetSelected(const ISceneObject* selected) { m_selected = selected; }
    const ISceneObject* GetSelected() const                       { return m_selected; }

private:
    std::vector<std::shared_ptr<ISceneObject>>            m_sceneObjects;
    std::vector<std::shared_ptr<IUpdatable>>              m_updatables;
    std::vector<std::shared_ptr<IModelRendererVisitable>> m_rendererVisitables;

    const ISceneObject* m_selected = nullptr;
};
