// LastSignalPlayerController.h
// LAST SIGNAL - HUD 컴포넌트를 소유하고, 로컬 플레이어에게 HUD 위젯을 생성/장착한다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LastSignalPlayerController.generated.h"

class ULastSignalHUDWidget;
class ULastSignalPlayerHUDComponent;
class UUserWidget;
class UWidget;

UCLASS()
class LASTSIGNAL_API ALastSignalPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALastSignalPlayerController();

	UFUNCTION(BlueprintCallable, Category = "LastSignal|HUD")
	ULastSignalPlayerHUDComponent* GetHUDComponent() const { return HUDComponent; }

	UFUNCTION(BlueprintCallable, Category = "LastSignal|HUD")
	ULastSignalHUDWidget* GetHUDWidget() const { return HUDWidgetInstance; }

	// 입력에서 특수공격(아드레날린) 버튼이 눌렸을 때 바인딩할 함수
	UFUNCTION(BlueprintCallable, Category = "LastSignal|Input")
	void TryTriggerSpecialAttack();

	UFUNCTION(BlueprintCallable, Category = "LastSignal|Input")
	void TogglePauseMenu();

	// 인트로/엔딩 슬라이드쇼 중이거나 인트로 뒤 페이드인 중 → 시계 숨김 + 타이머 멈춤
	bool IsInCutscene() const;

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override; // 조준 중이면 크로스헤어 숨김

	// WBP_LastSignalHUD의 Image_Crosshair (BeginPlay에서 찾아 둠)
	TWeakObjectPtr<UWidget> CrosshairWidget;

	// 오른쪽 위 손목시계 (슬라이드쇼 중엔 숨김)
	TWeakObjectPtr<UWidget> WatchWidget;

	// 왼쪽 아래 체력 (슬라이드쇼 중엔 숨김)
	TWeakObjectPtr<UWidget> HealthWidget;

	// 인트로/엔딩 슬라이드쇼(WBP_StorySlideshow)가 화면에 떠 있는지
	bool IsStorySlideshowOnScreen() const;

	// 지난 프레임에 슬라이드쇼가 떠 있었는지 → 사라지는 순간(인트로 끝) 페이드인 시작
	bool bStorySlideshowWasOnScreen = false;

	// 인트로 뒤 페이드인 (검은 화면 → 밝아짐, 그동안 조작 불가). 끝나면 FinishIntroFade
	FTimerHandle IntroFadeTimerHandle;
	void FinishIntroFade();

	// 에디터에서 WBP_LastSignalHUD 를 지정
	UPROPERTY(EditDefaultsOnly, Category = "LastSignal|HUD")
	TSubclassOf<ULastSignalHUDWidget> HUDWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LastSignal|HUD")
	TObjectPtr<ULastSignalPlayerHUDComponent> HUDComponent;

	UPROPERTY(BlueprintReadOnly, Category = "LastSignal|HUD")
	TObjectPtr<ULastSignalHUDWidget> HUDWidgetInstance;

	// 에디터에서 WBP_Pause 를 지정
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> PauseMenuWidgetInstance;
};
