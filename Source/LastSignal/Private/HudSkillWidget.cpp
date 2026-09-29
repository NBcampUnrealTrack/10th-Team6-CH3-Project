#include "HudSkillWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
    constexpr float EmptyOpacity = 0.25f;    // 아직 안 찬 부분
    constexpr float ChargingOpacity = 0.6f;  // 차오른 부분 (아직 사용 불가)
    constexpr float ReadyOpacity = 1.0f;     // 다 참 = 사용 가능 (진하게)
}

bool UHudSkillWidget::TakeOverFromHUD(UUserWidget *HUDWidget)
{
    if (!HUDWidget || !HUDWidget->WidgetTree)
        return false;

    UImage *Banner = Cast<UImage>(HUDWidget->GetWidgetFromName(TEXT("Image_GaugeReady")));
    UCanvasPanelSlot *BannerSlot = Banner ? Cast<UCanvasPanelSlot>(Banner->Slot) : nullptr;
    // 같은 좌표로 옮기려면 배너가 HUD 최상위 캔버스(화면 전체)에 바로 있어야 함
    // (앵커가 늘어나는 방식이면 Offsets가 크기가 아니라 여백이라 크기를 알 수 없음 → 역시 원래 그대로)
    if (!BannerSlot || Banner->GetParent() != HUDWidget->WidgetTree->RootWidget || BannerSlot->GetAnchors().IsStretchedHorizontal() || BannerSlot->GetAnchors().IsStretchedVertical())
    {
        UE_LOG(LogTemp, Warning, TEXT("[HudSkillWidget] Image_GaugeReady가 최상위 캔버스에 없어서 원래 게이지 유지"));
        return false;
    }

    BannerBrush = Banner->GetBrush();
    BannerLayout = BannerSlot->GetLayout();
    BannerSize = BannerSlot->GetAutoSize() ? FVector2D(BannerBrush.GetImageSize())
                                           : FVector2D(BannerLayout.Offsets.Right, BannerLayout.Offsets.Bottom);

    // 원래 배너(전체가 서서히 밝아지던 것)와 PRESS 글자는 HUD에서 빼 버림 → BP가 계속 건드려도 화면엔 안 나옴
    Banner->RemoveFromParent();
    if (UWidget *OldPressText = HUDWidget->GetWidgetFromName(TEXT("Text_SkillReady")))
        OldPressText->RemoveFromParent();
    return true;
}

TSharedRef<SWidget> UHudSkillWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
        BuildWidgetTree();

    return Super::RebuildWidget();
}

