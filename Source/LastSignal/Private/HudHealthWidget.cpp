#include "HudHealthWidget.h"
#include "LastSignalPlayerHUDComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"

namespace
{
    constexpr float PortraitSize = 150.0f;
    constexpr float LabelFontSize = 24.0f;
    constexpr float ValueFontSize = 46.0f;
    // Black Ops One은 글자 아래 빈 줄 공간이 큼 (스크린샷 실측: 글자 크기의 약 0.52~0.58배)
    // → 숫자 밑선을 그림 발끝에 맞추려고 그만큼 아래로 내림
    constexpr float ValueDescent = ValueFontSize * 0.523f;
    constexpr float LabelDescent = LabelFontSize * 0.577f;
    const TCHAR *HealthFont = TEXT("/Game/UI/Fonts/BlackOpsOne_Font.BlackOpsOne_Font"); // 메인메뉴 제목 폰트

    // 체력 구간별 색 (HTML 목업과 같은 색)
    FLinearColor HealthColor(float Percent01)
    {
        if (Percent01 > 0.6f)
            return FLinearColor(FColor::FromHex(TEXT("5FD35F"))); // 초록
        if (Percent01 > 0.3f)
            return FLinearColor(FColor::FromHex(TEXT("F2C230"))); // 노랑
        return FLinearColor(FColor::FromHex(TEXT("FF4A3D")));     // 빨강
    }

    UTextBlock *MakeHealthText(UWidgetTree *Tree, const FString &String, int32 Size, const TCHAR *FontPath, int32 LetterSpacing)
    {
        UTextBlock *Text = Tree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(String));
        FSlateFontInfo Font = Text->GetFont();
        if (UFont *Loaded = LoadObject<UFont>(nullptr, FontPath))
        {
            Font.FontObject = Loaded;
            Font.TypefaceFontName = NAME_None;
        }
        else
        {
            Font.TypefaceFontName = TEXT("Bold");
        }
        Font.Size = Size;
        Font.LetterSpacing = LetterSpacing;
        Font.OutlineSettings.OutlineSize = 1; // 밝은 배경에서도 읽히게
        Font.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
        Text->SetFont(Font);
        return Text;
    }

    UImage *MakePortraitImage(UWidgetTree *Tree, UTexture2D *Texture)
    {
        UImage *Image = Tree->ConstructWidget<UImage>();
        FSlateBrush Brush;
        Brush.SetResourceObject(Texture);
        Brush.SetImageSize(FVector2D(PortraitSize)); // SetDesiredSizeOverride는 빌드 전에 사라져서 브러시 크기로
        Image->SetBrush(Brush);
        return Image;
    }
}

TSharedRef<SWidget> UHudHealthWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
        BuildWidgetTree();

    return Super::RebuildWidget();
}

