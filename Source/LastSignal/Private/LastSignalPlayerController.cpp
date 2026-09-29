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
#include "Blueprint/WidgetTree.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "AudioDevice.h"
#include "Sound/SoundWave.h"

namespace
{
	// ESC 전용: 포커스와 상관없이 모든 키 입력을 먼저 봄 (인트로 중엔 UI 전용 입력이라 게임 입력이 안 들어오기 때문)
	class FIntroSkipInputProcessor : public IInputProcessor
	{
	public:
		TFunction<bool()> OnEscape; // true면 ESC를 여기서 먹음 (다른 위젯에 안 넘어감)
		virtual void Tick(const float DeltaTime, FSlateApplication &SlateApp, TSharedRef<ICursor> Cursor) override {}
		virtual bool HandleKeyDownEvent(FSlateApplication &SlateApp, const FKeyEvent &InKeyEvent) override
		{
			if (InKeyEvent.GetKey() == EKeys::Escape && !InKeyEvent.IsRepeat() && OnEscape)
				return OnEscape();
			return false;
		}
	};
}

ALastSignalPlayerController::ALastSignalPlayerController()
{
	HUDComponent = CreateDefaultSubobject<ULastSignalPlayerHUDComponent>(TEXT("HUDComponent"));
}

void ALastSignalPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// ESC: 인트로 중엔 두 번 눌러 건너뛰기, 게임 중엔 일시정지 (메인메뉴는 제외)
	if (IsLocalController() && FSlateApplication::IsInitialized() && UGameplayStatics::GetCurrentLevelName(this) != TEXT("L_MainMenu"))
	{
		TSharedRef<FIntroSkipInputProcessor> Processor = MakeShared<FIntroSkipInputProcessor>();
		TWeakObjectPtr<ALastSignalPlayerController> WeakThis(this);
		Processor->OnEscape = [WeakThis]()
		{
			ALastSignalPlayerController *Controller = WeakThis.Get();
			return Controller && Controller->HandleEscapeKey();
		};
		IntroSkipInput = Processor;
		FSlateApplication::Get().RegisterInputPreProcessor(Processor);
	}

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
	if (bIntroSkipRequested)
	{
		bIntroSkipRequested = false;
		SkipIntro();
	}
	const bool bSlideshowOnScreen = IsStorySlideshowOnScreen();

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
		if (HUDWidgetInstance)
			HUDWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible); // 화면과 같이 서서히 나타나게 지금 켬 (투명도는 위에서 0 → 1)
		GetWorldTimerManager().SetTimer(IntroFadeTimerHandle, this, &ALastSignalPlayerController::FinishIntroFade, FadeSeconds, false);
	}
	bStorySlideshowWasOnScreen = bSlideshowOnScreen;

	// (페이드인 시작 처리 뒤에 계산해야 슬라이드쇼가 사라진 첫 프레임에 HUD가 번쩍 안 보임)
	// HUD 투명도: 슬라이드쇼 중엔 0, 인트로 뒤 페이드인 동안엔 화면과 같이 0 → 1로 서서히, 그 외엔 1
	float HudAlpha = 1.0f;
	if (bSlideshowOnScreen)
		HudAlpha = 0.0f;
	else if (GetWorldTimerManager().IsTimerActive(IntroFadeTimerHandle))
		HudAlpha = FMath::Clamp(GetWorldTimerManager().GetTimerElapsed(IntroFadeTimerHandle) / FMath::Max(GetWorldTimerManager().GetTimerRate(IntroFadeTimerHandle), 0.01f), 0.0f, 1.0f);
	for (UWidget *HudPart : {WatchWidget.Get(), HealthWidget.Get(), SkillWidget.Get(), static_cast<UWidget *>(HUDWidgetInstance.Get())})
	{
		if (HudPart)
			HudPart->SetRenderOpacity(HudAlpha);
	}

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
	// 요소마다 자기 모서리 기준으로만 줄이면 같은 모서리에 쌓인 요소들(총 그림, 탄약, 스킬)끼리 간격이 틀어짐
	// → 줄인 다음, 화면 모서리 기준으로 줄인 것과 같은 자리로 옮겨 줌 (위치도 같은 비율로)
	UCanvasPanel *Root = HUDWidgetInstance ? Cast<UCanvasPanel>(HUDWidgetInstance->GetRootWidget()) : nullptr;
	if (!Root)
		return;
	HUDWidgetInstance->ForceLayoutPrepass(); // 자동 크기 요소의 실제 크기를 알기 위해
	for (UWidget *Child : Root->GetAllChildren())
	{
		const UCanvasPanelSlot *ChildSlot = Cast<UCanvasPanelSlot>(Child->Slot);
		if (!ChildSlot)
			continue;
		const FAnchors Anchors = ChildSlot->GetAnchors();
		if (Anchors.IsStretchedHorizontal() || Anchors.IsStretchedVertical() || Anchors.Minimum.Equals(FVector2D(0.5f, 0.5f)))
			continue;
		const FVector2D Size = ChildSlot->GetAutoSize() ? Child->GetDesiredSize() : ChildSlot->GetSize();
		// 기준점(요소의 모서리 쪽 점)이 화면 앵커에서 떨어진 거리
		const FVector2D PivotFromAnchor = ChildSlot->GetPosition() + (Anchors.Minimum - ChildSlot->GetAlignment()) * Size;
		Child->SetRenderTransformPivot(Anchors.Minimum);
		Child->SetRenderScale(Scale);
		Child->SetRenderTranslation(PivotFromAnchor * (HudScale - 1.0f));
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

UUserWidget *ALastSignalPlayerController::FindStorySlideshow() const
{
	// 레벨 BP/게임모드 BP가 만든 위젯이라 C++에서 클래스를 모름 → 경로로 찾음 (아직 안 불러와졌으면 = 화면에 없음)
	const TSubclassOf<UUserWidget> SlideshowClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/HUD/WBP_StorySlideshow.WBP_StorySlideshow_C"))).Get();
	if (!SlideshowClass)
		return nullptr;

	TArray<UUserWidget *> Slideshows;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(const_cast<ALastSignalPlayerController *>(this), Slideshows, SlideshowClass, /*TopLevelOnly = 화면에 붙은 것만*/ true);
	return Slideshows.Num() > 0 ? Slideshows[0] : nullptr;
}

// ===================== 인트로 건너뛰기 (ESC 두 번) =====================


bool ALastSignalPlayerController::HandleEscapeKey()
{
	if (!IsStorySlideshowOnScreen())
	{
		// 게임 중 ESC = 일시정지 (P와 같음). 일시정지 화면이 떠 있으면 그 화면이 ESC로 닫으니 여기선 안 건드림
		// 게임오버/크레딧/무기 선택처럼 마우스가 보이는 화면, 인트로 뒤 페이드인 중에도 안 건드림
		if (GetWorld()->IsPaused() || bShowMouseCursor || GetWorldTimerManager().IsTimerActive(IntroFadeTimerHandle))
			return false;
		TogglePauseMenu();
		return true; // 먹어야 방금 뜬 일시정지 화면이 같은 ESC를 받아 바로 닫히지 않음
	}

	const double Now = FPlatformTime::Seconds();
	if (Now - LastIntroEscapeSeconds < 2.0)
	{
		bIntroSkipRequested = true; // 실제 건너뛰기는 다음 PlayerTick에서
		return true;
	}
	LastIntroEscapeSeconds = Now;

	// 오른쪽 아래 작은 안내 "ESC 한 번 더 누르면 건너뛰기" (2초 뒤 사라짐)
	if (!SkipHintWidget)
	{
		SkipHintWidget = CreateWidget<UIntroSkipHintWidget>(this, UIntroSkipHintWidget::StaticClass());
		if (SkipHintWidget && SkipHintWidget->WidgetTree)
		{
			UCanvasPanel *HintRoot = SkipHintWidget->WidgetTree->ConstructWidget<UCanvasPanel>();
			SkipHintWidget->WidgetTree->RootWidget = HintRoot;
			UTextBlock *Hint = SkipHintWidget->WidgetTree->ConstructWidget<UTextBlock>();
			Hint->SetText(FText::FromString(TEXT("ESC 한 번 더 누르면 건너뛰기")));
			FSlateFontInfo Font = Hint->GetFont();
			if (UFont *Pretendard = LoadObject<UFont>(nullptr, TEXT("/Game/UI/Fonts/Pretendard_Bold_Font.Pretendard_Bold_Font")))
			{
				Font.FontObject = Pretendard;
				Font.TypefaceFontName = NAME_None;
			}
			Font.Size = 14;
			Hint->SetFont(Font);
			Hint->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.75f)));
			UCanvasPanelSlot *HintSlot = HintRoot->AddChildToCanvas(Hint);
			HintSlot->SetAnchors(FAnchors(1.0f, 1.0f));
			HintSlot->SetAlignment(FVector2D(1.0f, 1.0f));
			HintSlot->SetPosition(FVector2D(-40.0f, -36.0f));
			HintSlot->SetAutoSize(true);
			SkipHintWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	if (SkipHintWidget && !SkipHintWidget->IsInViewport())
		SkipHintWidget->AddToViewport(60); // 슬라이드쇼 위
	GetWorldTimerManager().SetTimer(SkipHintTimerHandle, this, &ALastSignalPlayerController::HideSkipHint, 2.0f, false);
	return true;
}

