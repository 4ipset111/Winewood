#include "inventory.h"

using namespace qc;

#include <algorithm>
#include <cmath>

namespace game {

namespace {

const char* kBackpackVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord0;

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 normalMatrix;

out vec3 vFragPos;
out vec3 vNormal;
out vec2 vTexCoord;

void main() {
    vFragPos = vec3(model * vec4(aPosition, 1.0));
    vNormal = normalize(mat3(normalMatrix) * aNormal);
    vTexCoord = aTexCoord0;
    gl_Position = mvp * vec4(aPosition, 1.0);
}
)";

const char* kBackpackFragmentShader = R"(#version 330 core
in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 fragColor;

void main() {
    vec4 tex = texture(texture0, vTexCoord);
    vec3 n = normalize(vNormal);

    vec3 lightDir1 = normalize(vec3(0.5, 0.75, 0.8));
    float diff1 = max(dot(n, lightDir1), 0.0);

    vec3 lightDir2 = normalize(vec3(-0.6, -0.2, -0.6));
    float diff2 = max(dot(n, lightDir2), 0.0) * 0.3;

    vec3 viewDir = vec3(0.0, 0.0, 1.0);
    float rim = pow(1.0 - max(dot(n, viewDir), 0.0), 3.0) * 0.2;

    vec3 ambient = vec3(0.30, 0.32, 0.35);
    vec3 lighting = ambient + vec3(0.80, 0.78, 0.75) * diff1 + vec3(0.20, 0.24, 0.28) * diff2 + vec3(rim);

    fragColor = vec4(tex.rgb * colDiffuse.rgb * lighting, tex.a * colDiffuse.a);
}
)";

}

bool Inventory::Initialize() {
    InitializeSlots();

    m_BackpackModel = LoadModel("resources/models/backpack/backpack.obj");
    if (m_BackpackModel.meshCount > 0) {
        BoundingBox box = GetModelBoundingBox(m_BackpackModel);
        m_BackpackCenter = Vec3{
            (box.min.x + box.max.x) * 0.5f,
            (box.min.y + box.max.y) * 0.5f,
            (box.min.z + box.max.z) * 0.5f
        };

        float dx = box.max.x - box.min.x;
        float dy = box.max.y - box.min.y;
        float dz = box.max.z - box.min.z;
        float maxDim = std::max({ dx, dy, dz });
        m_BackpackScale = (maxDim > 0.001f) ? (2.1f / maxDim) : 1.0f;

        m_BackpackTexture = LoadTexture("resources/models/backpack/RGB_Alpha.png");
        if (m_BackpackTexture.valid) {
            for (int i = 0; i < m_BackpackModel.materialCount; ++i) {
                if (m_BackpackModel.materials[i].maps) {
                    m_BackpackModel.materials[i].maps[MATERIAL_MAP_ALBEDO].texture = m_BackpackTexture;
                    m_BackpackModel.materials[i].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
                }
            }
        }

        m_BackpackShader = LoadShaderFromMemory(kBackpackVertexShader, kBackpackFragmentShader);
        if (IsShaderValid(m_BackpackShader)) {
            for (int i = 0; i < m_BackpackModel.materialCount; ++i) {
                m_BackpackModel.materials[i].shader = &m_BackpackShader;
            }
        }
    } else {
        TraceLog(LogLevel::Warn, "INVENTORY", "Could not load backpack model: resources/models/backpack/backpack.obj");
    }

    constexpr int rtDim = 296;
    m_BackpackRT = LoadRenderTexture(rtDim, rtDim);

    m_BackpackCamera.position = Vec3{ 0.0f, 0.0f, 3.4f };
    m_BackpackCamera.target = Vec3{ 0.0f, 0.0f, 0.0f };
    m_BackpackCamera.up = Vec3{ 0.0f, 1.0f, 0.0f };
    m_BackpackCamera.fovy = 45.0f;
    m_BackpackCamera.projection = CAMERA_PERSPECTIVE;

    return true;
}

