// HudSkillWidget.h
// LAST SIGNAL - 스페셜 스킬 게이지. WBP_LastSignalHUD의 "SPECIAL SKILL" 배너(Image_GaugeReady)를 같은 자리에서 대체.
// 흐린 배너 위에 게이지만큼 왼쪽 → 오른쪽으로 차오르고, 다 차면(사용 가능) 색이 완전히 진해짐. 원래 PRESS 글자(Text_SkillReady)는 빼고, 다 차면 배너 위에 작게 PRESS Q.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "SkillComponent.h"
#include "HudSkillWidget.generated.h"

class UImage;
class USizeBox;
class UTextBlock;
class UUserWidget;

UCLASS()
class LASTSIGNAL_API UHudSkillWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // AddToViewport 전에 호출: 원래 배너의 그림/위치를 가져오고 원래 배너와 PRESS 글자는 HUD에서 뺌
    // 배너가 HUD 최상위 캔버스에 바로 있지 않으면 false (원래 UI 그대로 둠)
    bool TakeOverFromHUD(UUserWidget *HUDWidget);

    // 배너가 붙어 있는 화면 모서리 (HUD 전체 크기 조절 때 이 점을 기준으로 줄임)
    FVector2D GetScreenAnchor() const { return BannerLayout.Anchors.Minimum; }

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry &MyGeometry, float InDeltaTime) override;

private:
    void BuildWidgetTree();
    void Refresh();

    UFUNCTION()
    void HandleSkillValueChanged(float CurrentValue, float MaxValue);

    UFUNCTION()
    void HandleSkillStateChanged(ESkillState NewState);

    FSlateBrush BannerBrush;
    FAnchorData BannerLayout;
    FVector2D BannerSize = FVector2D::ZeroVector;

    TWeakObjectPtr<USkillComponent> BoundSkill; // 캐릭터가 늦게 생겨도 Tick에서 찾아 연결
    float Percent = 0.0f;
    bool bReady = false;

    UPROPERTY()
    TObjectPtr<USizeBox> FillClip;

    UPROPERTY()
    TObjectPtr<UImage> FillImage;

    UPROPERTY()
    TObjectPtr<UTextBlock> PressText; // 다 찼을 때만 보이는 PRESS Q
};
