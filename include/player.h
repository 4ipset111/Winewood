#ifndef PLAYER_H
#define PLAYER_H

#include "QuarkCore/QuarkCore.hpp"
using namespace qc;

#include <vector>

class Player {
public:
    struct Collider {
        Vec3 min;
        Vec3 max;
        bool isGround = false;
        struct Triangle {
            Vec3 a;
            Vec3 b;
            Vec3 c;
        };
        std::vector<Triangle> triangles;
    };

    explicit Player(Vec3 startPosition = Vec3{ 0.0f, 0.0f, 0.0f });

    void Update();
    void ApplyRecoil(float pitch);

    const Camera3D& GetCamera() const { return m_Camera; }
    Vec3 GetPosition() const { return m_Position; }
    Vec3 GetVelocity() const { return m_Velocity; }
    bool IsGrounded() const { return m_IsGrounded; }
    void DrawDebugCollision(Color color) const;

    void SetSensitivity(Vec2 sensitivity) { m_Sensitivity = sensitivity; }
    void SetPosition(Vec3 position) { m_Position = position; }
    void SetColliders(const std::vector<Collider>& vColliders) { m_vColliders = vColliders; }

private:
    void UpdateBody(float rot, float side, float forward, bool jumpPressed, bool crouchHold);
    void UpdateCameraFPS();
    void ResolveCollisions(Vec3 previousPosition);

    Camera3D m_Camera{};

    Vec3 m_Position{ 0.0f, 0.0f, 0.0f };
    Vec3 m_Velocity{ 0.0f, 0.0f, 0.0f };
    Vec3 m_Direction{ 0.0f, 0.0f, 0.0f };
    bool m_IsGrounded = false;
    float m_JumpBufferTimer = 0.0f;
    std::vector<Collider> m_vColliders;

    Vec2 m_LookRotation{ 0.0f, 0.0f };
    Vec2 m_Sensitivity{ 0.001f, 0.001f };
    Vec2 m_Lean{ 0.0f, 0.0f };

    float m_HeadTimer = 0.0f;
    float m_WalkLerp = 0.0f;
    float m_HeadLerp = 1.0f;
    float m_RecoilPitch = 0.0f;
};

#endif // PLAYER_H