void Inventory::InitializeSlots() {
    for (int i = 0; i < TOTAL_SLOTS; ++i) {
        m_Slots[i] = { i, "", "", "", 0, 64, WHITE, true };
    }
}

void Inventory::Toggle() {
    if (m_IsOpen) {
        Close();
    } else {
        Open();
    }
}

void Inventory::Open() {
    m_IsOpen = true;
    m_SelectedSlot = -1;
}

void Inventory::Close() {
    m_IsOpen = false;
    m_SelectedSlot = -1;
    m_IsDraggingModel = false;
}

void Inventory::Update(float delta) {
    const float target = m_IsOpen ? 1.0f : 0.0f;
    if (m_SlideProgress < target) {
        m_SlideProgress = std::min(target, m_SlideProgress + m_SlideSpeed * delta);
    } else if (m_SlideProgress > target) {
        m_SlideProgress = std::max(target, m_SlideProgress - m_SlideSpeed * delta);
    }

    if (!IsVisible()) return;

    if (!m_IsDraggingModel) {
        m_BackpackRotation += delta * 1.35f;
        if (m_BackpackRotation >= 2.0f * PI) {
            m_BackpackRotation -= 2.0f * PI;
        }
    }

    const Vec2 mousePos = GetMousePosition();

    const float screenW = static_cast<float>(GetScreenWidth());
    const float screenH = static_cast<float>(GetScreenHeight());

    constexpr float slotSize = 56.0f;
    constexpr float slotGap = 4.0f;
    constexpr float gridDim = TOTAL_ROWS * slotSize + (TOTAL_ROWS - 1) * slotGap;

    constexpr float margin = 14.0f;
    constexpr float gap = 12.0f;
    constexpr float headerH = 30.0f;
    constexpr float footerH = 30.0f;

    constexpr float panelW = margin + gridDim + gap + gridDim + margin;
    constexpr float panelH = margin + headerH + 6.0f + gridDim + 6.0f + footerH + margin;

    const float panelY = (screenH - panelH) * 0.5f;
    const float targetOpenX = screenW - panelW - 20.0f;
    const float closedX = screenW + 20.0f;
    const float invP = 1.0f - m_SlideProgress;
    const float easedT = 1.0f - invP * invP * invP;
    const float panelX = closedX + (targetOpenX - closedX) * easedT;

    const Rectangle backpackBox{ panelX + margin, panelY + margin + headerH + 6.0f, gridDim, gridDim };

    if (m_IsOpen) {
        if (IsMouseButtonPressed(MouseButton::Left) && CheckCollisionPointRec(mousePos, backpackBox)) {
            m_IsDraggingModel = true;
            m_LastDragMousePos = mousePos;
        }

        if (m_IsDraggingModel) {
            if (IsMouseButtonDown(MouseButton::Left)) {
                const float dx = mousePos.x - m_LastDragMousePos.x;
                m_BackpackRotation -= dx * 0.015f;
                m_LastDragMousePos = mousePos;
            } else {
                m_IsDraggingModel = false;
            }
        }

        const float gridX = panelX + margin + gridDim + gap;
        const float gridY = panelY + margin + headerH + 6.0f;

        m_HoveredSlot = -1;
        for (int row = 0; row < TOTAL_ROWS; ++row) {
            for (int col = 0; col < SLOTS_PER_ROW; ++col) {
                const int index = row * SLOTS_PER_ROW + col;
                const Rectangle slotRect{
                    gridX + col * (slotSize + slotGap),
                    gridY + row * (slotSize + slotGap),
                    slotSize,
                    slotSize
                };

                if (CheckCollisionPointRec(mousePos, slotRect)) {
                    m_HoveredSlot = index;
                    break;
                }
            }
            if (m_HoveredSlot != -1) break;
        }

        if (IsMouseButtonPressed(MouseButton::Left) && m_HoveredSlot != -1) {
            if (m_SelectedSlot == -1) {
                if (!m_Slots[m_HoveredSlot].empty) {
                    m_SelectedSlot = m_HoveredSlot;
                }
            } else {
                if (m_SelectedSlot == m_HoveredSlot) {
                    m_SelectedSlot = -1;
                } else {
                    std::swap(m_Slots[m_SelectedSlot], m_Slots[m_HoveredSlot]);
                    m_Slots[m_SelectedSlot].id = m_SelectedSlot;
                    m_Slots[m_HoveredSlot].id = m_HoveredSlot;
                    m_SelectedSlot = -1;
                }
            }
        }

        if (m_HoveredSlot != -1 || CheckCollisionPointRec(mousePos, backpackBox)) {
            SetMouseCursor(MouseCursor::PointingHand);
        } else {
            SetMouseCursor(MouseCursor::Default);
        }
    }
}

