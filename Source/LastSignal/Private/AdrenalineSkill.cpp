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
    SkillDuration = 10.0f;       // 지속 시간
    ChargeRatePerSecond = 1.0f;  // 초당 패시브 충전량
    KillBonusValue = 10.0f;      // 킬카운트 1당 충전량

    ActiveNiagaraEffect = nullptr;
    AdrenalineVFX = nullptr;
    ActivationSound = nullptr;
    AttachSocketName = TEXT("hand_rSocket");
}

void UAdrenalineSkill::ActivateSkill()
{
    if (!CanActivateSkill())
    {
        Super::ActivateSkill();
        return;
    }

    Super::ActivateSkill();

    AActor *Owner = GetOwner();
    APlayerCharacter *Character = Cast<APlayerCharacter>(Owner);
    if (!Character)
    {
        return;
    }
    // 발동 사운드 재생
    if (ActivationSound)
    {
        UGameplayStatics::SpawnSoundAttached(ActivationSound, Character->GetRootComponent());
    }

    // 나이아가라 이펙트를 캐릭터 메시에 부착
    if (AdrenalineVFX && !ActiveNiagaraEffect)
    {
        USkeletalMeshComponent *CharacterMesh = Character->GetMesh();
        if (CharacterMesh)
        {
            FName TargetSocket = CharacterMesh->DoesSocketExist(AttachSocketName) ? AttachSocketName : NAME_None;

            ActiveNiagaraEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
                AdrenalineVFX,
                CharacterMesh,
                AttachSocketName, // 소켓 이름
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                EAttachLocation::SnapToTargetIncludingScale,
                true);
        }
    }

        // 현재 들고 있는 무기에 스탯 버프 적용
    if (Character->GetCurrentWeapon())
    {
            ApplyBuffToWeapon(Character->GetCurrentWeapon());
    }
}

void UAdrenalineSkill::DeactivateSkill()
{
    if (DeactivationSound)
    {
            if (AActor *Owner = GetOwner())
            {
            UGameplayStatics::SpawnSoundAttached(DeactivationSound, Owner->GetRootComponent());
            }
    }

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
        Combat->Stats.TacReloadTime = 1.0f;         // [미개발] 전술 재장전 시간
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