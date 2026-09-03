#include "player.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr float GRAVITY = 32.0f;
    constexpr float MAX_SPEED = 20.0f;
    constexpr float CROUCH_SPEED = 5.0f;
    constexpr float JUMP_FORCE = 12.0f;
    constexpr float MAX_ACCEL = 150.0f;
    constexpr float FRICTION = 0.86f;
    constexpr float AIR_DRAG = 0.98f;
    constexpr float CONTROL = 15.0f;
    constexpr float CROUCH_HEIGHT = 0.0f;
    constexpr float STAND_HEIGHT = 1.0f;
    constexpr float BOTTOM_HEIGHT = 0.5f;
    constexpr float PLAYER_RADIUS = 0.15f;
    constexpr float PLAYER_HEIGHT = 1.8f;

    Vec3 RotateAroundAxis(const Vec3& v, const Vec3& axis, float angle)
    {
        Vec3 n = axis.normalized();
        float c = cosf(angle);
        float s = sinf(angle);
        return v * c + n.cross(v) * s + n * (n.dot(v) * (1.0f - c));
    }

}

Player::Player(Vec3 startPosition) : m_Position(startPosition) {
    m_Camera.fovy = 60.0f;
    m_Camera.projection = CAMERA_PERSPECTIVE;
    m_Camera.position = Vec3{ m_Position.x, m_Position.y + (BOTTOM_HEIGHT + m_HeadLerp), m_Position.z };

    UpdateCameraFPS();
}

void Player::Update() {
    Vec2 mouseDelta = GetMouseDelta();
    m_LookRotation.x -= mouseDelta.x * m_Sensitivity.x;
    m_LookRotation.y += mouseDelta.y * m_Sensitivity.y;

    float sideway = (float)IsKeyDown(KeyboardKey::D) - (float)IsKeyDown(KeyboardKey::A);
    float forward = (float)IsKeyDown(KeyboardKey::W) - (float)IsKeyDown(KeyboardKey::S);
    bool crouching = IsKeyDown(KeyboardKey::LeftControl);
    bool jumpPressed = IsKeyPressed(KeyboardKey::Space);

    UpdateBody(m_LookRotation.x, sideway, forward, jumpPressed, crouching);

    float delta = GetFrameTime();
    m_HeadLerp = Lerp(m_HeadLerp, crouching ? CROUCH_HEIGHT : STAND_HEIGHT, 20.0f * delta);
    m_Camera.position = Vec3{ m_Position.x, m_Position.y + (BOTTOM_HEIGHT + m_HeadLerp), m_Position.z };

    if (m_IsGrounded && ((forward != 0.0f) || (sideway != 0.0f))) {
        m_HeadTimer += delta * 3.0f;
        m_WalkLerp = Lerp(m_WalkLerp, 1.0f, 10.0f * delta);
        m_Camera.fovy = Lerp(m_Camera.fovy, 55.0f, 5.0f * delta);
    }

    else {
        m_WalkLerp = Lerp(m_WalkLerp, 0.0f, 10.0f * delta);
        m_Camera.fovy = Lerp(m_Camera.fovy, 60.0f, 5.0f * delta);
    }

    m_Lean.x = Lerp(m_Lean.x, sideway * 0.02f, 10.0f * delta);
    m_Lean.y = Lerp(m_Lean.y, forward * 0.015f, 10.0f * delta);

    UpdateCameraFPS();
}

void Player::UpdateBody(float rot, float side, float forward, bool jumpPressed, bool crouchHold)
{
    Vec2 input { side, -forward };

    float delta = GetFrameTime();

    if (!m_IsGrounded)
        m_Velocity.y -= GRAVITY * delta;

    if (m_IsGrounded && jumpPressed) {
        m_Velocity.y = JUMP_FORCE;
        m_IsGrounded = false;
    }

    Vec3 front { sinf(rot), 0.0f, cosf(rot) };
    Vec3 right { cosf(-rot), 0.0f, sinf(-rot) };

    Vec3 desiredDir {
        input.x * right.x + input.y * front.x,
        0.0f,
        input.x * right.z + input.y * front.z,
    };

    m_Direction = Lerp(m_Direction, desiredDir, CONTROL * delta);

    float decel = m_IsGrounded ? FRICTION : AIR_DRAG;
    Vec3 hvel{ m_Velocity.x * decel, 0.0f, m_Velocity.z * decel };

    if (hvel.length() < (MAX_SPEED * 0.01f))
        hvel = Vec3{ 0.0f, 0.0f, 0.0f };

    float speed = hvel.dot(m_Direction);

    float maxSpeed = crouchHold ? CROUCH_SPEED : MAX_SPEED;
    float accel = Clamp(maxSpeed - speed, 0.0f, MAX_ACCEL * delta);

    hvel.x += m_Direction.x * accel;
    hvel.z += m_Direction.z * accel;

    m_Velocity.x = hvel.x;
    m_Velocity.z = hvel.z;

    Vec3 previousPosition = m_Position;
    m_Position.x += m_Velocity.x * delta;
    m_Position.y += m_Velocity.y * delta;
    m_Position.z += m_Velocity.z * delta;

    ResolveCollisions(previousPosition);
}