void Inventory::RenderBackpackToTexture() {
    if (m_BackpackRT.id == 0 || m_BackpackModel.meshCount == 0) return;

    BeginTextureMode(m_BackpackRT);
    ClearBackground(Color{ 0, 0, 0, 0 });
    BeginMode3D(m_BackpackCamera);

    const Mat4 transform = Mat4::scale(m_BackpackScale, m_BackpackScale, m_BackpackScale) *
                           Mat4::rotationX(0.12f) *
                           Mat4::rotationY(-m_BackpackRotation) *
                           Mat4::translation(-m_BackpackCenter.x, -m_BackpackCenter.y, -m_BackpackCenter.z);

    DrawModelEx(m_BackpackModel, transform);

    EndMode3D();
    EndTextureMode();
}

void Inventory::DrawItemIcon(const InventorySlot& slot, const Rectangle& rect) {
    const float cx = rect.x + rect.width * 0.5f;
    const float cy = rect.y + rect.height * 0.5f - 2.0f;

    if (slot.category == "WEAPON") {
        DrawRectangle(static_cast<int>(cx - 14), static_cast<int>(cy - 5), 26, 6, slot.color);
        DrawRectangle(static_cast<int>(cx - 7), static_cast<int>(cy + 1), 8, 11, slot.color);
        DrawRectangle(static_cast<int>(cx - 4), static_cast<int>(cy + 3), 5, 9, Color{18, 20, 24, 255});
    } else if (slot.category == "AMMO") {
        for (int i = -1; i <= 1; ++i) {
            const float ox = cx + i * 7.0f;
            DrawRectangle(static_cast<int>(ox - 2), static_cast<int>(cy - 2), 4, 12, Color{160, 130, 45, 255});
            DrawTriangle(
                Vec2{ ox - 2.0f, cy - 2.0f },
                Vec2{ ox, cy - 7.0f },
                Vec2{ ox + 2.0f, cy - 2.0f },
                Color{175, 80, 35, 255}
            );
        }
    } else if (slot.category == "MEDICAL") {
        DrawRectangle(static_cast<int>(cx - 12), static_cast<int>(cy - 12), 24, 24, Color{120, 26, 26, 255});
        DrawRectangle(static_cast<int>(cx - 2), static_cast<int>(cy - 8), 4, 16, Color{200, 200, 200, 255});
        DrawRectangle(static_cast<int>(cx - 8), static_cast<int>(cy - 2), 16, 4, Color{200, 200, 200, 255});
    } else {
        DrawRectangle(static_cast<int>(cx - 8), static_cast<int>(cy - 8), 16, 16, slot.color);
    }
}

