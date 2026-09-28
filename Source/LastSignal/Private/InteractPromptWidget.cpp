#include "InteractPromptWidget.h"
#include "InteractableTarget.h"
#include "PlayerCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

namespace
{
    // F 키캡: 옅은 검정 채우기 + 흰 테두리. Opacity로 테두리 선까지 같이 투명하게
    FSlateRoundedBoxBrush MakeKeyCapBrush(float Opacity)
    {
        return FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.25f), 6.0f, FLinearColor(0.95f, 0.95f, 0.95f, 0.9f * Opacity), 1.5f);
    }

    // 밝은 배경에서도 읽히게 흰 글자 + 옅은 그림자
    UTextBlock *MakePromptText(UWidgetTree *Tree, const FString &String, int32 Size, bool bBold)
    {
        UTextBlock *Text = Tree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(String));
        FSlateFontInfo Font = Text->GetFont();
        Font.Size = Size;
        if (bBold)
            Font.TypefaceFontName = TEXT("Bold");
        Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.0f)));
        Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
        Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
        return Text;
    }
}

TSharedRef<SWidget> UInteractPromptWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
        BuildWidgetTree();

    return Super::RebuildWidget();
}

void UInteractPromptWidget::BuildWidgetTree()
{
    UCanvasPanel *Root = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Root;

    // [F] 무전기 사용 — 무기 선택 화면의 [ESC][X] 키캡과 같은 모양 (흰 테두리, 둥글기 6, 두께 1.5)
    UHorizontalBox *Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    UCanvasPanelSlot *RowSlot = Root->AddChildToCanvas(Row);
    RowSlot->SetAnchors(FAnchors(0.5f, 0.62f)); // 조준점 조금 아래
    RowSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    RowSlot->SetAutoSize(true);

    USizeBox *KeyCapSize = WidgetTree->ConstructWidget<USizeBox>();
    KeyCapSize->SetWidthOverride(34.0f);
    KeyCapSize->SetHeightOverride(34.0f);
    KeyCap = WidgetTree->ConstructWidget<UBorder>();
    KeyCap->SetBrush(MakeKeyCapBrush(0.0f));
    KeyCap->SetHorizontalAlignment(HAlign_Center);
    KeyCap->SetVerticalAlignment(VAlign_Center);
    KeyCap->SetContent(MakePromptText(WidgetTree, TEXT("F"), 16, true));
    KeyCapSize->AddChild(KeyCap);
    Row->AddChildToHorizontalBox(KeyCapSize)->SetVerticalAlignment(VAlign_Center);

    ActionText = MakePromptText(WidgetTree, TEXT(""), 20, false);
    UHorizontalBoxSlot *ActionSlot = Row->AddChildToHorizontalBox(ActionText);
    ActionSlot->SetVerticalAlignment(VAlign_Center);
    ActionSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));

    SetVisibility(ESlateVisibility::HitTestInvisible); // 마우스 클릭을 가로채지 않게
    SetRenderOpacity(0.0f);
}

void UInteractPromptWidget::NativeTick(const FGeometry &MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 가까운 물건이 있고 안내 문구가 비어 있지 않을 때만 보인다 (문구는 물건마다 GetInteractPromptText로 정함)
    FText PromptText;
    if (const APlayerCharacter *Player = Cast<APlayerCharacter>(GetOwningPlayerPawn()))
    {
        AActor *Target = Player->NearbyInteractable;
        if (IsValid(Target) && Target->Implements<UInteractableTarget>())
            PromptText = IInteractableTarget::Execute_GetInteractPromptText(Target);
    }

    const bool bShow = !PromptText.IsEmpty();
    if (bShow)
        ActionText->SetText(PromptText);

    // 0.1초 정도에 걸쳐 부드럽게 나타나고 사라짐
    CurrentOpacity = FMath::FInterpTo(CurrentOpacity, bShow ? 1.0f : 0.0f, InDeltaTime, 15.0f);
    SetRenderOpacity(CurrentOpacity);
    // 둥근 테두리의 선 색은 위젯 투명도/BrushColor가 안 먹음(DrawElementTypes.cpp) → 선 색의 알파를 직접 바꾼 브러시로 교체
    KeyCap->SetBrush(MakeKeyCapBrush(CurrentOpacity));
    // Hidden으로 숨기면 Slate가 Tick을 안 불러서 다시 나타나지 못함 → 항상 HitTestInvisible로 두고 투명도로만 숨긴다
}
