// HudHealthWidget.h
// LAST SIGNAL - 왼쪽 아래 체력 HUD (크로스파이어 풍). 군인 그림이 아래부터 체력만큼 색으로 차오름 + "HP 100".
// 초록(61~100) → 노랑(31~60) → 빨강(0~30). HUD 컴포넌트의 HP 신호를 받는다. (WBP 없이 코드로 구성)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudHealthWidget.generated.h"

class ULastSignalPlayerHUDComponent;
class UImage;
class USizeBox;
class UTextBlock;

UCLASS()
class LASTSIGNAL_API UHudHealthWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // PlayerController가 생성 직후 호출: 신호 연결 + 현재 값 한 번 표시
    void BindHUDComponent(ULastSignalPlayerHUDComponent *InComponent);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    void BuildWidgetTree();

    UFUNCTION()
    void HandleHPChanged(float CurrentHP, float MaxHP);

    UPROPERTY()
    TObjectPtr<USizeBox> FillClip; // 높이 = 그림 크기 × 체력 비율, 넘치는 부분은 잘림

    UPROPERTY()
    TObjectPtr<UImage> FillImage; // 색이 칠해진 그림 (FillClip 안에서 아래 기준)

    UPROPERTY()
    TObjectPtr<UTextBlock> LabelText;

    UPROPERTY()
    TObjectPtr<UTextBlock> ValueText;
};
