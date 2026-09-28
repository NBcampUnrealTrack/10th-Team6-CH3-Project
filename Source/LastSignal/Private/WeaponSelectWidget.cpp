#include "WeaponSelectWidget.h"
#include "WeaponSelectionBox.h"
#include "PrimaryWeapon.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/BackgroundBlur.h"
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
        const TCHAR *Pros[2];     // 장점 2줄
        const TCHAR *Cons;        // 단점 1줄
        const TCHAR *IconPath;
        float Stats[4]; // 공격력, 연사, 사거리, 탄창 (0~1)
    };

    // ponytail: 스탯은 PrimaryFirearms.cpp 수치를 손으로 옮긴 값 — 밸런스를 바꾸면 여기도 같이 수정
    // 공격력 = 피해 × 산탄 수(25, 55, 90), 연사 = RPM(750, 300, 180), 사거리(10000, 20000, 4000), 탄창(30, 20, 6), 항목별 최댓값 = 1
    const FWeaponCardInfo WeaponCards[] = {
        {TEXT("M4A1"), TEXT("돌격소총"), TEXT("30 / 120"),
         {TEXT("빠른 연사 · 가장 빠른 장전"), TEXT("안정된 반동 · 넉넉한 탄창")},
         TEXT("낮은 한 발 피해"),
         TEXT("/Game/UI/HUD/T_WeaponCard_M4A1.T_WeaponCard_M4A1"), {0.28f, 1.0f, 0.5f, 1.0f}},
        {TEXT("SR25"), TEXT("지정사수"), TEXT("20 / 80"),
         {TEXT("강력한 단발 화력"), TEXT("가장 긴 사거리")},
         TEXT("느린 연사 · 낮은 조작성"),
         TEXT("/Game/UI/HUD/T_WeaponCard_SR25.T_WeaponCard_SR25"), {0.61f, 0.4f, 1.0f, 0.67f}},
        {TEXT("MP153"), TEXT("산탄총"), TEXT("6 / 36"),
         {TEXT("강력한 광역 공격"), TEXT("속사 시 폭발적인 화력")},
         TEXT("적은 탄창 · 낮은 전투 지속력"),
         TEXT("/Game/UI/HUD/T_WeaponCard_MP153.T_WeaponCard_MP153"), {1.0f, 0.24f, 0.2f, 0.2f}},
    };

    const TCHAR *StatLabels[] = {TEXT("공격력"), TEXT("연사"), TEXT("사거리"), TEXT("탄창")};

    const FLinearColor Amber(1.0f, 0.6f, 0.1f, 1.0f);
    const FLinearColor Ink(0.91f, 0.93f, 0.93f, 1.0f);
    const FLinearColor Muted(0.55f, 0.58f, 0.61f, 1.0f);
    // 불투명 — 반투명이면 호버 시 뒤의 주황빛이 카드에 비쳐서 사진 배경 경계가 드러남. 0.05(선형) = 사진 배경 회색(sRGB 63)
    const FLinearColor CardFill(0.05f, 0.05f, 0.05f, 1.0f);
    const FLinearColor CardOutline(0.3f, 0.33f, 0.36f, 1.0f);

    // 텍스처를 원하는 크기로 그리는 브러시 (UImage::SetDesiredSizeOverride는 화면에 뜨기 전에 부르면 무시돼서 브러시 크기로 지정)
    FSlateBrush MakeImageBrush(UTexture2D *Texture, const FVector2D &Size)
    {
        FSlateBrush Brush;
        Brush.SetResourceObject(Texture);
        Brush.SetImageSize(Size);
        return Brush;
    }

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
        // 배경색은 그대로 두고 테두리만 주황으로 — 배경색이 바뀌면 총 사진의 사각형 배경 경계가 드러남
        Style.SetHovered(FSlateRoundedBoxBrush(CardFill, Radius, Amber, 3.0f));
        Style.SetPressed(FSlateRoundedBoxBrush(CardFill, Radius, Amber, 3.0f));
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
        GlowImage->SetBrush(MakeImageBrush(GlowTexture, FVector2D(480.0f, 640.0f))); // 카드보다 사방 40 크게 → 카드 사이 간격 80
    GlowImage->SetVisibility(ESlateVisibility::Hidden);
    UOverlaySlot *GlowSlot = Root->AddChildToOverlay(GlowImage);
    GlowSlot->SetHorizontalAlignment(HAlign_Center);
    GlowSlot->SetVerticalAlignment(VAlign_Center);

    USizeBox *CardSize = WidgetTree->ConstructWidget<USizeBox>();
    CardSize->SetWidthOverride(400.0f);
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

    // 총 사진. 사진 배경(회색 63)이 카드 배경과 같은 색이라 칸 없이 바로 올려서 카드에 녹아들게 한다
    // 사진마다 비율이 달라서(M4A1 1965×913, SR25 1653×602 …) 350×160 칸을 비율 유지하며 최대로 채우고,
    // 칸 높이는 고정해서 세 카드의 이름/특징 줄 높이를 맞춘다
    USizeBox *IconSize = WidgetTree->ConstructWidget<USizeBox>();
    IconSize->SetHeightOverride(160.0f);
    UImage *Icon = WidgetTree->ConstructWidget<UImage>();
    if (UTexture2D *IconTexture = LoadObject<UTexture2D>(nullptr, Info.IconPath))
    {
        // GetSurfaceWidth는 에디터에서 텍스처 컴파일이 안 끝났으면 0이 나와서, 에셋에 저장된 원본 크기를 쓴다
        const FIntPoint ImportedSize = IconTexture->GetImportedSize();
        const float Aspect = ImportedSize.Y > 0 ? float(ImportedSize.X) / ImportedSize.Y : 2.5f;
        const float IconWidth = FMath::Min(350.0f, 160.0f * Aspect); // 카드 안쪽 너비 400 - 여백 25×2 = 350
        Icon->SetBrush(MakeImageBrush(IconTexture, FVector2D(IconWidth, IconWidth / Aspect)));
    }
    USizeBoxSlot *IconSlot = Cast<USizeBoxSlot>(IconSize->AddChild(Icon));
    IconSlot->SetHorizontalAlignment(HAlign_Center);
    IconSlot->SetVerticalAlignment(VAlign_Center);
    Content->AddChildToVerticalBox(IconSize)->SetPadding(FMargin(0.0f, 17.0f, 0.0f, 0.0f));

    // 이름
    UVerticalBoxSlot *NameSlot = Content->AddChildToVerticalBox(MakeText(WidgetTree, Info.Name, 28, Ink, true));
    NameSlot->SetHorizontalAlignment(HAlign_Center);
    NameSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));

    // 장점(+) 2줄, 단점(–) 1줄. 기호 칸 너비를 고정해서 글이 넘어가도 들여쓰기가 맞게
    auto AddTraitRow = [this, Content](const TCHAR *Symbol, const TCHAR *Trait, const FLinearColor &SymbolColor, const FLinearColor &TextColor, float TopPadding)
    {
        UHorizontalBox *Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        Content->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, TopPadding, 0.0f, 0.0f));

        UTextBlock *SymbolText = MakeText(WidgetTree, Symbol, 16, SymbolColor, true);
        SymbolText->SetMinDesiredWidth(20.0f);
        Row->AddChildToHorizontalBox(SymbolText);

        UTextBlock *TraitText = MakeText(WidgetTree, Trait, 16, TextColor);
        TraitText->SetAutoWrapText(true);
        Row->AddChildToHorizontalBox(TraitText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    };
    AddTraitRow(TEXT("+"), Info.Pros[0], Amber, Ink, 12.0f);
    AddTraitRow(TEXT("+"), Info.Pros[1], Amber, Ink, 4.0f);
    AddTraitRow(TEXT("–"), Info.Cons, FLinearColor(0.9f, 0.4f, 0.35f, 1.0f), Muted, 4.0f);

    // 특징 줄 수와 상관없이 스탯을 카드 맨 아래로 밀어냄
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

    // 뒤의 게임 화면을 흐리게 (블러 위에 반투명 검정을 한 겹 더 깐다)
    UBackgroundBlur *BackgroundBlur = WidgetTree->ConstructWidget<UBackgroundBlur>();
    BackgroundBlur->SetBlurStrength(16.0f);
    UCanvasPanelSlot *BlurSlot = Root->AddChildToCanvas(BackgroundBlur);
    BlurSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    BlurSlot->SetOffsets(FMargin(0.0f));

    // 화면 전체 반투명 검은 배경
    UImage *Background = WidgetTree->ConstructWidget<UImage>();
    Background->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
    UCanvasPanelSlot *BackgroundSlot = Root->AddChildToCanvas(Background);
    BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    BackgroundSlot->SetOffsets(FMargin(0.0f));

    // 가운데: 제목 → 카드 3장 → 안내 문구
    UVerticalBox *Center = WidgetTree->ConstructWidget<UVerticalBox>();
    UCanvasPanelSlot *CenterSlot = Root->AddChildToCanvas(Center);
    CenterSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    CenterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    CenterSlot->SetAutoSize(true);

    UTextBlock *SubTitle = MakeText(WidgetTree, TEXT("SUPPLY CRATE"), 16, Amber, true);
    FSlateFontInfo SubTitleFont = SubTitle->GetFont();
    SubTitleFont.LetterSpacing = 300; // 자간 (1/1000 em 단위)
    SubTitle->SetFont(SubTitleFont);
    Center->AddChildToVerticalBox(SubTitle)->SetHorizontalAlignment(HAlign_Center);
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

    Center->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("클릭해서 선택"), 18, Muted))
        ->SetHorizontalAlignment(HAlign_Center);

    // 오른쪽 위 닫기 버튼 (안 고르고 나가기): 반투명 유리 느낌의 둥근 사각형 + 큰 ×, 호버 시 주황 테두리
    UButton *CloseButton = WidgetTree->ConstructWidget<UButton>();
    FButtonStyle CloseStyle;
    CloseStyle.SetNormal(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.06f), 14.0f, FLinearColor(1.0f, 1.0f, 1.0f, 0.25f), 1.5f));
    CloseStyle.SetHovered(FSlateRoundedBoxBrush(FLinearColor(1.0f, 0.6f, 0.1f, 0.12f), 14.0f, Amber, 2.0f));
    CloseStyle.SetPressed(FSlateRoundedBoxBrush(FLinearColor(1.0f, 0.6f, 0.1f, 0.25f), 14.0f, Amber, 2.0f));
    CloseStyle.SetNormalPadding(FMargin(0.0f));
    CloseStyle.SetPressedPadding(FMargin(0.0f));
    CloseButton->SetStyle(CloseStyle);
    CloseButton->OnClicked.AddDynamic(this, &UWeaponSelectWidget::CloseWidget);
    UButtonSlot *CloseTextSlot = Cast<UButtonSlot>(CloseButton->AddChild(MakeText(WidgetTree, TEXT("×"), 36, Ink)));
    CloseTextSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f)); // ×가 글꼴 기준선 때문에 살짝 아래로 쏠려서 보정
    CloseTextSlot->SetHorizontalAlignment(HAlign_Center);
    CloseTextSlot->SetVerticalAlignment(VAlign_Center);
    UCanvasPanelSlot *CloseSlot = Root->AddChildToCanvas(CloseButton);
    CloseSlot->SetAnchors(FAnchors(1.0f, 0.0f));
    CloseSlot->SetAlignment(FVector2D(1.0f, 0.0f));
    CloseSlot->SetPosition(FVector2D(-48.0f, 48.0f));
    CloseSlot->SetSize(FVector2D(56.0f, 56.0f));
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