void UHudSkillWidget::BuildWidgetTree()
{
    UCanvasPanel *Root = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Root;

    // 원래 배너와 같은 앵커/위치/크기
    USizeBox *Banner = WidgetTree->ConstructWidget<USizeBox>();
    Banner->SetWidthOverride(BannerSize.X);
    Banner->SetHeightOverride(BannerSize.Y);
    UCanvasPanelSlot *BannerSlot = Root->AddChildToCanvas(Banner);
    BannerSlot->SetLayout(BannerLayout);
    BannerSlot->SetAutoSize(true);

    UOverlay *Layers = WidgetTree->ConstructWidget<UOverlay>();
    Banner->SetContent(Layers);

    // 흐린 전체 배너
    UImage *EmptyImage = WidgetTree->ConstructWidget<UImage>();
    EmptyImage->SetBrush(BannerBrush);
    EmptyImage->SetRenderOpacity(EmptyOpacity);
    Layers->AddChildToOverlay(EmptyImage);

    // 게이지만큼만 보이는 배너: 폭이 줄어드는 상자 안에 원래 크기 그림을 왼쪽에 붙여 둠 → 오른쪽이 잘림
    FillClip = WidgetTree->ConstructWidget<USizeBox>();
    FillClip->SetWidthOverride(0.0f);
    FillClip->SetClipping(EWidgetClipping::ClipToBounds);
    UOverlaySlot *ClipSlot = Layers->AddChildToOverlay(FillClip);
    ClipSlot->SetHorizontalAlignment(HAlign_Left);
    ClipSlot->SetVerticalAlignment(VAlign_Fill);

    UCanvasPanel *FillCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    FillClip->SetContent(FillCanvas);
    FillImage = WidgetTree->ConstructWidget<UImage>();
    FillImage->SetBrush(BannerBrush);
    UCanvasPanelSlot *FillSlot = FillCanvas->AddChildToCanvas(FillImage);
    FillSlot->SetAnchors(FAnchors(0.0f, 0.0f));
    FillSlot->SetPosition(FVector2D::ZeroVector);
    FillSlot->SetSize(BannerSize);

    // 다 차면 배너 바로 위 가운데에 작게 "PRESS Q" (체력과 같은 Black Ops One)
    PressText = WidgetTree->ConstructWidget<UTextBlock>();
    PressText->SetText(FText::FromString(TEXT("PRESS Q")));
    FSlateFontInfo Font = PressText->GetFont();
    if (UFont *BlackOpsOne = LoadObject<UFont>(nullptr, TEXT("/Game/UI/Fonts/BlackOpsOne_Font.BlackOpsOne_Font")))
    {
        Font.FontObject = BlackOpsOne;
        Font.TypefaceFontName = NAME_None;
    }
    Font.Size = 14;
    Font.LetterSpacing = 150;
    Font.OutlineSettings.OutlineSize = 1;
    Font.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
    PressText->SetFont(Font);
    PressText->SetColorAndOpacity(FSlateColor(FLinearColor(FColor::FromHex(TEXT("F2C230"))))); // 노랑 (체력 노랑과 같은 색)
    UCanvasPanelSlot *PressSlot = Root->AddChildToCanvas(PressText);
    PressSlot->SetAnchors(BannerLayout.Anchors);
    PressSlot->SetAlignment(FVector2D(0.5f, 1.0f));
    PressSlot->SetAutoSize(true);
    // 배너 왼쪽 위 = 앵커 기준 위치 - 정렬 × 크기
    // Skill_image(2089x753) 실측: 빨간 "SPECIAL SKILL" 글자 띠는 가로 40%~97%(가운데 68.6%), 위쪽 끝이 높이의 23% (그 위는 투명 여백)
    // → 띠 가운데보다 살짝 왼쪽(62%, 배너 전체 기준으로 가운데 느낌), 띠 위쪽 끝에서 2px 위에 붙임
    const FVector2D BannerTopLeft(BannerLayout.Offsets.Left - BannerLayout.Alignment.X * BannerSize.X,
                                  BannerLayout.Offsets.Top - BannerLayout.Alignment.Y * BannerSize.Y);
    PressSlot->SetPosition(BannerTopLeft + FVector2D(BannerSize.X * 0.62f, BannerSize.Y * 0.23f - 2.0f));

    SetVisibility(ESlateVisibility::HitTestInvisible); // 클릭을 가로채지 않게
    Refresh();
}

void UHudSkillWidget::NativeTick(const FGeometry &MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // PRESS Q 깜빡임 (1초 주기, 완전히 사라지진 않게 15%까지만)
    if (PressText && bReady)
        PressText->SetRenderOpacity(0.575f + 0.425f * FMath::Cos(GetWorld()->GetTimeSeconds() * 2.0f * PI));

    if (BoundSkill.IsValid())
        return;

    // 캐릭터의 스킬 컴포넌트를 찾아 연결 (한 번만)
    const APlayerController *PlayerController = GetOwningPlayer();
    const APawn *Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    if (USkillComponent *Skill = Pawn ? Pawn->FindComponentByClass<USkillComponent>() : nullptr)
    {
        BoundSkill = Skill;
        Skill->OnSkillValueChanged.AddDynamic(this, &UHudSkillWidget::HandleSkillValueChanged);
        Skill->OnSkillStateChanged.AddDynamic(this, &UHudSkillWidget::HandleSkillStateChanged);

        // 연결 전에 이미 다 차 있던 경우(RETRY/다음 레벨에서 게이지 100을 이어받음) → 스킬은 더 이상 신호를 안 보내서
        // 여기서 현재 상태를 직접 읽음. 안 하면 게이지가 빈 채로 멈춰 보임
        if (Skill->CanActivateSkill())
        {
            bReady = true;
            Percent = 1.0f;
            Refresh();
        }
    }
}

void UHudSkillWidget::HandleSkillValueChanged(float CurrentValue, float MaxValue)
{
    Percent = MaxValue > 0.0f ? FMath::Clamp(CurrentValue / MaxValue, 0.0f, 1.0f) : 0.0f;
    Refresh();
}

void UHudSkillWidget::HandleSkillStateChanged(ESkillState NewState)
{
    bReady = NewState == ESkillState::Ready;
    if (bReady)
        Percent = 1.0f;
    Refresh();
}

void UHudSkillWidget::Refresh()
{
    if (!FillClip)
        return;

    FillClip->SetWidthOverride(BannerSize.X * Percent);
    FillImage->SetRenderOpacity(bReady ? ReadyOpacity : ChargingOpacity);
    PressText->SetVisibility(bReady ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}
