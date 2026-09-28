#include "WeaponSelectWidget.h"
#include "WeaponSelectionBox.h"
#include "PrimaryWeapon.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"

namespace
{
    // 카드에 표시할 무기 정보 (인덱스 = 상자의 AvailablePrimaryWeapons 순서)
    struct FWeaponCardInfo
    {
        const TCHAR *Name;
        const TCHAR *Type;
        const TCHAR *Ammo;
        const TCHAR *Description;
        const TCHAR *IconPath;
        float Stats[4]; // 공격력, 연사, 사거리, 탄창 (0~1)
    };

    // ponytail: 스탯은 PrimaryFirearms.cpp 수치를 손으로 옮긴 값 — 밸런스를 바꾸면 여기도 같이 수정
    // 공격력 = 피해 × 산탄 수(25, 55, 90), 연사 = RPM(750, 300, 180), 사거리(10000, 20000, 4000), 탄창(30, 20, 6), 항목별 최댓값 = 1
    const FWeaponCardInfo WeaponCards[] = {
        {TEXT("M4A1"), TEXT("돌격소총"), TEXT("30 / 120"),
         TEXT("자동 사격이 가능한 돌격소총. 연사력이 높아 여러 마리를 상대하기 좋다."),
         TEXT("/Game/UI/HUD/T_WeaponCard_M4A1.T_WeaponCard_M4A1"), {0.28f, 1.0f, 0.5f, 1.0f}},
        {TEXT("SR25"), TEXT("지정사수"), TEXT("20 / 80"),
         TEXT("한 발의 위력이 강한 지정사수 소총. 멀리 있는 적을 정확하게 처리한다."),
         TEXT("/Game/UI/HUD/T_WeaponCard_SR25.T_WeaponCard_SR25"), {0.61f, 0.4f, 1.0f, 0.67f}},
        {TEXT("MP153"), TEXT("산탄총"), TEXT("6 / 36"),
         TEXT("근거리에서 압도적인 산탄총. 한 발씩 장전한다."),
         TEXT("/Game/UI/HUD/T_WeaponCard_MP153.T_WeaponCard_MP153"), {1.0f, 0.24f, 0.2f, 0.2f}},
    };

    const TCHAR *StatLabels[] = {TEXT("공격력"), TEXT("연사"), TEXT("사거리"), TEXT("탄창")};

    const FLinearColor Amber(1.0f, 0.6f, 0.1f, 1.0f);
    const FLinearColor Ink(0.91f, 0.93f, 0.93f, 1.0f);
    const FLinearColor Muted(0.55f, 0.58f, 0.61f, 1.0f);
    const FLinearColor CardFill(0.05f, 0.05f, 0.05f, 0.86f);
    const FLinearColor CardOutline(0.3f, 0.33f, 0.36f, 1.0f);

    UTextBlock *MakeText(UWidgetTree *Tree, const FString &String, int32 Size, const FLinearColor &Color, bool bBold = false)
    {
        UTextBlock *Text = Tree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(String));
        FSlateFontInfo Font = Text->GetFont();
        Font.Size = Size;
        if (bBold)
            Font.TypefaceFontName = TEXT("Bold");
        Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(Color));
        return Text;
    }

    // 평소 회색 테두리, 마우스를 올리거나 누르면 주황 테두리인 둥근 버튼 스타일
    FButtonStyle MakeRoundedButtonStyle(float Radius)
    {
        const FSlateRoundedBoxBrush Normal(CardFill, Radius, CardOutline, 2.0f);
        FButtonStyle Style;
        Style.SetNormal(Normal);
        Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.12f, 0.13f, 0.14f, 0.92f), Radius, Amber, 3.0f));
        Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.2f, 0.2f, 0.2f, 0.95f), Radius, Amber, 3.0f));
        Style.SetDisabled(Normal);
        Style.SetNormalPadding(FMargin(0.0f));
        Style.SetPressedPadding(FMargin(0.0f));
        return Style;
    }
}

// ===================== 카드 =====================

TSharedRef<SWidget> UWeaponCardWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
        BuildWidgetTree();

    return Super::RebuildWidget();
}

