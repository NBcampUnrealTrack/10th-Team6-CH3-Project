#include "HudWatchWidget.h"
#include "LastSignalPlayerHUDComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"

namespace
{
    const FLinearColor StopwatchColor(0.44f, 0.89f, 0.96f, 1.0f); // 하늘색
    const FLinearColor CountdownColor(1.0f, 0.29f, 0.24f, 1.0f); // 빨간색

    UTextBlock *MakeWatchText(UWidgetTree *Tree, const FString &String, int32 Size, const TCHAR *FontPath, int32 LetterSpacing)
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
        Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(StopwatchColor));
        return Text;
    }

    const TCHAR *SegmentFont = TEXT("/Game/UI/Fonts/DSEG7_Font.DSEG7_Font");
    const TCHAR *LabelFont = TEXT("/Game/UI/Fonts/Pretendard_Bold_Font.Pretendard_Bold_Font");
}

TSharedRef<SWidget> UHudWatchWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
        BuildWidgetTree();

    return Super::RebuildWidget();
}

void UHudWatchWidget::BuildWidgetTree()
{
    UCanvasPanel *Root = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Root;

    // 반투명 검은 판 + 얇은 흰 테두리 (오른쪽 위)
    UBorder *Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f), 5.0f, FLinearColor(1.0f, 1.0f, 1.0f, 0.28f), 1.0f));
    Panel->SetPadding(FMargin(12.0f, 7.0f, 12.0f, 6.0f));
    UCanvasPanelSlot *PanelSlot = Root->AddChildToCanvas(Panel);
    PanelSlot->SetAnchors(FAnchors(1.0f, 0.0f));
    PanelSlot->SetAlignment(FVector2D(1.0f, 0.0f));
    PanelSlot->SetPosition(FVector2D(-32.0f, 32.0f));
    PanelSlot->SetAutoSize(true);

    UVerticalBox *Content = WidgetTree->ConstructWidget<UVerticalBox>();
    Panel->SetContent(Content);

    // 큰 숫자: 옅은 88:88 위에 실제 시간을 겹침 (진짜 LCD처럼 꺼진 칸이 보임)
    UOverlay *DigitsOverlay = WidgetTree->ConstructWidget<UOverlay>();
    Content->AddChildToVerticalBox(DigitsOverlay);
    GhostText = MakeWatchText(WidgetTree, TEXT("88:88"), 34, SegmentFont, 0);
    GhostText->SetRenderOpacity(0.08f);
    DigitsOverlay->AddChildToOverlay(GhostText);
    DigitsText = MakeWatchText(WidgetTree, TEXT("00:00"), 34, SegmentFont, 0);
    DigitsOverlay->AddChildToOverlay(DigitsText);

    // 작은 줄: KILLS ··· 처치 수
    UHorizontalBox *SubRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    Content->AddChildToVerticalBox(SubRow)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
    ModeText = MakeWatchText(WidgetTree, TEXT("KILLS"), 11, LabelFont, 150); // 왼쪽 라벨 (고정)
    SubRow->AddChildToHorizontalBox(ModeText);
    SubRow->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>())->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    KillsText = MakeWatchText(WidgetTree, TEXT("0"), 11, LabelFont, 150); // 오른쪽 숫자
    UHorizontalBoxSlot *KillsSlot = SubRow->AddChildToHorizontalBox(KillsText);
    KillsSlot->SetPadding(FMargin(16.0f, 0.0f, 0.0f, 0.0f));

    SetVisibility(ESlateVisibility::HitTestInvisible); // 클릭을 가로채지 않게
}

void UHudWatchWidget::BindHUDComponent(ULastSignalPlayerHUDComponent *InComponent)
{
    if (!InComponent)
        return;

    InComponent->OnTimerUpdated.AddDynamic(this, &UHudWatchWidget::HandleTimerUpdated);
    InComponent->OnScoreChanged.AddDynamic(this, &UHudWatchWidget::HandleScoreChanged);

    // 위젯이 늦게 붙어도 현재 값부터 보이게
    HandleTimerUpdated(InComponent->TimeValue, InComponent->TimerMode);
    HandleScoreChanged(InComponent->Score);
}

void UHudWatchWidget::HandleTimerUpdated(float TimeValue, ELastSignalTimerMode Mode)
{
    if (!DigitsText) // 아직 화면이 안 만들어졌으면(AddToViewport 전) 건너뜀 — 1초 뒤 다음 신호에 표시됨
        return;

    const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(TimeValue));
    DigitsText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), TotalSeconds / 60, TotalSeconds % 60)));

    const bool bCountdown = Mode == ELastSignalTimerMode::Countdown;
    const FSlateColor Color(bCountdown ? CountdownColor : StopwatchColor);
    DigitsText->SetColorAndOpacity(Color);
    GhostText->SetColorAndOpacity(Color);
    ModeText->SetColorAndOpacity(Color);
    KillsText->SetColorAndOpacity(Color);
}

void UHudWatchWidget::HandleScoreChanged(int32 NewScore)
{
    if (KillsText)
        KillsText->SetText(FText::AsNumber(NewScore));
}
