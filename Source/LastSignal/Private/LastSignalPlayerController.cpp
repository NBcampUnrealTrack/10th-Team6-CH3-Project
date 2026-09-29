// LastSignalPlayerController.cpp

#include "LastSignalPlayerController.h"
#include "LastSignalHUDWidget.h"
#include "InteractPromptWidget.h"
#include "HudWatchWidget.h"
#include "HudHealthWidget.h"
#include "HudSkillWidget.h"
#include "MainMenuWidget.h"
#include "LastSignalPlayerHUDComponent.h"
#include "LastSignalGameMode.h"
#include "LastSignalGameInstance.h"
#include "PlayerCharacter.h"
#include "PrimaryWeapon.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Sound/SoundBase.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

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
				InteractPrompt->AddToViewport(0); // HUD와 같은 층 → 나중에 뜨는 인트로/엔딩 슬라이드쇼가 위를 덮음

			// 오른쪽 위 손목시계(시간 + 처치 수). WBP의 원래 시간/점수 글자는 숨기고 이걸로 대체
			if (UHudWatchWidget *Watch = CreateWidget<UHudWatchWidget>(this, UHudWatchWidget::StaticClass()))
			{
				Watch->AddToViewport(0); // HUD와 같은 층 (슬라이드쇼가 덮게). 화면이 먼저 만들어져야 현재 값 표시가 됨
				Watch->BindHUDComponent(HUDComponent);
				WatchWidget = Watch;
			}

			// 왼쪽 아래 체력 (군인 그림이 체력만큼 차오름). WBP의 원래 HP바는 숨기고 이걸로 대체
			if (UHudHealthWidget *Health = CreateWidget<UHudHealthWidget>(this, UHudHealthWidget::StaticClass()))
			{
				Health->AddToViewport(0);
				Health->BindHUDComponent(HUDComponent);
				HealthWidget = Health;
			}
			// 스페셜 스킬 게이지: 원래 배너(전체가 서서히 밝아짐 + PRESS)를 같은 자리에서 왼쪽→오른쪽 차오르는 게이지로 교체
			if (UHudSkillWidget *Skill = CreateWidget<UHudSkillWidget>(this, UHudSkillWidget::StaticClass()))
			{
				if (Skill->TakeOverFromHUD(HUDWidgetInstance))
				{
					Skill->AddToViewport(0);
					SkillWidget = Skill;
				}
			}

			for (const TCHAR *OldTextName : {TEXT("Text_Time"), TEXT("Text_Score"), TEXT("ProgressBar_HP")})
			{
				if (UWidget *OldText = HUDWidgetInstance->GetWidgetFromName(OldTextName))
					OldText->SetVisibility(ESlateVisibility::Collapsed);
			}

			// 오른쪽 아래 탄약 숫자도 체력과 같은 폰트(Black Ops One, 메인메뉴 제목 폰트)로. 크기는 WBP 그대로
			if (UTextBlock *AmmoText = Cast<UTextBlock>(HUDWidgetInstance->GetWidgetFromName(TEXT("Text_AmmoCount"))))
			{
				if (UFont *BlackOpsOne = LoadObject<UFont>(nullptr, TEXT("/Game/UI/Fonts/BlackOpsOne_Font.BlackOpsOne_Font")))
				{
					FSlateFontInfo Font = AmmoText->GetFont();
					Font.FontObject = BlackOpsOne;
					Font.TypefaceFontName = NAME_None;
					AmmoText->SetFont(Font);
				}
			}

			// 크로스헤어가 작아서 2.2배로 키움 (WBP 수정 없이). 조준 중 숨김은 PlayerTick에서
			if (UWidget *Crosshair = HUDWidgetInstance->GetWidgetFromName(TEXT("Image_Crosshair")))
			{
				Crosshair->SetRenderScale(FVector2D(2.2f));
				CrosshairWidget = Crosshair;
			}

			ApplyHudScale();

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

	// 슬라이드쇼/페이드인 중엔 시계, 체력 숨김 (Hidden으로 하면 위젯 Tick이 멈춰서 투명도로)
	const bool bSlideshowOnScreen = IsStorySlideshowOnScreen();
	for (UWidget *CodeHudWidget : {WatchWidget.Get(), HealthWidget.Get(), SkillWidget.Get()})
	{
		if (CodeHudWidget)
			CodeHudWidget->SetRenderOpacity(IsInCutscene() ? 0.0f : 1.0f);
	}

	// 인트로가 끝난 순간: 검은 화면에서 2초 동안 밝아짐. 그동안은 조작 불가, HUD도 숨긴 채로
	if (bStorySlideshowWasOnScreen && !bSlideshowOnScreen && UGameplayStatics::GetCurrentLevelName(this) == TEXT("L_SafeZone_Spawn"))
	{
		constexpr float FadeSeconds = 2.0f;
		if (PlayerCameraManager)
			PlayerCameraManager->StartCameraFade(1.0f, 0.0f, FadeSeconds, FLinearColor::Black, false, /*bHoldWhenFinished*/ false);
		if (APawn *ControlledPawn = GetPawn())
			ControlledPawn->DisableInput(this); // 이동/시점/사격 막음 (ESC 일시정지는 컨트롤러 입력이라 그대로)
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
		GetWorldTimerManager().SetTimer(IntroFadeTimerHandle, this, &ALastSignalPlayerController::FinishIntroFade, FadeSeconds, false);
	}
	bStorySlideshowWasOnScreen = bSlideshowOnScreen;

	UpdateFootsteps(DeltaTime);
}

