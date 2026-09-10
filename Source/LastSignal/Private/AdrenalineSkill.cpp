#include "AdrenalineSkill.h"
#include "NiagaraComponent.h"
// #include "Character.h" 캐릭터.h 임의로 넣어둠
// #include "GunBase.h" 총.h 임의로 넣어둠

UAdrenalineSkill::UAdrenalineSkill()
{
    MaxSkillValue = 100.0f;       // 최대 스킬 게이지 100
    SkillDuration = 20.0f;       // 지속 시간
    ChargeRatePerSecond = 1.0f;  // 초당 패시브 충전량
    KillBonusValue = 5.0f;      // 킬카운트 1당 충전량

    ActiveNiagaraEffect = nullptr;
}

void UAdrenalineSkill::ActivateSkill()
{
    Super::ActivateSkill();

    // 실제 캐릭터와 무기 포인터를 가져와 스탯 적용
    /*
    AActor* Owner = GetOwner();
    ALastSignalCharacter* Character = Cast<ALastSignalCharacter>(Owner);
    if (Character && Character->GetCurrentWeapon())
    {
        AGunBase* Weapon = Character->GetCurrentWeapon();

        // 기존 무기 스탯 저장
        OriginalVerticalRecoil = Weapon->VerticalRecoil;
        OriginalHorizonRecoil = Weapon->HorizonRecoil;
        OriginalRecoilCameraShake = Weapon->RecoilCameraShake;
        OriginalWeight = Weapon->Weight;
        OriginalRPM = Weapon->RPM;
        OriginalAutoFireSpread = Weapon->AutoFireSpread;
        OriginalReloadTime = Weapon->ReloadTime;
        OriginalTacReloadTime = Weapon->TacReloadTime;

        // 아드레날린 스탯
        Weapon->VerticalRecoil *= 0.5f;       // verticalRecoil (0.5배)
        Weapon->HorizonRecoil *= 0.5f;        // horizonRecoil (0.5배)
        Weapon->RecoilCameraShake *= 0.5f;    // RecoilCameraShake (0.5배)
        Weapon->Weight *= 0.5f;               // weight (0.5배)
        Weapon->RPM = 900;                    // rpm 900 고정
        Weapon->AutoFireSpread *= 0.6f;       // AutofireSpread (0.6배)
        Weapon->ReloadTime = 1.5f;            // reloadtime 1.5초 (배수 연산시 /= 1.5f)
        Weapon->TacReloadTime = 1.0f;         // tacReloadtime 1.0초 (배수 연산시 /= 1.5f)

        // 캐릭터 이동속도 재계산 로직을 호출
        // Character->UpdateCharacterMovementSpeed();
    }
    */
}

void UAdrenalineSkill::DeactivateSkill()
{
    // 이펙트 제거
    if (ActiveNiagaraEffect)
    {
        ActiveNiagaraEffect->DestroyComponent();
        ActiveNiagaraEffect = nullptr;
    }

    // 백업한 기존 무기 스탯 북구
    /*
    AActor* Owner = GetOwner();
    ALastSignalCharacter* Character = Cast<ALastSignalCharacter>(Owner);
    if (Character && Character->GetCurrentWeapon())
    {
        AGunBase* Weapon = Character->GetCurrentWeapon();

        Weapon->VerticalRecoil = OriginalVerticalRecoil;
        Weapon->HorizonRecoil = OriginalHorizonRecoil;
        Weapon->RecoilCameraShake = OriginalRecoilCameraShake;
        Weapon->Weight = OriginalWeight;
        Weapon->RPM = OriginalRPM;
        Weapon->AutoFireSpread = OriginalAutoFireSpread;
        Weapon->ReloadTime = OriginalReloadTime;
        Weapon->TacReloadTime = OriginalTacReloadTime;

        // 캐릭터 이동속도 재계산 로직을 호출
        // Character->UpdateCharacterMovementSpeed();
    }
    */

    Super::DeactivateSkill();
}