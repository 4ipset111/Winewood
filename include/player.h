#ifndef PLAYER_H
#define PLAYER_H

#include "QuarkCore/QuarkCore.hpp"

using namespace qc;

class Player {
public:
    explicit Player(Vec3 startPosition = Vec3{ 0.0f, 0.0f, 0.0f });

    void Update();

    const Camera3D& GetCamera() const { return m_Camera; }
    Vec3 GetPosition() const { return m_Position; }
    Vec3 GetVelocity() const { return m_Velocity; }
    bool IsGrounded() const { return m_IsGrounded; }

    void SetSensitivity(Vec2 sensitivity) { m_Sensitivity = sensitivity; }
    void SetPosition(Vec3 position) { m_Position = position; }

private:
    void UpdateBody(float rot, float side, float forward, bool jumpPressed, bool crouchHold);
    void UpdateCameraFPS();

    Camera3D m_Camera{};

    Vec3 m_Position{ 0.0f, 0.0f, 0.0f };
    Vec3 m_Velocity{ 0.0f, 0.0f, 0.0f };
    Vec3 m_Direction{ 0.0f, 0.0f, 0.0f };
    bool m_IsGrounded = false;

    Vec2 m_LookRotation{ 0.0f, 0.0f };
    Vec2 m_Sensitivity{ 0.001f, 0.001f };
    Vec2 m_Lean{ 0.0f, 0.0f };

    float m_HeadTimer = 0.0f;
    float m_WalkLerp = 0.0f;
    float m_HeadLerp = 1.0f;
};

#endif // PLAYER_H