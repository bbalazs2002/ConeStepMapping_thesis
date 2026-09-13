#include <gtest/gtest.h>
#include "Headers/Manager/SceneManager.h"
#include "Headers/Command/CommandQueue.h"
#include "Headers/Command/CreateRMObjectCommand.h"
#include "Headers/Command/CreateObjObjectCommand.h"
#include "Headers/Command/DeleteObjectCommand.h"
#include "Headers/Command/SetSelectedCommand.h"
#include "Headers/Model/RayMarchedModel.h"
#include "Headers/Model/Model.h"
#include "Interfaces/ISceneObject.h"
#include "Interfaces/IGUIVisitor.h"
#include "Interfaces/IModelRendererVisitor.h"
#include "Headers/Types.h"

// Helper: create a RayMarchedModel with no meshes, no GL resources
static std::shared_ptr<RayMarchedModel> MakeRM(const std::string& name) {
    return std::make_shared<RayMarchedModel>(name);
}

static std::shared_ptr<Model> MakeModel(const std::string& name) {
    return std::make_shared<Model>(name);
}

// IT-01
TEST(SceneManager, AddIncreasesSceneObjectCount) {
    SceneManager mgr;
    EXPECT_EQ(mgr.GetSceneObjects().size(), 0u);
    mgr.Add(MakeRM("s1"));
    EXPECT_EQ(mgr.GetSceneObjects().size(), 1u);
    mgr.Add(MakeRM("s2"));
    EXPECT_EQ(mgr.GetSceneObjects().size(), 2u);
}

// IT-03
TEST(SceneManager, RemoveDecreasesCount) {
    SceneManager mgr;
    auto obj = MakeRM("s1");
    mgr.Add(obj);
    EXPECT_EQ(mgr.GetSceneObjects().size(), 1u);
    mgr.Remove(obj);
    EXPECT_EQ(mgr.GetSceneObjects().size(), 0u);
}

// IT-03: Remove also clears m_selected if the removed object was selected
TEST(SceneManager, RemoveSelectedClearsSelection) {
    SceneManager mgr;
    auto obj = MakeRM("s1");
    mgr.Add(obj);
    mgr.SetSelected(obj.get());
    EXPECT_EQ(mgr.GetSelected(), obj.get());
    mgr.Remove(obj);
    EXPECT_EQ(mgr.GetSelected(), nullptr);
}

// SetSelected + GetSelected
TEST(SceneManager, SetAndGetSelected) {
    SceneManager mgr;
    auto s1 = MakeRM("s1");
    auto s2 = MakeRM("s2");
    mgr.Add(s1);
    mgr.Add(s2);
    mgr.SetSelected(s1.get());
    EXPECT_EQ(mgr.GetSelected(), s1.get());
    mgr.SetSelected(s2.get());
    EXPECT_EQ(mgr.GetSelected(), s2.get());
    mgr.SetSelected(nullptr);
    EXPECT_EQ(mgr.GetSelected(), nullptr);
}

// Clear removes all objects
TEST(SceneManager, ClearEmptiesScene) {
    SceneManager mgr;
    mgr.Add(MakeRM("s1"));
    mgr.Add(MakeRM("s2"));
    mgr.Add(MakeRM("s3"));
    mgr.Clear();
    EXPECT_EQ(mgr.GetSceneObjects().size(), 0u);
}

// IT-01: CreateRMObjectCommand adds to SceneManager on Execute
TEST(SceneManager_Commands, CreateRMCommandAddsObject) {
    SceneManager mgr;
    CommandQueue q;
    q.Push(std::make_unique<CreateRMObjectCommand>(mgr, MakeRM("new_surface")));
    q.Execute();
    EXPECT_EQ(mgr.GetSceneObjects().size(), 1u);
    EXPECT_EQ(mgr.GetSceneObjects()[0]->GetName(), "new_surface");
}

// IT-02: CreateObjObjectCommand adds Model
TEST(SceneManager_Commands, CreateObjCommandAddsObject) {
    SceneManager mgr;
    CommandQueue q;
    q.Push(std::make_unique<CreateObjObjectCommand>(mgr, MakeModel("my_model")));
    q.Execute();
    EXPECT_EQ(mgr.GetSceneObjects().size(), 1u);
    EXPECT_EQ(mgr.GetSceneObjects()[0]->GetName(), "my_model");
}

// IT-03: DeleteObjectCommand removes object on Execute
TEST(SceneManager_Commands, DeleteCommandRemovesObject) {
    SceneManager mgr;
    auto obj = MakeRM("to_delete");
    mgr.Add(obj);
    EXPECT_EQ(mgr.GetSceneObjects().size(), 1u);

    CommandQueue q;
    q.Push(std::make_unique<DeleteObjectCommand>(mgr, obj));
    q.Execute();
    EXPECT_EQ(mgr.GetSceneObjects().size(), 0u);
}