void Player::ResolveCollisions(Vec3 previousPosition)
{
    m_IsGrounded = false;

    for (const Collider& collider : m_vColliders) {
        const float playerMinX = m_Position.x - PLAYER_RADIUS;
        const float playerMaxX = m_Position.x + PLAYER_RADIUS;
        const float playerMinY = m_Position.y;
        const float playerMaxY = m_Position.y + PLAYER_HEIGHT;
        const float playerMinZ = m_Position.z - PLAYER_RADIUS;
        const float playerMaxZ = m_Position.z + PLAYER_RADIUS;

        if (playerMaxX <= collider.min.x || playerMinX >= collider.max.x ||
            playerMaxY <= collider.min.y || playerMinY >= collider.max.y ||
            playerMaxZ <= collider.min.z || playerMinZ >= collider.max.z)
            continue;

        const bool wasBelow = previousPosition.y + PLAYER_HEIGHT <= collider.min.y;
        const bool wasAbove = previousPosition.y >= collider.max.y;
        const float pushX = std::min(playerMaxX - collider.min.x, collider.max.x - playerMinX);
        const float pushY = std::min(playerMaxY - collider.min.y, collider.max.y - playerMinY);
        const float pushZ = std::min(playerMaxZ - collider.min.z, collider.max.z - playerMinZ);

        if (wasBelow && m_Velocity.y >= 0.0f) {
            m_Position.y = collider.min.y - PLAYER_HEIGHT;
            m_Velocity.y = 0.0f;
        } else if (wasAbove && m_Velocity.y <= 0.0f) {
            m_Position.y = collider.max.y;
            m_Velocity.y = 0.0f;
            m_IsGrounded = true;
        } else if (pushX <= pushY && pushX <= pushZ) {
            m_Position.x += m_Position.x < collider.min.x ? -pushX : pushX;
            m_Velocity.x = 0.0f;
        } else if (pushZ <= pushY) {
            m_Position.z += m_Position.z < collider.min.z ? -pushZ : pushZ;
            m_Velocity.z = 0.0f;
        } else if (m_Position.y < collider.min.y) {
            m_Position.y = collider.min.y - PLAYER_HEIGHT;
            m_Velocity.y = 0.0f;
        } else {
            m_Position.y = collider.max.y;
            m_Velocity.y = 0.0f;
            m_IsGrounded = true;
        }
    }

    if (m_Position.y <= 0.0f) {
        m_Position.y = 0.0f;
        m_Velocity.y = 0.0f;
        m_IsGrounded = true;
    }
}

void Player::DrawDebugCollision(Color color) const
{
    DrawCubeWiresV(
        Vec3{m_Position.x, m_Position.y + PLAYER_HEIGHT * 0.5f, m_Position.z},
        Vec3{PLAYER_RADIUS * 2.0f, PLAYER_HEIGHT, PLAYER_RADIUS * 2.0f},
        color);
}

void Player::UpdateCameraFPS()
{
    const Vec3 up { 0.0f, 1.0f, 0.0f };
    const Vec3 targetOffset { 0.0f, 0.0f, -1.0f };

    Vec3 yaw = RotateAroundAxis(targetOffset, up, m_LookRotation.x);
    m_LookRotation.y = Clamp(m_LookRotation.y, -PI / 2 + 0.0001f, PI / 2 - 0.0001f);

    Vec3 right = yaw.cross(up).normalized();

    float pitchAngle = -m_LookRotation.y - m_Lean.y;
    pitchAngle = Clamp(pitchAngle, -PI / 2 + 0.0001f, PI / 2 - 0.0001f);
    Vec3 pitch = RotateAroundAxis(yaw, right, pitchAngle);

    float headSin = sinf(m_HeadTimer * PI);
    float headCos = cosf(m_HeadTimer * PI);
    const float stepRotation = 0.01f;
    m_Camera.up = RotateAroundAxis(up, pitch, headSin * stepRotation + m_Lean.x);

    const float bobSide = 0.1f;
    const float bobUp = 0.15f;
    Vec3 bobbing = right * (headSin * bobSide);
    bobbing.y = fabsf(headCos * bobUp);

    Vec3 cameraBase { m_Position.x, m_Position.y + (BOTTOM_HEIGHT + m_HeadLerp), m_Position.z };
    m_Camera.position = cameraBase + bobbing * m_WalkLerp;
    m_Camera.target = m_Camera.position + pitch;
}