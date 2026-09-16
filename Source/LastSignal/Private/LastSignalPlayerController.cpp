// LastSignalPlayerController.cpp

#include "LastSignalPlayerController.h"
#include "LastSignalHUDWidget.h"
#include "LastSignalPlayerHUDComponent.h"
#include "Blueprint/UserWidget.h"

ALastSignalPlayerController::ALastSignalPlayerController()
{
	HUDComponent = CreateDefaultSubobject<ULastSignalPlayerHUDComponent>(TEXT("HUDComponent"));
}

void ALastSignalPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 로컬 플레이어만 자기 화면에 HUD를 그린다 (스플릿스크린/멀티플레이 대비)
	if (IsLocalController() && HUDWidgetClass)
	{
		HUDWidgetInstance = CreateWidget<ULastSignalHUDWidget>(this, HUDWidgetClass);
		if (HUDWidgetInstance)
		{
			HUDWidgetInstance->BindHUDComponent(HUDComponent);
			HUDWidgetInstance->AddToViewport();

			// 초기 미션 목표 예시 (실제로는 스테이지 매니저/게임모드에서 호출)
			HUDComponent->SetMissionObjective(FText::FromString(TEXT("구조 신호를 따라 지하철로 이동하라")));
		}
	}
}

void ALastSignalPlayerController::TryTriggerSpecialAttack()
{
	if (HUDComponent)
	{
		HUDComponent->TryActivateSpecialAttack();
	}
}