void ALastSignalPlayerController::HideSkipHint()
{
	if (SkipHintWidget)
		SkipHintWidget->RemoveFromParent();
}

void ALastSignalPlayerController::SkipIntro()
{
	HideSkipHint();
	UUserWidget *Slideshow = FindStorySlideshow();
	if (!Slideshow)
		return;

	// 1) 지금 나오는 나레이션 끄기 (슬라이드쇼가 PlaySound2D로 틀어서 핸들이 없음 → 나레이션 목록의 소리를 전부 정지)
	if (const FArrayProperty *NarrationsProperty = FindFProperty<FArrayProperty>(Slideshow->GetClass(), TEXT("SlideNarrations")))
	{
		const FObjectPropertyBase *Inner = CastField<FObjectPropertyBase>(NarrationsProperty->Inner);
		FScriptArrayHelper Narrations(NarrationsProperty, NarrationsProperty->ContainerPtrToValuePtr<void>(Slideshow));
		FAudioDevice *AudioDevice = GetWorld()->GetAudioDeviceRaw();
		for (int32 Index = 0; Inner && AudioDevice && Index < Narrations.Num(); ++Index)
		{
			if (USoundWave *Narration = Cast<USoundWave>(Inner->GetObjectPropertyValue(Narrations.GetRawPtr(Index))))
				AudioDevice->StopSoundsUsingResource(Narration);
		}
	}

	// 2) 다음 슬라이드로 넘어가는 타이머 정지 (안 하면 떼어 낸 위젯이 다음 나레이션을 또 틂)
	GetWorldTimerManager().ClearAllTimersForObject(Slideshow);

	// 3) 슬라이드쇼를 떼어 냄 → 다음 PlayerTick이 사라진 걸 보고 원래 인트로 끝과 똑같이 페이드인 + 입력/마우스/HUD 복구 + 게임 시작
	//    (BP의 "끝났다" 신호를 코드로 억지로 보내면 크래시 나서, 정리는 여기서 직접 함)
	Slideshow->RemoveFromParent();
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void ALastSignalPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IntroSkipInput.IsValid() && FSlateApplication::IsInitialized())
		FSlateApplication::Get().UnregisterInputPreProcessor(IntroSkipInput);
	IntroSkipInput.Reset();
	Super::EndPlay(EndPlayReason);
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