void Inventory::DrawSlotItem(const InventorySlot& slot, const Rectangle& rect, bool isHovered, bool isSelected) {
    Color bgColor = isHovered ? Color{ 14, 16, 19, 255 } : Color{ 4, 5, 6, 255 };
    if (isSelected) bgColor = Color{ 20, 22, 26, 255 };
    DrawRectangleRec(rect, bgColor);

    Color borderColor = Color{ 20, 22, 26, 255 };
    float borderWidth = 1.0f;
    if (isHovered) {
        borderColor = Color{ 70, 75, 85, 255 };
        borderWidth = 1.0f;
    }
    if (isSelected) {
        borderColor = Color{ 160, 125, 30, 255 };
        borderWidth = 1.0f;
    }
    DrawRectangleLinesEx(rect, borderWidth, borderColor);

    if (!slot.empty) {
        DrawRectangle(
            static_cast<int>(rect.x + 2),
            static_cast<int>(rect.y + rect.height - 3),
            static_cast<int>(rect.width - 4),
            1,
            slot.color
        );

        DrawItemIcon(slot, rect);

        if (slot.count > 1) {
            const char* countText = TextFormat("%d", slot.count);
            const int tw = MeasureText(countText, 11);
            DrawRectangle(
                static_cast<int>(rect.x + rect.width - tw - 4),
                static_cast<int>(rect.y + rect.height - 14),
                tw + 2,
                11,
                Color{ 2, 3, 4, 230 }
            );
            DrawDebugText(
                countText,
                static_cast<int>(rect.x + rect.width - tw - 3),
                static_cast<int>(rect.y + rect.height - 14),
                11,
                Color{ 180, 185, 195, 255 }
            );
        }
    }
}

