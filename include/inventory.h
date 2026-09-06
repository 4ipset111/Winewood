#ifndef INVENTORY_H
#define INVENTORY_H

#include "QuarkCore/QuarkCore.hpp"

#include <array>
#include <string>

namespace game {

struct InventorySlot {
    int id = 0;
    std::string name;
    std::string category;
    std::string description;
    int count = 0;
    int maxStack = 64;
    qc::Color color = qc::WHITE;
    bool empty = true;
};

class Inventory {
public:
    static constexpr int TOTAL_SLOTS = 25;
    static constexpr int SLOTS_PER_ROW = 5;
    static constexpr int TOTAL_ROWS = 5;

    bool Initialize();
    void Update(float delta);
    void Draw();
    void Shutdown();

    void Toggle();
    void Open();
    void Close();

    bool IsOpen() const { return m_IsOpen; }
    bool IsVisible() const { return m_SlideProgress > 0.001f; }

private:
    void InitializeSlots();
    void RenderBackpackToTexture();
    void DrawItemIcon(const InventorySlot& slot, const qc::Rectangle& rect);
    void DrawSlotItem(const InventorySlot& slot, const qc::Rectangle& rect, bool isHovered, bool isSelected);

    bool m_IsOpen = false;
    float m_SlideProgress = 0.0f;
    float m_SlideSpeed = 6.0f;

    qc::Model m_BackpackModel{};
    qc::Texture2D m_BackpackTexture{};
    qc::RenderTexture2D m_BackpackRT{};
    qc::Camera3D m_BackpackCamera{};
    qc::Shader m_BackpackShader{};
    qc::Vec3 m_BackpackCenter{ 0.0f, 0.0f, 0.0f };
    float m_BackpackScale = 1.0f;
    float m_BackpackRotation = 0.0f;
    bool m_IsDraggingModel = false;
    qc::Vec2 m_LastDragMousePos{ 0.0f, 0.0f };

    std::array<InventorySlot, TOTAL_SLOTS> m_Slots{};
    int m_HoveredSlot = -1;
    int m_SelectedSlot = -1;
};

}

#endif // INVENTORY_H
