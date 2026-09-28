// WeaponSelectWidget.h
// LAST SIGNAL - 보급 상자(F) 상호작용 시 뜨는 주무기 선택 화면.
// UMG 에디터 없이 위젯 트리를 전부 코드로 만든다 (WBP 없음).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponSelectWidget.generated.h"

class AWeaponSelectionBox;
class UBorder;
class UImage;
class UTextBlock;
class UProgressBar;
class UWeaponSelectWidget;

// 카드 한 장 (사진, 종류 태그, 탄약, 설명, 스탯 막대). 마우스를 올리면 커지고 주황빛이 난다.
UCLASS()
class LASTSIGNAL_API UWeaponCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 0 = M4A1, 1 = SR25, 2 = MP153 (상자의 AvailablePrimaryWeapons 순서와 같음)
    int32 WeaponIndex = 0;

    TWeakObjectPtr<UWeaponSelectWidget> OwnerSelectWidget;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    void BuildWidgetTree();
    void SetHighlighted(bool bHighlighted);

    UFUNCTION()
    void HandleHovered();

    UFUNCTION()
    void HandleUnhovered();

    UFUNCTION()
    void HandleClicked();

    UPROPERTY()
    TObjectPtr<UImage> GlowImage;

    UPROPERTY()
    TArray<TObjectPtr<UProgressBar>> StatBars;
};

// 선택 화면 전체 (어두운 배경, 제목, 카드 3장, 안내 문구, 닫기 버튼)
UCLASS()
class LASTSIGNAL_API UWeaponSelectWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 이 화면을 연 보급 상자 — 카드를 고르면 이 상자의 SelectAndEquipWeapon을 호출
    TWeakObjectPtr<AWeaponSelectionBox> SelectionBox;

    void SelectWeapon(int32 WeaponIndex);

    UFUNCTION()
    void CloseWidget();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent) override; // ESC로 닫기

private:
    void BuildWidgetTree();
    void SetCloseHighlighted(bool bHighlighted);

    UFUNCTION()
    void HandleCloseHovered();

    UFUNCTION()
    void HandleCloseUnhovered();

    // 닫기 버튼: 얇은 선 두 개로 그린 X + ESC 키캡 (호버 시 전부 주황)
    UPROPERTY()
    TArray<TObjectPtr<UImage>> CloseCrossLines;

    UPROPERTY()
    TObjectPtr<UBorder> CloseKeyCap;

    UPROPERTY()
    TObjectPtr<UTextBlock> CloseKeyText;

    UPROPERTY()
    TObjectPtr<UBorder> CloseCrossCap; // X를 담은 키캡 (ESC 키캡과 같은 모양)
};
