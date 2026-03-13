#include "SceneHierarchyView.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>

#include "fmt/format.h"
#include "imgui/imgui.h"

#include "gameplay/component/StaticMeshComponent.h"
#include "gameplay/component/TransformComponent.h"
#include "gameplay/mesh/StaticMesh.h"

namespace shine::editor::views {

void SceneHierarchyView::SetWorldHierarchyService(shine::gameplay::world::IWorldActorHierarchyService* worldService) {
    worldService_ = worldService;
}

void SceneHierarchyView::onInit() {
    SetName("场景层级");
}

void SceneHierarchyView::onShutDown() {
    visibleObjects_.clear();
    contextObject_ = nullptr;
    if (worldService_) {
        worldService_->clearSelection();
    }
}

void SceneHierarchyView::onRender() {
    if (!ImGui::Begin(name.c_str(), &isOpen)) {
        ImGui::End();
        return;
    }

    ImGui::InputTextWithHint("##SceneSearch", "搜索对象或类型", searchBuffer_, sizeof(searchBuffer_));
    ImGui::SameLine();
    if (ImGui::Button("添加")) {
        ImGui::OpenPopup("SceneCreateActorPopup");
    }
    if (ImGui::BeginPopup("SceneCreateActorPopup")) {
        if (ImGui::MenuItem("创建 EmptyActor")) {
            createEmptyActor();
        }
        if (ImGui::MenuItem("创建 StaticMeshActor")) {
            createStaticMeshActor();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    const auto selectedObjects = worldService_ ? worldService_->getSelectedObjectsSnapshot() : std::vector<shine::gameplay::SObject*>{};
    const bool canDelete = std::ranges::any_of(selectedObjects, [this](const shine::gameplay::SObject* obj) {
        return isEditorOwned(obj);
    });
    if (!canDelete) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("删除选中")) {
        deleteSelectedObjects();
    }
    if (!canDelete) {
        ImGui::EndDisabled();
    }

    refreshObjects();
    if (ImGui::TreeNodeEx("SceneRoot", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
        for (int i = 0; i < static_cast<int>(visibleObjects_.size()); ++i) {
            RenderObjectNode(visibleObjects_[i], i);
        }
        ImGui::TreePop();
    }

    if (worldService_ && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
        worldService_->clearSelection();
    }

    if (ImGui::BeginPopup("SceneRenamePopup")) {
        ImGui::InputText("新名称", renameBuffer_, sizeof(renameBuffer_));
        if (ImGui::Button("确认")) {
            if (contextObject_) {
                contextObject_->setName(renameBuffer_);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void SceneHierarchyView::refreshObjects() {
    visibleObjects_.clear();
    if (!worldService_) {
        return;
    }

    auto objects = worldService_->getAllActorsSnapshot();
    std::ranges::sort(objects, [](const shine::gameplay::SObject* a, const shine::gameplay::SObject* b) {
        return a->getName() < b->getName();
    });

    std::string filter = searchBuffer_;
    std::ranges::transform(filter, filter.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (auto* obj : objects) {
        // if (!obj) {
        //     continue;
        // }
        // if (filter.empty()) {
        //     visibleObjects_.push_back(obj);
        //     continue;
        // }

        // STextView objectName = obj->getName();
        // std::string className = obj->getClassName();
        // std::ranges::transform(objectName, objectName.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        // std::ranges::transform(className, className.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        // if (objectName.find(filter) != std::string::npos || className.find(filter) != std::string::npos) {
        //     visibleObjects_.push_back(obj);
        // }
    }
}

void SceneHierarchyView::RenderObjectNode(shine::gameplay::SObject* obj, int index) {
    if (!obj) {
        return;
    }

    const bool isSelected = worldService_ && worldService_->isSelected(obj);
    const std::string displayName = obj->getName().empty() ? fmt::format("Object_{}", index) : obj->getName().to_string();
    const std::string label = fmt::format("{} [{}]", displayName, obj->getClassName());
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (isSelected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    ImGui::TreeNodeEx(reinterpret_cast<void*>(obj), flags, "%s", label.c_str());
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        if (worldService_) {
            if (ImGui::GetIO().KeyCtrl) {
                worldService_->toggleSelectedObject(obj);
            } else {
                worldService_->setSelectedObject(obj);
            }
        }
    } else if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        if (worldService_) {
            worldService_->setSelectedObject(obj);
        }
    }

    if (ImGui::BeginPopupContextItem()) {
        contextObject_ = obj;
        if (ImGui::MenuItem("重命名")) {
            std::snprintf(renameBuffer_, sizeof(renameBuffer_), "%s", obj->getName().data());
            ImGui::OpenPopup("SceneRenamePopup");
        }
        const bool owned = isEditorOwned(obj);
        if (!owned) {
            ImGui::BeginDisabled();
        }
        if (ImGui::MenuItem("删除")) {
            deleteObject(obj);
        }
        if (!owned) {
            ImGui::EndDisabled();
        }
        ImGui::EndPopup();
    }
}

void SceneHierarchyView::createEmptyActor() {
    if (!worldService_) {
        return;
    }
    auto actor = std::make_unique<shine::gameplay::EmptyActor>();
    actor->setName(STextView::from_string(fmt::format("EmptyActor_{}", nextEmptyActorId_++)));
    actor->addComponent<shine::gameplay::component::TransformComponent>();
    auto* actorPtr = actor.get();
    worldService_->addActorToPersistentLevel(std::move(actor));
    worldService_->setSelectedObject(actorPtr);
}

void SceneHierarchyView::createStaticMeshActor() {
    if (!worldService_) {
        return;
    }
    auto actor = std::make_unique<shine::gameplay::StaticMeshActor>();
    actor->setName(STextView::from_string(fmt::format("StaticMeshActor_{}", nextStaticMeshActorId_++)));
    auto* transform = actor->addComponent<shine::gameplay::component::TransformComponent>();
    transform->setScale({0.35f, 0.35f, 0.35f});
    auto* meshComp = actor->addComponent<shine::gameplay::component::StaticMeshComponent>();
    auto mesh = std::make_shared<shine::gameplay::StaticMesh>();
    mesh->initCubeWithNormals();
    meshComp->setMesh(mesh);
    auto* actorPtr = actor.get();
    worldService_->addActorToPersistentLevel(std::move(actor));
    worldService_->setSelectedObject(actorPtr);
}

void SceneHierarchyView::deleteSelectedObjects() {
    if (!worldService_) {
        return;
    }
    const auto selectedObjects = worldService_->getSelectedObjectsSnapshot();
    for (auto* obj : selectedObjects) {
        deleteObject(obj);
    }
}

void SceneHierarchyView::deleteObject(shine::gameplay::SObject* obj) {
    if (!obj || !isEditorOwned(obj)) {
        return;
    }
    if (worldService_) {
        worldService_->removeActor(obj);
    }
}

void SceneHierarchyView::SetSelectedObject(shine::gameplay::SObject* obj) {
    if (worldService_) worldService_->setSelectedObject(obj);
}

bool SceneHierarchyView::isEditorOwned(const shine::gameplay::SObject* obj) const {
    if (!obj) {
        return false;
    }
    const auto name = obj->getName();
    return name.rfind("EmptyActor_", 0) == 0 || name.rfind("StaticMeshActor_", 0) == 0;
}

} // namespace shine::editor::views
