// LastSignalPlayerController.cpp

#include "LastSignalPlayerController.h"
#include "LastSignalHUDWidget.h"
#include "InteractPromptWidget.h"
#include "HudWatchWidget.h"
#include "MainMenuWidget.h"
#include "LastSignalPlayerHUDComponent.h"
#include "LastSignalGameMode.h"
#include "PlayerCharacter.h"
#include "PrimaryWeapon.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

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

			// [F] 상호작용 안내 (무전기, 무기 상자, 탄약 상자, 메디킷 공통) — 코드로 만든 위젯이라 WBP 지정 없음
			if (UInteractPromptWidget *InteractPrompt = CreateWidget<UInteractPromptWidget>(this, UInteractPromptWidget::StaticClass()))
				InteractPrompt->AddToViewport(1);

			// 오른쪽 위 손목시계(시간 + 처치 수). WBP의 원래 시간/점수 글자는 숨기고 이걸로 대체
			if (UHudWatchWidget *Watch = CreateWidget<UHudWatchWidget>(this, UHudWatchWidget::StaticClass()))
			{
				Watch->AddToViewport(1); // 화면이 먼저 만들어져야 현재 값 표시가 됨
				Watch->BindHUDComponent(HUDComponent);
			}
			for (const TCHAR *OldTextName : {TEXT("Text_Time"), TEXT("Text_Score")})
			{
				if (UWidget *OldText = HUDWidgetInstance->GetWidgetFromName(OldTextName))
					OldText->SetVisibility(ESlateVisibility::Collapsed);
			}

			// 크로스헤어가 작아서 2.2배로 키움 (WBP 수정 없이). 조준 중 숨김은 PlayerTick에서
			if (UWidget *Crosshair = HUDWidgetInstance->GetWidgetFromName(TEXT("Image_Crosshair")))
			{
				Crosshair->SetRenderScale(FVector2D(2.2f));
				CrosshairWidget = Crosshair;
			}

			// 시작 목표 문구: GameMode의 레벨별 목록에서 현재 레벨 것을 찾아 표시
			// (GameMode BeginPlay에서 하면 위젯 바인딩 전이라 방송을 놓침 → 위젯 바인딩 직후인 여기서 호출)
			if (ALastSignalGameMode *GameMode = GetWorld()->GetAuthGameMode<ALastSignalGameMode>())
			{
				if (const FText *Objective = GameMode->LevelObjectives.Find(FName(UGameplayStatics::GetCurrentLevelName(this))))
					HUDComponent->SetMissionObjective(*Objective);
			}
		}
	}
}

void ALastSignalPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// 정조준(우클릭) 중에는 크로스헤어 숨김 — 조준경/가늠자로 조준하니까
	if (UWidget *Crosshair = CrosshairWidget.Get())
	{
		const APlayerCharacter *PlayerCharacter = Cast<APlayerCharacter>(GetPawn());
		const APrimaryWeapon *Weapon = PlayerCharacter ? PlayerCharacter->GetCurrentWeapon() : nullptr;
		const bool bAiming = Weapon && Weapon->IsAiming();
		Crosshair->SetRenderOpacity(bAiming ? 0.0f : 1.0f);
	}
}

void ALastSignalPlayerController::TryTriggerSpecialAttack()
{
	if (HUDComponent)
	{
		HUDComponent->TryActivateSpecialAttack();
	}
}

void ALastSignalPlayerController::TogglePauseMenu()
{
    const bool bNewPaused = !GetWorld()->IsPaused(); // 엔진이 이미 갖고있는 pause 상태를 그대로 기준으로 삼음 (별도 bool 안 만듦)
    UGameplayStatics::SetGamePaused(GetWorld(), bNewPaused);

    if (bNewPaused)
    {
        // 코드로 만든 일시정지 화면(UPauseMenuWidget)을 쓴다. PauseMenuClass(WBP_Pause)는 더 이상 사용 안 함
        PauseMenuWidgetInstance = CreateWidget<UPauseMenuWidget>(this, UPauseMenuWidget::StaticClass());

        FInputModeUIOnly InputMode;
        if (PauseMenuWidgetInstance)
        {
            PauseMenuWidgetInstance->AddToViewport(20);
            InputMode.SetWidgetToFocus(PauseMenuWidgetInstance->TakeWidget()); // P/ESC로 계속하기를 받으려면 포커스 필요
        }
        SetInputMode(InputMode);
        bShowMouseCursor = true;
    }
    else
    {
        if (PauseMenuWidgetInstance)
        {
            PauseMenuWidgetInstance->RemoveFromParent();
            PauseMenuWidgetInstance = nullptr;
        }

        SetInputMode(FInputModeGameOnly());
        bShowMouseCursor = false;
    }
}
