#include "Headers/Manager/SceneManager.h"
#include "Interfaces/ISceneObject.h"
#include "Interfaces/IUpdatable.h"
#include "Interfaces/IGUIVisitor.h"
#include "Interfaces/IModelRendererVisitable.h"
#include "Interfaces/IModelRendererVisitor.h"
#include "Headers/Types.h"

#include <algorithm>

SceneManager::~SceneManager() = default;

void SceneManager::Add(std::shared_ptr<ISceneObject> s)
{
    m_sceneObjects.push_back(s);
    if (auto u = std::dynamic_pointer_cast<IUpdatable>(s))
        m_updatables.push_back(u);
    if (auto r = std::dynamic_pointer_cast<IModelRendererVisitable>(s))
        m_rendererVisitables.push_back(r);
}

void SceneManager::Remove(const std::shared_ptr<ISceneObject>& s)
{
    if (m_selected == s.get())
        m_selected = nullptr;

    void* raw = dynamic_cast<void*>(s.get());

    auto eraseBy = [raw](auto& vec) {
        vec.erase(std::remove_if(vec.begin(), vec.end(),
            [raw](const auto& p) { return dynamic_cast<void*>(p.get()) == raw; }),
            vec.end());
    };

    eraseBy(m_sceneObjects);
    eraseBy(m_updatables);
    eraseBy(m_rendererVisitables);
}

void SceneManager::Update(const SUpdateInfo& info)
{
    for (auto& u : m_updatables)
        u->Update(info);
}

void SceneManager::Render(IModelRendererVisitor& v)
{
    v.SetSelected(m_selected);
    for (auto& r : m_rendererVisitables)
        r->AcceptRendererVisitor(v);
}

void SceneManager::RenderGUI(IGUIVisitor& v)
{
    for (auto& s : m_sceneObjects)
        s->AcceptGUIVisitor(v);
}

void SceneManager::Clear()
{
    m_selected = nullptr;
    m_sceneObjects.clear();
    m_updatables.clear();
    m_rendererVisitables.clear();
}

const std::vector<std::shared_ptr<ISceneObject>>& SceneManager::GetSceneObjects() const
{
    return m_sceneObjects;
}