void ALastSignalPlayerController::ApplyHudScale()
{
	// HUD가 크다는 피드백 → 전체를 같은 비율로 줄임 (이 숫자 하나로 조절, 1.0 = 원래 크기)
	constexpr float HudScale = 0.8f;
	const FVector2D Scale(HudScale);

	// 코드로 만든 위젯은 화면 전체 크기라 위젯째로, 붙어 있는 모서리를 기준점으로 줄임
	if (UWidget *Watch = WatchWidget.Get())
	{
		Watch->SetRenderTransformPivot(FVector2D(1.0f, 0.0f)); // 오른쪽 위
		Watch->SetRenderScale(Scale);
	}
	if (UWidget *Health = HealthWidget.Get())
	{
		Health->SetRenderTransformPivot(FVector2D(0.0f, 1.0f)); // 왼쪽 아래
		Health->SetRenderScale(Scale);
	}
	if (UHudSkillWidget *Skill = Cast<UHudSkillWidget>(SkillWidget.Get()))
	{
		Skill->SetRenderTransformPivot(Skill->GetScreenAnchor());
		Skill->SetRenderScale(Scale);
	}

	// WBP_LastSignalHUD 안의 요소(탄약 숫자, 목표 문구 등)는 하나씩, 각자 붙은 모서리 기준으로
	// 화면 가운데(크로스헤어, 히트마커)와 화면 전체를 덮는 것(아드레날린 효과)은 그대로
	UCanvasPanel *Root = HUDWidgetInstance ? Cast<UCanvasPanel>(HUDWidgetInstance->GetRootWidget()) : nullptr;
	if (!Root)
		return;
	for (UWidget *Child : Root->GetAllChildren())
	{
		const UCanvasPanelSlot *ChildSlot = Cast<UCanvasPanelSlot>(Child->Slot);
		if (!ChildSlot)
			continue;
		const FAnchors Anchors = ChildSlot->GetAnchors();
		if (Anchors.IsStretchedHorizontal() || Anchors.IsStretchedVertical() || Anchors.Minimum.Equals(FVector2D(0.5f, 0.5f)))
			continue;
		Child->SetRenderTransformPivot(Anchors.Minimum);
		Child->SetRenderScale(Scale);
	}
}

void ALastSignalPlayerController::UpdateFootsteps(float DeltaTime)
{
	const ACharacter *PlayerCharacter = Cast<ACharacter>(GetPawn());
	const UCharacterMovementComponent *Movement = PlayerCharacter ? PlayerCharacter->GetCharacterMovement() : nullptr;
	const float Speed = Movement ? Movement->Velocity.Size2D() : 0.0f;
	if (!Movement || !Movement->IsMovingOnGround() || Speed < 50.0f)
	{
		FootstepDistance = 120.0f; // 멈췄다 다시 걸으면 첫 발이 금방 나오게
		return;
	}

	// 걸음 폭: 걷기(600) 약 2.2m, 달리기(900) 약 2.6m
	const float Stride = 150.0f + Speed * 0.12f;
	FootstepDistance += Speed * DeltaTime;
	if (FootstepDistance < Stride)
		return;
	FootstepDistance -= Stride;

	// 콘크리트 부츠 소리 (큐 안에 여러 버전이 있어 매번 조금씩 다르게 남). 달리면 달리기 소리
	const bool bRunning = Speed > 750.0f;
	if (USoundBase *Step = LoadObject<USoundBase>(nullptr, bRunning
		? TEXT("/Game/Essential_Foosteps_SK/CUE/Concrete/Footstep_Concrete_Boots_Run_8_Cue.Footstep_Concrete_Boots_Run_8_Cue")
		: TEXT("/Game/Essential_Foosteps_SK/CUE/Concrete/Footstep_Concrete_Boots_Jog_8_Cue.Footstep_Concrete_Boots_Jog_8_Cue")))
	{
		UGameplayStatics::PlaySound2D(this, Step, 0.35f); // 내 발소리라 위치 없이 작게
	}
}

void ALastSignalPlayerController::FinishIntroFade()
{
	// 다 밝아짐 → 여기서부터 게임 시작: HUD 표시, 조작 가능, 시간 0부터
	// (레벨 BP가 인트로 시작 때 HUD를 숨기고 다시 안 켜서, 여기서 켬)
	if (HUDWidgetInstance)
		HUDWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (APawn *ControlledPawn = GetPawn())
		ControlledPawn->EnableInput(this);
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());

	if (ALastSignalGameMode *GameMode = GetWorld()->GetAuthGameMode<ALastSignalGameMode>())
		GameMode->StartStopwatch();
	if (ULastSignalGameInstance *CurrentGameInstance = GetGameInstance<ULastSignalGameInstance>())
		CurrentGameInstance->GameStartRealSeconds = FPlatformTime::Seconds(); // 게임오버/크레딧의 플레이 시간도 인트로 제외
}

bool ALastSignalPlayerController::IsInCutscene() const
{
	return IsStorySlideshowOnScreen() || GetWorldTimerManager().IsTimerActive(IntroFadeTimerHandle);
}

bool ALastSignalPlayerController::IsStorySlideshowOnScreen() const
{
	// 레벨 BP/게임모드 BP가 만든 위젯이라 C++에서 클래스를 모름 → 경로로 찾음 (아직 안 불러와졌으면 = 화면에 없음)
	const TSubclassOf<UUserWidget> SlideshowClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/HUD/WBP_StorySlideshow.WBP_StorySlideshow_C"))).Get();
	if (!SlideshowClass)
		return false;

	TArray<UUserWidget *> Slideshows;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(const_cast<ALastSignalPlayerController *>(this), Slideshows, SlideshowClass, /*TopLevelOnly = 화면에 붙은 것만*/ true);
	return Slideshows.Num() > 0;
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