// IT-07: multiple commands in one Execute run in order
TEST(SceneManager_Commands, MultipleCommandsRunInOrder) {
    SceneManager mgr;
    CommandQueue q;
    q.Push(std::make_unique<CreateRMObjectCommand>(mgr, MakeRM("A")));
    q.Push(std::make_unique<CreateRMObjectCommand>(mgr, MakeRM("B")));
    q.Push(std::make_unique<CreateRMObjectCommand>(mgr, MakeRM("C")));
    q.Execute();

    const auto& objs = mgr.GetSceneObjects();
    ASSERT_EQ(objs.size(), 3u);
    EXPECT_EQ(objs[0]->GetName(), "A");
    EXPECT_EQ(objs[1]->GetName(), "B");
    EXPECT_EQ(objs[2]->GetName(), "C");
}

// SetSelectedCommand
TEST(SceneManager_Commands, SetSelectedCommandUpdatesSelection) {
    SceneManager mgr;
    auto obj = MakeRM("s1");
    mgr.Add(obj);

    CommandQueue q;
    q.Push(std::make_unique<SetSelectedCommand>(mgr, obj));
    q.Execute();
    EXPECT_EQ(mgr.GetSelected(), obj.get());
}

// -----------------------------------------------------------------------------
// Update / Render / RenderGUI dispatch — no test previously called these three
// loops, so AcceptGUIVisitor/AcceptRendererVisitor on Model and RayMarchedModel,
// and SceneManager::Update/Render/RenderGUI themselves, were entirely uncovered.
// Fakes below count Visit() calls instead of touching real GL/ImGui state.
// -----------------------------------------------------------------------------

namespace {

class CountingGUIVisitor : public IGUIVisitor {
public:
    int modelVisits = 0, rmVisits = 0;
    void Visit(Model& /*target*/)          override { ++modelVisits; }
    void Visit(RayMarchedModel& /*target*/) override { ++rmVisits; }
};

class CountingRendererVisitor : public IModelRendererVisitor {
public:
    int modelVisits = 0, rmVisits = 0;
    const ISceneObject* lastSelected = nullptr;
    void Visit(const Model& /*target*/)          override { ++modelVisits; }
    void Visit(const RayMarchedModel& /*target*/) override { ++rmVisits; }
    void SetSelected(const ISceneObject* selected) override { lastSelected = selected; }
};

} // namespace

TEST(SceneManager, RenderGUIVisitsEveryObject) {
    SceneManager mgr;
    mgr.Add(MakeRM("rm1"));
    mgr.Add(MakeModel("m1"));
    mgr.Add(MakeRM("rm2"));

    CountingGUIVisitor visitor;
    mgr.RenderGUI(visitor);

    EXPECT_EQ(visitor.rmVisits, 2);
    EXPECT_EQ(visitor.modelVisits, 1);
}

TEST(SceneManager, RenderVisitsOnlyRendererVisitablesAndPropagatesSelection) {
    SceneManager mgr;
    auto rm = MakeRM("rm1");
    mgr.Add(rm);
    mgr.Add(MakeModel("m1"));
    mgr.SetSelected(rm.get());

    CountingRendererVisitor visitor;
    mgr.Render(visitor);

    EXPECT_EQ(visitor.rmVisits, 1);
    EXPECT_EQ(visitor.modelVisits, 1);
    EXPECT_EQ(visitor.lastSelected, rm.get());
}

TEST(SceneManager, UpdateRunsWithoutErrorOnAllUpdatables) {
    SceneManager mgr;
    mgr.Add(MakeRM("rm1"));
    mgr.Add(MakeModel("m1"));
    mgr.Add(MakeRM("rm2"));

    SUpdateInfo info;
    info.DeltaTimeInSec = 0.016f;
    info.ElapsedTimeInSec = 1.0f;
    EXPECT_NO_THROW(mgr.Update(info));
}

// -----------------------------------------------------------------------------
// Model — trivial getters/setters that no test previously called. No GL calls
// happen here (plain field storage), so these belong in the no-GL suite.
// -----------------------------------------------------------------------------

TEST(Model, ProgramIDStoredAndReturned) {
    Model m("m");
    EXPECT_EQ(m.GetProgramID(), 0u);
    m.SetProgram(42);
    EXPECT_EQ(m.GetProgramID(), 42u);
}

TEST(Model, WireframeFlagStoredAndReturned) {
    Model m("m");
    EXPECT_FALSE(m.IsWireframe());
    m.SetWireframe(true);
    EXPECT_TRUE(m.IsWireframe());
}

TEST(Model, SelectedProgramIDStoredAndReturned) {
    Model m("m");
    EXPECT_EQ(m.GetSelectedProgramID(), 0u);
    m.SetSelectedProgram(7);
    EXPECT_EQ(m.GetSelectedProgramID(), 7u);
}
