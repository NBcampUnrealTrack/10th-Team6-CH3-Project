#include "PrimaryFirearms.h"

#include "WeaponCombatComponent.h"

AM4A1::AM4A1()
{
    // 아래 수치는 게임 밸런스 조절을 위한 초기 예시다.
    FWeaponStats &S = Combat->Stats;

    S.FireMode = EWeaponFireMode::Automatic;
    S.AttackType = EWeaponAttackType::Single;
    S.ReloadType = EWeaponReloadType::Magazine;

    S.Damage = 25.0f;
    S.RPM = 750.0f;
    S.Range = 10000.0f;

    S.MagazineSize = 30;
    S.InitialReserveAmmo = 120;
    S.ReloadTime = 2.5f;

    S.PitchKick = FVector2D(-1.2f, -1.0f);
    S.YawKick = FVector2D(-0.4f, 0.4f);

    S.CameraKickSpeed = 25.0f;
    S.CameraReturnSpeed = 8.0f;

    S.VisualKickScale = 1.0f;
    S.VisualKickSpeed = 25.0f;
    S.VisualReturnSpeed = 12.0f;

    S.BaseSpreadAngle = 0.4f;
    S.MaxSpreadAngle = 3.5f;
    S.SpreadIncreasePerShot = 0.3f;
    S.SpreadRecoverySpeed = 9.0f;
}

ASR25::ASR25()
{
    FWeaponStats &S = Combat->Stats;

    S.FireMode = EWeaponFireMode::SemiAuto;
    S.AttackType = EWeaponAttackType::Single;
    S.ReloadType = EWeaponReloadType::Magazine;

    S.Damage = 55.0f;
    S.RPM = 300.0f;
    S.Range = 20000.0f;

    S.MagazineSize = 20;
    S.InitialReserveAmmo = 80;
    S.ReloadTime = 3.0f;

    S.PitchKick = FVector2D(-2.2f, -1.8f);
    S.YawKick = FVector2D(-0.6f, 0.6f);

    S.CameraKickSpeed = 28.0f;
    S.CameraReturnSpeed = 7.0f;

    S.VisualKickScale = 1.5f;
    S.VisualKickSpeed = 25.0f;
    S.VisualReturnSpeed = 10.0f;

    S.BaseSpreadAngle = 0.05f;
    S.MaxSpreadAngle = 1.8f;
    S.SpreadIncreasePerShot = 0.5f;
    S.SpreadRecoverySpeed = 14.0f;
}

AMP153::AMP153()
{
    FWeaponStats &S = Combat->Stats;

    S.FireMode = EWeaponFireMode::SemiAuto;
    S.AttackType = EWeaponAttackType::Shotgun;
    S.ReloadType = EWeaponReloadType::PerShell;

    // 산탄 명중·데미지 계산은 추후 구현한다.
    S.Damage = 10.0f;
    S.PelletCount = 9;

    S.RPM = 180.0f;
    S.Range = 4000.0f;

    S.MagazineSize = 6;
    S.InitialReserveAmmo = 36;

    // 개별 장전에서는 한 발을 넣는 시간이다.
    S.ReloadTime = 0.65f;

    S.PitchKick = FVector2D(-3.0f, -2.5f);
    S.YawKick = FVector2D(-0.8f, 0.8f);

    S.CameraKickSpeed = 30.0f;
    S.CameraReturnSpeed = 6.0f;

    S.VisualKickScale = 2.0f;
    S.VisualKickSpeed = 28.0f;
    S.VisualReturnSpeed = 9.0f;
    
    S.BaseSpreadAngle = 3.0f;
    S.MaxSpreadAngle = 5.0f;
    S.SpreadIncreasePerShot = 0.8f;
    S.SpreadRecoverySpeed = 6.0f;
}