void UWeaponCardWidget::BuildWidgetTree()
{
    const FWeaponCardInfo &Info = WeaponCards[FMath::Clamp(WeaponIndex, 0, UE_ARRAY_COUNT(WeaponCards) - 1)];

    // 빛(뒤)과 카드(앞)를 같은 자리에 겹친다
    UOverlay *Root = WidgetTree->ConstructWidget<UOverlay>();
    WidgetTree->RootWidget = Root;

    GlowImage = WidgetTree->ConstructWidget<UImage>();
    if (UTexture2D *GlowTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/HUD/T_CardGlow.T_CardGlow")))
        GlowImage->SetBrushFromTexture(GlowTexture, true); // 440×640 원본 크기
    GlowImage->SetVisibility(ESlateVisibility::Hidden);
    UOverlaySlot *GlowSlot = Root->AddChildToOverlay(GlowImage);
    GlowSlot->SetHorizontalAlignment(HAlign_Center);
    GlowSlot->SetVerticalAlignment(VAlign_Center);

    USizeBox *CardSize = WidgetTree->ConstructWidget<USizeBox>();
    CardSize->SetWidthOverride(360.0f);
    CardSize->SetHeightOverride(560.0f);
    UOverlaySlot *CardSlot = Root->AddChildToOverlay(CardSize);
    CardSlot->SetHorizontalAlignment(HAlign_Center);
    CardSlot->SetVerticalAlignment(VAlign_Center);

    UButton *CardButton = WidgetTree->ConstructWidget<UButton>();
    CardButton->SetStyle(MakeRoundedButtonStyle(16.0f));
    CardButton->OnHovered.AddDynamic(this, &UWeaponCardWidget::HandleHovered);
    CardButton->OnUnhovered.AddDynamic(this, &UWeaponCardWidget::HandleUnhovered);
    CardButton->OnClicked.AddDynamic(this, &UWeaponCardWidget::HandleClicked);
    CardSize->AddChild(CardButton);

    UVerticalBox *Content = WidgetTree->ConstructWidget<UVerticalBox>();
    UButtonSlot *ContentSlot = Cast<UButtonSlot>(CardButton->AddChild(Content));
    ContentSlot->SetPadding(FMargin(25.0f));
    ContentSlot->SetHorizontalAlignment(HAlign_Fill);
    ContentSlot->SetVerticalAlignment(VAlign_Fill);

    // 윗줄: 종류 태그 ··· 탄약
    UHorizontalBox *TopRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    Content->AddChildToVerticalBox(TopRow);

    UBorder *TypeBorder = WidgetTree->ConstructWidget<UBorder>();
    TypeBorder->SetBrush(FSlateRoundedBoxBrush(FLinearColor(1.0f, 0.6f, 0.1f, 0.16f), 6.0f));
    TypeBorder->SetPadding(FMargin(10.0f, 3.0f));
    TypeBorder->SetContent(MakeText(WidgetTree, Info.Type, 14, Amber));
    TopRow->AddChildToHorizontalBox(TypeBorder);

    UHorizontalBoxSlot *TopSpacerSlot = TopRow->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>());
    TopSpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UHorizontalBoxSlot *AmmoSlot = TopRow->AddChildToHorizontalBox(MakeText(WidgetTree, Info.Ammo, 16, Muted));
    AmmoSlot->SetVerticalAlignment(VAlign_Center);

    // 총 사진 (옅은 칸 안)
    UBorder *IconBorder = WidgetTree->ConstructWidget<UBorder>();
    IconBorder->SetBrush(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.04f), 10.0f));
    IconBorder->SetPadding(FMargin(12.0f, 17.0f));
    // 사진마다 비율이 달라서(M4A1 1965×913, SR25 1653×602 …) 286×130 칸 안에 비율을 유지하며 맞추고,
    // 칸 높이는 고정해서 세 카드의 이름/설명 줄 높이를 맞춘다
    USizeBox *IconSize = WidgetTree->ConstructWidget<USizeBox>();
    IconSize->SetHeightOverride(130.0f);
    UImage *Icon = WidgetTree->ConstructWidget<UImage>();
    if (UTexture2D *IconTexture = LoadObject<UTexture2D>(nullptr, Info.IconPath))
    {
        Icon->SetBrushFromTexture(IconTexture);
        const float Aspect = IconTexture->GetSurfaceWidth() / FMath::Max(IconTexture->GetSurfaceHeight(), 1.0f);
        const float IconWidth = FMath::Min(286.0f, 130.0f * Aspect);
        Icon->SetDesiredSizeOverride(FVector2D(IconWidth, IconWidth / Aspect));
    }
    USizeBoxSlot *IconSlot = Cast<USizeBoxSlot>(IconSize->AddChild(Icon));
    IconSlot->SetHorizontalAlignment(HAlign_Center);
    IconSlot->SetVerticalAlignment(VAlign_Center);
    IconBorder->SetContent(IconSize);
    Content->AddChildToVerticalBox(IconBorder)->SetPadding(FMargin(0.0f, 17.0f, 0.0f, 0.0f));

    // 이름, 설명
    UVerticalBoxSlot *NameSlot = Content->AddChildToVerticalBox(MakeText(WidgetTree, Info.Name, 28, Ink, true));
    NameSlot->SetHorizontalAlignment(HAlign_Center);
    NameSlot->SetPadding(FMargin(0.0f, 17.0f, 0.0f, 0.0f));

    UTextBlock *Description = MakeText(WidgetTree, Info.Description, 18, FLinearColor(0.76f, 0.79f, 0.81f, 1.0f));
    Description->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Description)->SetPadding(FMargin(0.0f, 17.0f, 0.0f, 0.0f));

    // 설명 길이와 상관없이 스탯을 카드 맨 아래로 밀어냄
    Content->AddChildToVerticalBox(WidgetTree->ConstructWidget<USpacer>())->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    // 스탯 막대 4줄
    FProgressBarStyle BarStyle;
    BarStyle.SetBackgroundImage(FSlateRoundedBoxBrush(FLinearColor(0.17f, 0.19f, 0.21f, 1.0f), 4.0f));
    BarStyle.SetFillImage(FSlateRoundedBoxBrush(FLinearColor::White, 4.0f));

    for (int32 StatIndex = 0; StatIndex < UE_ARRAY_COUNT(StatLabels); ++StatIndex)
    {
        UHorizontalBox *StatRow = WidgetTree->ConstructWidget<UHorizontalBox>();
        Content->AddChildToVerticalBox(StatRow)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));

        UTextBlock *Label = MakeText(WidgetTree, StatLabels[StatIndex], 15, Muted);
        Label->SetMinDesiredWidth(65.0f); // 라벨 길이가 달라도 막대 시작 위치를 맞춤
        StatRow->AddChildToHorizontalBox(Label)->SetVerticalAlignment(VAlign_Center);

        // 프로그레스바는 두께 설정이 없어서 SizeBox로 높이 8 고정
        USizeBox *BarSize = WidgetTree->ConstructWidget<USizeBox>();
        BarSize->SetHeightOverride(8.0f);
        UHorizontalBoxSlot *BarSlot = StatRow->AddChildToHorizontalBox(BarSize);
        BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        BarSlot->SetVerticalAlignment(VAlign_Center);

        UProgressBar *Bar = WidgetTree->ConstructWidget<UProgressBar>();
        Bar->SetWidgetStyle(BarStyle);
        Bar->SetPercent(Info.Stats[StatIndex]);
        Bar->SetFillColorAndOpacity(Ink);
        BarSize->AddChild(Bar);
        StatBars.Add(Bar);
    }
}

