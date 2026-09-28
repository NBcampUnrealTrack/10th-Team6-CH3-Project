// InteractPromptWidget.h
// LAST SIGNAL - 상호작용 가능한 물건(무전기, 무기 상자, 탄약 상자, 메디킷) 근처에서 뜨는 [F] 안내.
// 플레이어의 NearbyInteractable을 매 프레임 확인해서, 그 물건의 GetInteractPromptText를 띄운다 (WBP 없이 코드로 구성).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractPromptWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS()
class LASTSIGNAL_API UInteractPromptWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry &MyGeometry, float InDeltaTime) override;

private:
    void BuildWidgetTree();

    UPROPERTY()
    TObjectPtr<UTextBlock> ActionText;

    // 둥근 테두리 선은 위젯 투명도(RenderOpacity)가 안 먹어서, 테두리 색 알파도 따로 맞춘다
    UPROPERTY()
    TObjectPtr<UBorder> KeyCap;

    float CurrentOpacity = 0.0f;
};