void Inventory::Draw() {
    if (!IsVisible()) return;

    RenderBackpackToTexture();

    const float screenW = static_cast<float>(GetScreenWidth());
    const float screenH = static_cast<float>(GetScreenHeight());

    const unsigned char overlayAlpha = static_cast<unsigned char>(160.0f * m_SlideProgress);
    DrawRectangle(0, 0, static_cast<int>(screenW), static_cast<int>(screenH), Color{ 0, 0, 0, overlayAlpha });

    constexpr float slotSize = 56.0f;
    constexpr float slotGap = 4.0f;
    constexpr float gridDim = TOTAL_ROWS * slotSize + (TOTAL_ROWS - 1) * slotGap;

    constexpr float margin = 14.0f;
    constexpr float gap = 12.0f;
    constexpr float headerH = 30.0f;
    constexpr float footerH = 30.0f;

    constexpr float panelW = margin + gridDim + gap + gridDim + margin;
    constexpr float panelH = margin + headerH + 6.0f + gridDim + 6.0f + footerH + margin;

    const float panelY = (screenH - panelH) * 0.5f;

    const float targetOpenX = screenW - panelW - 20.0f;
    const float closedX = screenW + 20.0f;
    const float invP = 1.0f - m_SlideProgress;
    const float easedT = 1.0f - invP * invP * invP;
    const float panelX = closedX + (targetOpenX - closedX) * easedT;

    const Rectangle panelRect{ panelX, panelY, panelW, panelH };

    DrawRectangleRec(panelRect, Color{ 2, 2, 2, 255 });
    DrawRectangleLinesEx(panelRect, 1.0f, Color{ 22, 24, 28, 255 });

    const Rectangle headerRect{ panelX + margin, panelY + margin, panelW - margin * 2.0f, headerH };
    DrawRectangleRec(headerRect, Color{ 6, 7, 8, 255 });
    DrawRectangleLinesEx(headerRect, 1.0f, Color{ 18, 20, 24, 255 });

    DrawDebugText("INVENTORY", static_cast<int>(headerRect.x + 10), static_cast<int>(headerRect.y + 7), 16, Color{ 150, 155, 165, 255 });
    DrawDebugText("[TAB]", static_cast<int>(headerRect.x + headerRect.width - 45), static_cast<int>(headerRect.y + 8), 14, Color{ 80, 85, 95, 255 });

    const Rectangle backpackBox{ panelX + margin, panelY + margin + headerH + 6.0f, gridDim, gridDim };
    DrawRectangleRec(backpackBox, Color{ 0, 0, 0, 255 });
    DrawRectangleLinesEx(backpackBox, 1.0f, Color{ 18, 20, 24, 255 });

    DrawDebugText("BACKPACK", static_cast<int>(backpackBox.x + 8), static_cast<int>(backpackBox.y + 8), 13, Color{ 100, 105, 115, 255 });

    const Rectangle srcRT{ 0.0f, static_cast<float>(m_BackpackRT.texture.height), static_cast<float>(m_BackpackRT.texture.width), -static_cast<float>(m_BackpackRT.texture.height) };
    const Rectangle dstRT{ backpackBox.x + 4.0f, backpackBox.y + 4.0f, backpackBox.width - 8.0f, backpackBox.height - 8.0f };
    DrawTexturePro(m_BackpackRT.texture, srcRT, dstRT, Vec2{ 0.0f, 0.0f }, 0.0f, WHITE);

    const float gridX = panelX + margin + gridDim + gap;
    const float gridY = panelY + margin + headerH + 6.0f;

    const Rectangle gridBorderBox{ gridX - 2.0f, gridY - 2.0f, gridDim + 4.0f, gridDim + 4.0f };
    DrawRectangleLinesEx(gridBorderBox, 1.0f, Color{ 14, 16, 18, 255 });

    for (int row = 0; row < TOTAL_ROWS; ++row) {
        for (int col = 0; col < SLOTS_PER_ROW; ++col) {
            const int index = row * SLOTS_PER_ROW + col;
            const Rectangle slotRect{
                gridX + col * (slotSize + slotGap),
                gridY + row * (slotSize + slotGap),
                slotSize,
                slotSize
            };
            const bool isHovered = (m_HoveredSlot == index);
            const bool isSelected = (m_SelectedSlot == index);
            DrawSlotItem(m_Slots[index], slotRect, isHovered, isSelected);
        }
    }

    const Rectangle footerRect{ panelX + margin, gridY + gridDim + 6.0f, panelW - margin * 2.0f, footerH };
    DrawRectangleRec(footerRect, Color{ 5, 6, 7, 255 });
    DrawRectangleLinesEx(footerRect, 1.0f, Color{ 16, 18, 22, 255 });

    if (m_HoveredSlot != -1) {
        const auto& slot = m_Slots[m_HoveredSlot];
        if (!slot.empty) {
            DrawDebugText(slot.name.c_str(), static_cast<int>(footerRect.x + 10), static_cast<int>(footerRect.y + 8), 13, slot.color);
        } else {
            DrawDebugText(TextFormat("EMPTY | %02d", m_HoveredSlot + 1), static_cast<int>(footerRect.x + 10), static_cast<int>(footerRect.y + 8), 13, Color{ 70, 75, 85, 255 });
        }
    } else if (m_SelectedSlot != -1 && !m_Slots[m_SelectedSlot].empty) {
        const auto& slot = m_Slots[m_SelectedSlot];
        DrawDebugText(slot.name.c_str(), static_cast<int>(footerRect.x + 10), static_cast<int>(footerRect.y + 8), 13, Color{ 160, 125, 30, 255 });
    } else {
        DrawDebugText("No one slot selected.", static_cast<int>(footerRect.x + 10), static_cast<int>(footerRect.y + 8), 12, Color{ 55, 60, 70, 255 });
    }
}

void Inventory::Shutdown() {
    if (m_BackpackRT.id != 0) {
        UnloadRenderTexture(m_BackpackRT);
        m_BackpackRT = {};
    }
    if (IsShaderValid(m_BackpackShader)) {
        UnloadShader(m_BackpackShader);
        m_BackpackShader = {};
    }
    if (m_BackpackTexture.valid) {
        UnloadTexture(m_BackpackTexture);
        m_BackpackTexture = {};
    }
    if (m_BackpackModel.meshCount > 0) {
        UnloadModel(m_BackpackModel);
        m_BackpackModel = {};
    }
}

}
