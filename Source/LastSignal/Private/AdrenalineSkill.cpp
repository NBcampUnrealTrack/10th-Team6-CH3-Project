#include "AdrenalineSkill.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerCharacter.h"
#include "PrimaryWeapon.h"
#include "WeaponCombatComponent.h"

UAdrenalineSkill::UAdrenalineSkill()
{
    MaxSkillValue = 100.0f;       // 최대 스킬 게이지 100
    SkillDuration = 20.0f;       // 지속 시간
    ChargeRatePerSecond = 1.0f;  // 초당 패시브 충전량
    KillBonusValue = 5.0f;      // 킬카운트 1당 충전량

    ActiveNiagaraEffect = nullptr;
    AdrenalineVFX = nullptr;
    ActivationSound = nullptr;
}

void UAdrenalineSkill::ActivateSkill()
{
    Super::ActivateSkill();

    AActor *Owner = GetOwner();
    APlayerCharacter *Character = Cast<APlayerCharacter>(Owner);
    if (!Character)
        return;

    // 1. [최초 1회 실행] 발동 사운드 재생 (무기 교체 시에는 재재생되지 않음)
    if (ActivationSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ActivationSound, Character->GetActorLocation());
    }

    // 2. [최초 1회 실행] 나이아가라 이펙트를 캐릭터 메시에 부착 (스킬 지속시간 내내 유지)
    if (AdrenalineVFX && !ActiveNiagaraEffect)
    {
        ActiveNiagaraEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
            AdrenalineVFX,
            Character->GetMesh(),
            TEXT("hand_rSocket"), // 이펙트를 붙일 캐릭터 소켓 이름
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true);
    }

    // 3. 현재 들고 있는 무기에 스탯 버프 적용
    if (Character->GetCurrentWeapon())
    {
        ApplyBuffToWeapon(Character->GetCurrentWeapon());
    }
}

void UAdrenalineSkill::DeactivateSkill()
{
    // 스킬 종료 시에만 이펙트 제거
    if (ActiveNiagaraEffect)
    {
        ActiveNiagaraEffect->DestroyComponent();
        ActiveNiagaraEffect = nullptr;
    }

    // 백업한 무기 스탯 원복
    RestoreWeaponStats();

    Super::DeactivateSkill();
}

void UAdrenalineSkill::OnWeaponSwapped(APrimaryWeapon *NewWeapon)
{
    // 스킬이 활성화 상태가 아니면 동작 안 함
    if (CurrentState != ESkillState::Active)
        return;

    // 사운드는 재생하지 않고 스탯 처리만 진행
    RestoreWeaponStats();
    ApplyBuffToWeapon(NewWeapon);
}

void UAdrenalineSkill::ApplyBuffToWeapon(APrimaryWeapon *Weapon)
{
    if (!Weapon)
        return;

    UWeaponCombatComponent *Combat = Weapon->GetCombatComponent();
    if (Combat)
    {
        OriginalWeaponStats = Combat->Stats;
        CurrentBuffedWeapon = Weapon;

        Combat->Stats.PitchKick *= 0.5f; // 수직 반동 설정
        Combat->Stats.YawKick *= 0.5f;   // 수평 반동 설정
        Combat->Stats.VisualKickScale *= 0.5f; // 카메라/화면이 덜덜 흔들리는 시각적 반동 설정
        Combat->Stats.RPM = 900.0f; // RPM 900 설정
        Combat->Stats.ReloadTime = 1.5f; // 일반 재장전 속도 설정
        // Combat->Stats.Weight *= 0.5f;               // [미개발] 무기 무게가 낮을수록 캐릭터 속도증가 시스템
        // Combat->>Stats.AutoFireSpread *= 0.6f;      // [미개발] 연사 가중치
        // Combat->Stats.TacReloadTime = 1.0f;         // [미개발] 전술 재장전 시간
    }
}

void UAdrenalineSkill::RestoreWeaponStats()
{
    if (CurrentBuffedWeapon.IsValid())
    {
        APrimaryWeapon *Weapon = CurrentBuffedWeapon.Get();
        if (Weapon && Weapon->GetCombatComponent())
        {
            Weapon->GetCombatComponent()->Stats = OriginalWeaponStats;
        }
        CurrentBuffedWeapon.Reset();
    }
}