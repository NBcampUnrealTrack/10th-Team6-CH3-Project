#include "AdrenalineSkill.h"
#include "NiagaraComponent.h"
// #include "Character.h" 캐릭터
// #include "GunBase.h" 총

UAdrenalineSkill::UAdrenalineSkill()
{
	MaxSkillValue = 100.0f; //스킬 게이지
	ChargeRatePerSecond = 1.0f; //초 당 게이지 충전 양
	KillBonusValue = 5.0f; // 처치시 보너스 충전양
	SkillDuration = 20.0f; // 지속시간 

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

        // 1. 기존 무기 스탯 백업
        OriginalVerticalRecoil = Weapon->VerticalRecoil;
        OriginalHorizonRecoil = Weapon->HorizonRecoil;
        OriginalRecoilCameraShake = Weapon->RecoilCameraShake;
        OriginalWeight = Weapon->Weight;
        OriginalRPM = Weapon->RPM;
        OriginalAutoFireSpread = Weapon->AutoFireSpread;
        OriginalReloadTime = Weapon->ReloadTime;
        OriginalTacReloadTime = Weapon->TacReloadTime;

        // 2. 요구받은 아드레날린 스탯 적용
        Weapon->VerticalRecoil *= 0.5f;       // verticalRecoil (0.5배)
        Weapon->HorizonRecoil *= 0.5f;        // horizonRecoil (0.5배)
        Weapon->RecoilCameraShake *= 0.5f;    // RecoilCameraShake (0.5배)
        Weapon->Weight *= 0.5f;               // weight (0.5배)
        Weapon->RPM = 900;                    // rpm 900 고정
        Weapon->AutoFireSpread *= 0.6f;       // AutofireSpread (0.6배)
        Weapon->ReloadTime = 1.5f;            // reloadtime 1.5초 (배수 연산시 /= 1.5f)
        Weapon->TacReloadTime = 1.0f;         // tacReloadtime 1.0초 (배수 연산시 /= 1.5f)

        // 캐릭터 이동속도 재계산 로직이 있다면 호출
        // Character->UpdateCharacterMovementSpeed();
    }
    */
}

void UAdrenalineSkill::DeactivateSkill()
{
	if (ActiveNiagaraEffect)
	{
		ActiveNiagaraEffect->DestroyComponent();
		ActiveNiagaraEffect = nullptr;
	}

    // 백업해둔 기존 무기 스탯 원복
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

        // 캐릭터 이동속도 복구 로직이 있다면 호출
        // Character->UpdateCharacterMovementSpeed();
    }
    */

	Super::DeactivateSkill();
}