void UHudHealthWidget::BuildWidgetTree()
{
    UCanvasPanel *Root = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Root;

    // 왼쪽 아래: [군인 그림] [HP 100] — 그림과 글자를 따로 놓고, 글자 밑선을 그림 아래선에 맞춤
    constexpr float ScreenMargin = 32.0f;

    UTexture2D *Portrait = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/HUD/T_HealthPortrait.T_HealthPortrait"));

    // 그림: 흐린 흰 그림 위에, 아래부터 체력만큼만 보이는 색 그림을 겹침
    USizeBox *Figure = WidgetTree->ConstructWidget<USizeBox>();
    Figure->SetWidthOverride(PortraitSize);
    Figure->SetHeightOverride(PortraitSize);
    UCanvasPanelSlot *FigureSlot = Root->AddChildToCanvas(Figure);
    FigureSlot->SetAnchors(FAnchors(0.0f, 1.0f));
    FigureSlot->SetAlignment(FVector2D(0.0f, 1.0f));
    FigureSlot->SetPosition(FVector2D(ScreenMargin, -ScreenMargin));
    FigureSlot->SetAutoSize(true);

    UOverlay *FigureOverlay = WidgetTree->ConstructWidget<UOverlay>();
    Figure->SetContent(FigureOverlay);

    UImage *EmptyImage = MakePortraitImage(WidgetTree, Portrait);
    EmptyImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.22f)); // 체력이 빠진 부분
    FigureOverlay->AddChildToOverlay(EmptyImage);

    FillClip = WidgetTree->ConstructWidget<USizeBox>();
    FillClip->SetHeightOverride(PortraitSize);
    FillClip->SetClipping(EWidgetClipping::ClipToBounds);
    UOverlaySlot *ClipSlot = FigureOverlay->AddChildToOverlay(FillClip);
    ClipSlot->SetHorizontalAlignment(HAlign_Fill);
    ClipSlot->SetVerticalAlignment(VAlign_Bottom);

    // 잘리는 상자 안에서 그림은 항상 원래 크기로 아래에 붙어 있음 → 상자가 줄면 위부터 사라짐
    UCanvasPanel *FillCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    FillClip->SetContent(FillCanvas);
    FillImage = MakePortraitImage(WidgetTree, Portrait);
    UCanvasPanelSlot *FillSlot = FillCanvas->AddChildToCanvas(FillImage);
    FillSlot->SetAnchors(FAnchors(0.0f, 1.0f));
    FillSlot->SetAlignment(FVector2D(0.0f, 1.0f));
    FillSlot->SetPosition(FVector2D::ZeroVector);
    FillSlot->SetSize(FVector2D(PortraitSize));

    // 숫자: HP 100
    UHorizontalBox *NumberRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    UCanvasPanelSlot *NumberSlot = Root->AddChildToCanvas(NumberRow);
    NumberSlot->SetAnchors(FAnchors(0.0f, 1.0f));
    NumberSlot->SetAlignment(FVector2D(0.0f, 1.0f));
    // 가로: 그림 오른쪽 끝 바로 옆 (그림 파일 양옆 투명 여백이 약 5%라 살짝 붙임)
    // 세로: 숫자 밑선 = 그림 아래선에서 6px 위
    NumberSlot->SetPosition(FVector2D(ScreenMargin + PortraitSize - PortraitSize * 0.05f + 12.0f, -ScreenMargin - 6.0f + ValueDescent));
    NumberSlot->SetAutoSize(true);

    // HP 글자, 숫자 모두 제목 폰트(Black Ops One) + 같은 색
    LabelText = MakeHealthText(WidgetTree, TEXT("HP"), LabelFontSize, HealthFont, 0);
    UHorizontalBoxSlot *LabelSlot = NumberRow->AddChildToHorizontalBox(LabelText);
    LabelSlot->SetVerticalAlignment(VAlign_Bottom);
    LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, ValueDescent - LabelDescent)); // 작은 HP 글자 밑선을 큰 숫자 밑선에 맞춤

    ValueText = MakeHealthText(WidgetTree, TEXT("100"), ValueFontSize, HealthFont, 0);
    NumberRow->AddChildToHorizontalBox(ValueText)->SetVerticalAlignment(VAlign_Bottom);

    HandleHPChanged(100.0f, 100.0f);
    SetVisibility(ESlateVisibility::HitTestInvisible); // 클릭을 가로채지 않게
}

void UHudHealthWidget::BindHUDComponent(ULastSignalPlayerHUDComponent *InComponent)
{
    if (!InComponent)
        return;

    InComponent->OnHPChanged.AddDynamic(this, &UHudHealthWidget::HandleHPChanged);
    HandleHPChanged(InComponent->CurrentHP, InComponent->MaxHP); // 위젯이 늦게 붙어도 현재 값부터
}

void UHudHealthWidget::HandleHPChanged(float CurrentHP, float MaxHP)
{
    if (!FillClip) // 아직 화면이 안 만들어졌으면(AddToViewport 전) 건너뜀
        return;

    const float Percent = MaxHP > 0.0f ? FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f) : 0.0f;
    const FLinearColor Color = HealthColor(Percent);

    FillClip->SetHeightOverride(PortraitSize * Percent);
    FillImage->SetColorAndOpacity(Color);
    ValueText->SetText(FText::AsNumber(FMath::CeilToInt(FMath::Max(0.0f, CurrentHP))));
    ValueText->SetColorAndOpacity(FSlateColor(Color));
    LabelText->SetColorAndOpacity(FSlateColor(Color));
}
