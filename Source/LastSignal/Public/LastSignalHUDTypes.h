// LastSignalHUDTypes.h
// LAST SIGNAL - HUD 시스템에서 공용으로 사용하는 열거형 / 구조체 정의

#pragma once

#include "CoreMinimal.h"
#include "LastSignalHUDTypes.generated.h"

/**
 * 화면 우상단 타이머 표시 모드
 * - Stopwatch : [안전지대:스폰] ~ [구역1] 구간, 흰색으로 경과시간 증가
 * - Countdown : [안전지대:재정비]에서 라디오 상호작용 후, 빨간색으로 남은시간 감소
 */
UENUM(BlueprintType)
enum class ELastSignalTimerMode : uint8
{
	Stopwatch UMETA(DisplayName = "Stopwatch (경과시간, 흰색)"),
	Countdown UMETA(DisplayName = "Countdown (남은시간, 빨간색)")
};

/**
 * 좀비 피격 데미지 단계에 따른 모션 상태 (UI/UX 컨셉 - 데미지량 표현)
 * 100~51% : Normal, 50~1% : Staggered
 * AnimBP에서 이 값을 참조해 비틀거리는 모션을 재생한다.
 */
UENUM(BlueprintType)
enum class EZombieDamageState : uint8
{
	Normal UMETA(DisplayName = "100~51% : 기본 모션"),
	Staggered UMETA(DisplayName = "50~1% : 비틀거리는 모션")
};

/** 하단 중앙 / 우하단에 표시되는 무기 정보 */
USTRUCT(BlueprintType)
struct FLastSignalWeaponHUDData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FText WeaponName = FText::FromString(TEXT("Five-Seven"));

	// 무기 아이콘 (하단 중앙 총기 표시)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSoftObjectPtr<class UTexture2D> WeaponIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 CurrentAmmo = 7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 MagazineSize = 7;
};
