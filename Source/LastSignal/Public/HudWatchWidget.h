// HudWatchWidget.h
// LAST SIGNAL - 오른쪽 위 손목시계 HUD (프로젝트 좀보이드 풍). 반투명 판 + 7세그먼트(DSEG7) 숫자 + KILLS/처치 수 한 줄.
// 스톱워치(하늘색) → 무전기 이후 카운트다운(빨간색). HUD 컴포넌트의 타이머/점수 신호를 받는다. (WBP 없이 코드로 구성)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastSignalHUDTypes.h"
#include "HudWatchWidget.generated.h"

class ULastSignalPlayerHUDComponent;
class UTextBlock;

UCLASS()
class LASTSIGNAL_API UHudWatchWidget : public UUserWidget
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
    void HandleTimerUpdated(float TimeValue, ELastSignalTimerMode Mode);

    UFUNCTION()
    void HandleScoreChanged(int32 NewScore);

    UPROPERTY()
    TObjectPtr<UTextBlock> DigitsText;

    UPROPERTY()
    TObjectPtr<UTextBlock> GhostText; // 뒤에 옅게 깔리는 88:88 (꺼진 LCD 칸)

    UPROPERTY()
    TObjectPtr<UTextBlock> ModeText;

    UPROPERTY()
    TObjectPtr<UTextBlock> KillsText;
};
