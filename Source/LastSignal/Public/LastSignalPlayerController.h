// LastSignalPlayerController.h
// LAST SIGNAL - HUD 컴포넌트를 소유하고, 로컬 플레이어에게 HUD 위젯을 생성/장착한다.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LastSignalPlayerController.generated.h"

class ULastSignalHUDWidget;
class ULastSignalPlayerHUDComponent;
class UUserWidget;

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

protected:
	virtual void BeginPlay() override;

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
