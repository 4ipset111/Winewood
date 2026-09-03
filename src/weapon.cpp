#include "weapon.h"

#include "resources.h"

#include <algorithm>

using namespace qc;

bool TokarevWeapon::Initialize()
{
    for (int soundIndex = 0; soundIndex < static_cast<int>(m_aShootSounds.size()); ++soundIndex) {
        const std::string name = TextFormat("tokarev_shoot_%d", soundIndex + 1);
        const std::string path = TextFormat(
            "resources/sounds/Tokarev_Shoot_%d.wav", soundIndex + 1);
        gs_Resources.Load<Sound>(name, path);
        m_aShootSounds[soundIndex] = gs_Resources.Get<Sound>(name);
    }

    return true;
}

std::optional<WeaponShot> TokarevWeapon::Update(const Camera3D& camera, float delta)
{
    UpdateSoundPlayback(delta);

    if (IsMouseButtonPressed(MouseButton::Left) && !m_ShootPending) {
        m_ShootPending = true;
        m_ShootDelay = 0.10f;
    }

    m_ShootDelay -= delta;
    if (!m_ShootPending || m_ShootDelay > 0.0f)
        return std::nullopt;

    m_ShootPending = false;
    PlayShotSound();

    return WeaponShot{
        Ray{camera.position, (camera.target - camera.position).normalized()},
        5.0f * DEG2RAD};
}

void TokarevWeapon::UpdateSoundPlayback(float delta)
{
    if (m_CurrentShootSound.stream.buffer) {
        m_CurrentShootElapsed += delta;
        constexpr float fadeDuration = 0.12f;
        const float fadeStart = std::max(0.0f, m_CurrentShootDuration - fadeDuration);
        if (m_CurrentShootElapsed >= fadeStart && m_CurrentShootDuration > 0.0f) {
            const float fadeProgress = (m_CurrentShootElapsed - fadeStart) / fadeDuration;
            m_CurrentShootVolume = 2.0f * (1.0f - Clamp(fadeProgress, 0.0f, 1.0f));
            SetSoundVolume(m_CurrentShootSound, m_CurrentShootVolume);
        }
    }

    if (m_FadingShootSound.stream.buffer && m_FadingShootVolume > 0.0f) {
        m_FadingShootVolume = std::max(0.0f, m_FadingShootVolume - delta / 0.12f);
        SetSoundVolume(m_FadingShootSound, m_FadingShootVolume);
        if (m_FadingShootVolume == 0.0f)
            StopSound(m_FadingShootSound);
    }
}

void TokarevWeapon::PlayShotSound()
{
    if (m_FadingShootSound.stream.buffer)
        StopSound(m_FadingShootSound);
    m_FadingShootSound = m_CurrentShootSound;
    m_FadingShootVolume = m_CurrentShootVolume;

    m_CurrentShootSound = m_aShootSounds[m_ShootSoundIndex];
    m_ShootSoundIndex = (m_ShootSoundIndex + 1) % static_cast<int>(m_aShootSounds.size());
    m_CurrentShootVolume = 2.0f;
    m_CurrentShootElapsed = 0.0f;
    m_CurrentShootDuration = m_CurrentShootSound.stream.sampleRate > 0
        ? static_cast<float>(m_CurrentShootSound.frameCount)
            / static_cast<float>(m_CurrentShootSound.stream.sampleRate)
        : 0.0f;
    SetSoundVolume(m_CurrentShootSound, m_CurrentShootVolume);
    SetSoundPan(m_CurrentShootSound, 0.0f);
    PlaySound(m_CurrentShootSound);
}

void TokarevWeapon::Shutdown()
{
    if (m_CurrentShootSound.stream.buffer)
        StopSound(m_CurrentShootSound);
    if (m_FadingShootSound.stream.buffer)
        StopSound(m_FadingShootSound);
}