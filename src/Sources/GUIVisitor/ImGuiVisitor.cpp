#include "Headers/GUIVisitor/ImGuiVisitor.h"
#include "Interfaces/ICommandQueue.h"
#include "Headers/Model/ModelBase.h"
#include "Headers/Model/Model.h"
#include "Headers/Model/RayMarchedModel.h"
#include "Headers/Command/SetLocationCommand.h"
#include "Headers/Command/SetRotationCommand.h"
#include "Headers/Command/SetScaleCommand.h"
#include "Headers/Command/SetMaxStepsCommand.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>

ImGuiVisitor::ImGuiVisitor(ICommandQueue& queue)
    : m_commandQueue(queue)
{
}

// -- Common controls for all models -------------------------------------------

void ImGuiVisitor::VisitModelBase(ModelBase& target)
{
    char nameBuf[256];
    snprintf(nameBuf, sizeof(nameBuf), "%s", target.GetName().c_str());
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
        target.SetName(nameBuf);

    bool show = target.IsShown();
    if (ImGui::Checkbox("Show", &show))
        target.SetShow(show);

    auto basePtr = target.shared_from_this();

    glm::vec3 loc = target.GetTransform().GetLocation();
    if (ImGui::DragFloat3("Location", glm::value_ptr(loc), 0.01f))
        m_commandQueue.Push(std::make_unique<SetLocationCommand>(basePtr, loc));

    glm::vec3 euler = glm::degrees(glm::eulerAngles(target.GetTransform().GetRotation()));
    if (ImGui::DragFloat3("Rotation (deg)", glm::value_ptr(euler), 0.5f))
        m_commandQueue.Push(std::make_unique<SetRotationCommand>(basePtr, glm::quat(glm::radians(euler))));

    glm::vec3 scale = target.GetTransform().GetScale();
    if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.01f, 0.001f, 100.0f))
        m_commandQueue.Push(std::make_unique<SetScaleCommand>(basePtr, scale));
}

// -- Visit(Model) -------------------------------------------------------------

void ImGuiVisitor::Visit(Model& target)
{
    ImGui::PushID(&target);
    VisitModelBase(target);

    bool wf = target.IsWireframe();
    if (ImGui::Checkbox("Wireframe", &wf))
        target.SetWireframe(wf);

    ImGui::PopID();
}

// -- Visit(RayMarchedModel) ---------------------------------------------------

void ImGuiVisitor::Visit(RayMarchedModel& target)
{
    ImGui::PushID(&target);
    VisitModelBase(target);

    if (ImGui::TreeNode("Ray Marching")) {
        auto ptr = std::dynamic_pointer_cast<RayMarchedModel>(target.shared_from_this());

        int maxSteps = target.GetMaxSteps();
        if (ImGui::SliderInt("Max Steps", &maxSteps, 1, 256))
            m_commandQueue.Push(std::make_unique<SetMaxStepsCommand>(ptr, maxSteps));

        float normalMult = target.GetNormalMult();
        if (ImGui::DragFloat("Normal Mult", &normalMult, 0.01f, 0.0f, 10.0f))
            target.SetNormalMult(normalMult);

        bool discard = target.GetDiscardFragments();
        if (ImGui::Checkbox("Discard Fragments", &discard))
            target.SetDiscardFragments(discard);

        bool displayNC = target.GetDisplayNonConverged();
        if (ImGui::Checkbox("Display Non-Converged", &displayNC))
            target.SetDisplayNonConverged(displayNC);

        ImGui::TreePop();
    }

    ImGui::PopID();
}