void UWeaponCardWidget::SetHighlighted(bool bHighlighted)
{
    SetRenderScale(FVector2D(bHighlighted ? 1.05f : 1.0f));
    GlowImage->SetVisibility(bHighlighted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    for (UProgressBar *Bar : StatBars)
        Bar->SetFillColorAndOpacity(bHighlighted ? Amber : Ink);
}

void UWeaponCardWidget::HandleHovered()
{
    SetHighlighted(true);
}

void UWeaponCardWidget::HandleUnhovered()
{
    SetHighlighted(false);
}

void UWeaponCardWidget::HandleClicked()
{
    if (OwnerSelectWidget.IsValid())
        OwnerSelectWidget->SelectWeapon(WeaponIndex);
}

// ===================== 선택 화면 =====================

TSharedRef<SWidget> UWeaponSelectWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
        BuildWidgetTree();

    return Super::RebuildWidget();
}

void UWeaponSelectWidget::BuildWidgetTree()
{
    UCanvasPanel *Root = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Root;

    // 화면 전체 반투명 검은 배경
    UImage *Background = WidgetTree->ConstructWidget<UImage>();
    Background->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
    UCanvasPanelSlot *BackgroundSlot = Root->AddChildToCanvas(Background);
    BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    BackgroundSlot->SetOffsets(FMargin(0.0f));

    // 가운데: 제목 → 카드 3장 → 안내 문구
    UVerticalBox *Center = WidgetTree->ConstructWidget<UVerticalBox>();
    UCanvasPanelSlot *CenterSlot = Root->AddChildToCanvas(Center);
    CenterSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    CenterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    CenterSlot->SetAutoSize(true);

    Center->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("SUPPLY CRATE"), 16, Amber))->SetHorizontalAlignment(HAlign_Center);
    Center->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("주무기 선택"), 40, Ink, true))->SetHorizontalAlignment(HAlign_Center);

    UHorizontalBox *Cards = WidgetTree->ConstructWidget<UHorizontalBox>();
    UVerticalBoxSlot *CardsSlot = Center->AddChildToVerticalBox(Cards);
    CardsSlot->SetHorizontalAlignment(HAlign_Center);

    const TArray<TSubclassOf<APrimaryWeapon>> AvailableWeapons =
        SelectionBox.IsValid() ? SelectionBox->GetAvailablePrimaryWeapons() : TArray<TSubclassOf<APrimaryWeapon>>();

    for (int32 CardIndex = 0; CardIndex < UE_ARRAY_COUNT(WeaponCards); ++CardIndex)
    {
        UWeaponCardWidget *Card = WidgetTree->ConstructWidget<UWeaponCardWidget>(UWeaponCardWidget::StaticClass());
        Card->WeaponIndex = CardIndex;
        Card->OwnerSelectWidget = this;

        // 상자에 무기 BP가 아직 안 들어간 칸은 흐리게 + 클릭 불가
        const bool bAvailable = AvailableWeapons.IsValidIndex(CardIndex) && AvailableWeapons[CardIndex];
        Card->SetIsEnabled(bAvailable);
        Card->SetRenderOpacity(bAvailable ? 1.0f : 0.35f);

        Cards->AddChildToHorizontalBox(Card);
    }

    Center->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("카드를 클릭해서 선택 · 상자에서 F를 다시 누르면 바꿀 수 있어요"), 18, Muted))
        ->SetHorizontalAlignment(HAlign_Center);

    // 오른쪽 위 동그란 닫기 버튼 (안 고르고 나가기)
    UButton *CloseButton = WidgetTree->ConstructWidget<UButton>();
    CloseButton->SetStyle(MakeRoundedButtonStyle(30.0f));
    CloseButton->OnClicked.AddDynamic(this, &UWeaponSelectWidget::CloseWidget);
    UButtonSlot *CloseTextSlot = Cast<UButtonSlot>(CloseButton->AddChild(MakeText(WidgetTree, TEXT("X"), 24, Ink)));
    CloseTextSlot->SetHorizontalAlignment(HAlign_Center);
    CloseTextSlot->SetVerticalAlignment(VAlign_Center);
    UCanvasPanelSlot *CloseSlot = Root->AddChildToCanvas(CloseButton);
    CloseSlot->SetAnchors(FAnchors(1.0f, 0.0f));
    CloseSlot->SetAlignment(FVector2D(1.0f, 0.0f));
    CloseSlot->SetPosition(FVector2D(-40.0f, 40.0f));
    CloseSlot->SetSize(FVector2D(60.0f, 60.0f));
}

void UWeaponSelectWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 일시정지 메뉴와 같은 방식: 조작은 UI로만, 마우스 커서 표시
    if (APlayerController *PlayerController = GetOwningPlayer())
    {
        PlayerController->SetInputMode(FInputModeUIOnly());
        PlayerController->bShowMouseCursor = true;
    }
}

void UWeaponSelectWidget::SelectWeapon(int32 WeaponIndex)
{
    if (SelectionBox.IsValid())
        SelectionBox->SelectAndEquipWeapon(WeaponIndex, GetOwningPlayerPawn());

    CloseWidget();
}

void UWeaponSelectWidget::CloseWidget()
{
    if (APlayerController *PlayerController = GetOwningPlayer())
    {
        PlayerController->SetInputMode(FInputModeGameOnly());
        PlayerController->bShowMouseCursor = false;
    }

    RemoveFromParent();
}
