#ifndef WEAPON_H
#define WEAPON_H

#include "QuarkCore/QuarkCore.hpp"

#include <array>
#include <optional>

struct WeaponShot {
    qc::Ray ray{};
    float recoilPitch = 0.0f;
};

class IWeapon {
public:
    virtual ~IWeapon() = default;

    virtual bool Initialize() = 0;
    virtual std::optional<WeaponShot> Update(const qc::Camera3D& camera, float delta) = 0;
    virtual void Shutdown() = 0;
};

class TokarevWeapon final : public IWeapon {
public:
    bool Initialize() override;
    std::optional<WeaponShot> Update(const qc::Camera3D& camera, float delta) override;
    void Shutdown() override;

private:
    void UpdateSoundPlayback(float delta);
    void PlayShotSound();

    std::array<qc::Sound, 4> m_aShootSounds{};
    int m_ShootSoundIndex = 0;
    qc::Sound m_CurrentShootSound{};
    float m_CurrentShootVolume = 0.0f;
    float m_CurrentShootElapsed = 0.0f;
    float m_CurrentShootDuration = 0.0f;
    qc::Sound m_FadingShootSound{};
    float m_FadingShootVolume = 0.0f;
    bool m_ShootPending = false;
    float m_ShootDelay = 0.0f;
};

#endif // WEAPON_H