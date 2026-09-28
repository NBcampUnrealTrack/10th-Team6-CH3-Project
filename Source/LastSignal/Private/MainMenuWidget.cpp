#include "MainMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "LastSignalPlayerController.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "FileMediaSource.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include "Misc/Paths.h"

namespace
{
    const FLinearColor MenuAmber(1.0f, 0.6f, 0.1f, 1.0f);
    const FLinearColor MenuInk(0.92f, 0.92f, 0.92f, 1.0f);

    constexpr float VideoEndSeconds = 12.0f;  // 영상에서 카메라가 멈추는 시점 (MainMenuIntro.mp4 = 12초 + 마지막 장면 2초)
    constexpr float TitleFadeStart = 10.0f;   // 제목이 서서히 나타나기 시작하는 시점

    // Black Ops One이 없으면 기본 글꼴 Bold로 대신 쓴다
    UTextBlock *MakeMenuText(UWidgetTree *Tree, const FString &String, int32 Size, int32 LetterSpacing)
    {
        UTextBlock *Text = Tree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(String));
        FSlateFontInfo Font = Text->GetFont();
        if (UFont *BlackOps = LoadObject<UFont>(nullptr, TEXT("/Game/UI/Fonts/BlackOpsOne_Font.BlackOpsOne_Font")))
        {
            Font.FontObject = BlackOps;
            Font.TypefaceFontName = NAME_None;
        }
        else
        {
            Font.TypefaceFontName = TEXT("Bold");
        }
        Font.Size = Size;
        Font.LetterSpacing = LetterSpacing;
        Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(MenuInk));
        Text->SetShadowOffset(FVector2D(2.0f, 2.0f));
        Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
        FSlateFontInfo OutlinedFont = Text->GetFont(); // 밝은 창문 위에서도 읽히게 얇은 검은 테두리
        OutlinedFont.OutlineSettings.OutlineSize = 1;
        OutlinedFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
        Text->SetFont(OutlinedFont);
        Text->SetJustification(ETextJustify::Left);
        return Text;
    }
}

TSharedRef<SWidget> UMainMenuWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
        BuildWidgetTree();

    return Super::RebuildWidget();
}

void UMainMenuWidget::BuildWidgetTree()
{
    UCanvasPanel *Root = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Root;

    // 배경 영상 (화면 전체)
    MediaPlayer = NewObject<UMediaPlayer>(this);
    MediaPlayer->SetLooping(false);
    MediaTexture = NewObject<UMediaTexture>(this);
    MediaTexture->AutoClear = false; // 멈춘 뒤에도 마지막 장면 유지
    MediaTexture->SetMediaPlayer(MediaPlayer);
    MediaTexture->UpdateResource();

    UImage *Background = WidgetTree->ConstructWidget<UImage>();
    FSlateBrush VideoBrush;
    VideoBrush.SetResourceObject(MediaTexture);
    VideoBrush.SetImageSize(FVector2D(1920.0f, 1080.0f));
    Background->SetBrush(VideoBrush);
    UCanvasPanelSlot *BackgroundSlot = Root->AddChildToCanvas(Background);
    BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    BackgroundSlot->SetOffsets(FMargin(0.0f));

    // 왼쪽 절반에 검정 → 투명 그라데이션 (왼쪽 창문 빛 위에서도 흰 글자가 보이게). 텍스처는 코드에서 256×1로 생성
    TArray<uint8> ShadePixels;
    ShadePixels.SetNumZeroed(256 * 4);
    for (int32 X = 0; X < 256; ++X)
        ShadePixels[X * 4 + 3] = static_cast<uint8>(255.0f * 0.8f * FMath::Pow(1.0f - X / 255.0f, 1.6f)); // BGRA 중 알파만
    LeftShadeTexture = UTexture2D::CreateTransient(256, 1, PF_B8G8R8A8, NAME_None, ShadePixels);
    LeftShadeTexture->UpdateResource();
    LeftShade = WidgetTree->ConstructWidget<UImage>();
    LeftShade->SetBrushFromTexture(LeftShadeTexture);
    LeftShade->SetVisibility(ESlateVisibility::HitTestInvisible);
    LeftShade->SetRenderOpacity(0.0f);
    UCanvasPanelSlot *ShadeSlot = Root->AddChildToCanvas(LeftShade);
    ShadeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.6f, 1.0f));
    ShadeSlot->SetOffsets(FMargin(0.0f));

    // 왼쪽: 제목 + 버튼 (왼쪽 정렬)
    UVerticalBox *Menu = WidgetTree->ConstructWidget<UVerticalBox>();
    UCanvasPanelSlot *MenuSlot = Root->AddChildToCanvas(Menu);
    MenuSlot->SetAnchors(FAnchors(0.08f, 0.5f));
    MenuSlot->SetAlignment(FVector2D(0.0f, 0.5f));
    MenuSlot->SetAutoSize(true);

    UTextBlock *TitleText = MakeMenuText(WidgetTree, TEXT("LAST SIGNAL"), 96, 120);
    Title = TitleText;
    Menu->AddChildToVerticalBox(TitleText)->SetHorizontalAlignment(HAlign_Left);

    // 배경 없는 글자 버튼. 호버 시 주황
    FButtonStyle TextButtonStyle;
    TextButtonStyle.SetNormal(FSlateNoResource());
    TextButtonStyle.SetHovered(FSlateNoResource());
    TextButtonStyle.SetPressed(FSlateNoResource());
    TextButtonStyle.SetNormalPadding(FMargin(0.0f));
    TextButtonStyle.SetPressedPadding(FMargin(0.0f));

    UButton *Start = WidgetTree->ConstructWidget<UButton>();
    Start->SetStyle(TextButtonStyle);
    StartText = MakeMenuText(WidgetTree, TEXT("START GAME"), 40, 200);
    Start->AddChild(StartText);
    Start->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleStartClicked);
    Start->OnHovered.AddDynamic(this, &UMainMenuWidget::HandleStartHovered);
    Start->OnUnhovered.AddDynamic(this, &UMainMenuWidget::HandleStartUnhovered);
    UVerticalBoxSlot *StartSlot = Menu->AddChildToVerticalBox(Start);
    StartSlot->SetHorizontalAlignment(HAlign_Left);
    StartSlot->SetPadding(FMargin(0.0f, 60.0f, 0.0f, 0.0f));
    StartButton = Start;

    UButton *Quit = WidgetTree->ConstructWidget<UButton>();
    Quit->SetStyle(TextButtonStyle);
    QuitText = MakeMenuText(WidgetTree, TEXT("QUIT"), 40, 200);
    Quit->AddChild(QuitText);
    Quit->OnClicked.AddDynamic(this, &UMainMenuWidget::HandleQuitClicked);
    Quit->OnHovered.AddDynamic(this, &UMainMenuWidget::HandleQuitHovered);
    Quit->OnUnhovered.AddDynamic(this, &UMainMenuWidget::HandleQuitUnhovered);
    UVerticalBoxSlot *QuitSlot = Menu->AddChildToVerticalBox(Quit);
    QuitSlot->SetHorizontalAlignment(HAlign_Left);
    QuitSlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));
    QuitButton = Quit;

    // 처음엔 숨김 (Tick에서 투명도로 등장). 버튼은 나타나기 전엔 클릭 안 되게
    Title->SetRenderOpacity(0.0f);
    StartButton->SetRenderOpacity(0.0f);
    QuitButton->SetRenderOpacity(0.0f);
    StartButton->SetVisibility(ESlateVisibility::HitTestInvisible);
    QuitButton->SetVisibility(ESlateVisibility::HitTestInvisible);

    // 맨 위 검은 막: 검정에서 밝아지며 시작
    BlackFade = WidgetTree->ConstructWidget<UImage>();
    BlackFade->SetColorAndOpacity(FLinearColor::Black);
    BlackFade->SetVisibility(ESlateVisibility::HitTestInvisible);
    UCanvasPanelSlot *FadeSlot = Root->AddChildToCanvas(BlackFade);
    FadeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    FadeSlot->SetOffsets(FMargin(0.0f));
}

void UMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 영상은 에셋이 아니라 파일 경로로 연다 (패키징: DefaultGame.ini의 DirectoriesToAlwaysStageAsNonUFS=Movies)
    MediaSource = NewObject<UFileMediaSource>(this);
    MediaSource->SetFilePath(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Movies/MainMenuIntro.mp4")));
    MediaPlayer->OpenSource(MediaSource); // PlayOnOpen 기본값 true → 열리면 바로 재생

    // 키보드 입력(건너뛰기)을 받도록 포커스
    SetIsFocusable(true);
    if (APlayerController *PlayerController = GetOwningPlayer())
    {
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(TakeWidget());
        PlayerController->SetInputMode(InputMode);
        PlayerController->bShowMouseCursor = true;
    }
}

void UMainMenuWidget::NativeDestruct()
{
    if (MediaPlayer)
        MediaPlayer->Close();

    Super::NativeDestruct();
}

void UMainMenuWidget::NativeTick(const FGeometry &MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ElapsedTime += InDeltaTime;

    // 검정 → 밝게 (1.2초)
    BlackFade->SetRenderOpacity(FMath::Clamp(1.0f - ElapsedTime / 1.2f, 0.0f, 1.0f));

    // 영상 시간 기준으로 진행. 영상이 3초 안에 안 열리면(파일 없음 등) 바로 메뉴를 띄운다
    const bool bVideoRunning = MediaPlayer && (MediaPlayer->IsPlaying() || MediaPlayer->IsPaused());
    const float VideoTime = bVideoRunning ? static_cast<float>(MediaPlayer->GetTime().GetTotalSeconds()) : 0.0f;
    const bool bVideoFailed = !bVideoRunning && ElapsedTime > 3.0f;

    if (MediaPlayer && MediaPlayer->IsPlaying() && VideoTime >= PauseAtSeconds)
        MediaPlayer->Pause(); // 마지막 장면 정지 구간(12~14초) 안에서 멈춤

    // 제목: 10초부터 1.5초 동안 서서히
    const float TitleAlpha = bVideoFailed ? 1.0f : FMath::Clamp((VideoTime - TitleFadeStart) / 1.5f, 0.0f, 1.0f);
    Title->SetRenderOpacity(FMath::Max(Title->GetRenderOpacity(), TitleAlpha));
    LeftShade->SetRenderOpacity(Title->GetRenderOpacity()); // 그림자도 제목과 같이 깔림

    // 버튼: 영상이 멈추는 12초에 START → QUIT 순서로 떠오름
    if (MenuShownTime < 0.0f && (VideoTime >= VideoEndSeconds || bVideoFailed))
    {
        MenuShownTime = ElapsedTime;
        StartButton->SetVisibility(ESlateVisibility::Visible);
        QuitButton->SetVisibility(ESlateVisibility::Visible);
    }
    if (MenuShownTime >= 0.0f)
    {
        const float Since = ElapsedTime - MenuShownTime;
        StartButton->SetRenderOpacity(FMath::Clamp(Since / 0.5f, 0.0f, 1.0f));
        QuitButton->SetRenderOpacity(FMath::Clamp((Since - 0.2f) / 0.5f, 0.0f, 1.0f));
        StartButton->SetRenderTranslation(FVector2D(0.0f, 20.0f * (1.0f - StartButton->GetRenderOpacity())));
        QuitButton->SetRenderTranslation(FVector2D(0.0f, 20.0f * (1.0f - QuitButton->GetRenderOpacity())));
    }
}

void UMainMenuWidget::SkipIntro()
{
    if (MenuShownTime >= 0.0f)
        return;

    // 마지막 장면 구간으로 이동 → 계속 재생되다 PauseAtSeconds에서 멈춤 (바로 Pause하면 이전 프레임이 남을 수 있음)
    if (MediaPlayer && (MediaPlayer->IsPlaying() || MediaPlayer->IsPaused()))
    {
        MediaPlayer->Seek(FTimespan::FromSeconds(VideoEndSeconds));
        MediaPlayer->Play();
    }
    Title->SetRenderOpacity(1.0f);
    ElapsedTime = FMath::Max(ElapsedTime, 1.2f); // 검은 막도 바로 걷음
}

FReply UMainMenuWidget::NativeOnMouseButtonDown(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent)
{
    SkipIntro();
    return FReply::Handled();
}

FReply UMainMenuWidget::NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent)
{
    SkipIntro();
    return FReply::Handled();
}

void UMainMenuWidget::HandleStartClicked()
{
    if (APlayerController *PlayerController = GetOwningPlayer())
    {
        PlayerController->SetInputMode(FInputModeGameOnly());
        PlayerController->bShowMouseCursor = false;
    }
    UGameplayStatics::OpenLevel(this, TEXT("L_SafeZone_Spawn"));
}

void UMainMenuWidget::HandleQuitClicked()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UMainMenuWidget::HandleStartHovered()
{
    StartText->SetColorAndOpacity(FSlateColor(MenuAmber));
}

void UMainMenuWidget::HandleStartUnhovered()
{
    StartText->SetColorAndOpacity(FSlateColor(MenuInk));
}

void UMainMenuWidget::HandleQuitHovered()
{
    QuitText->SetColorAndOpacity(FSlateColor(MenuAmber));
}

void UMainMenuWidget::HandleQuitUnhovered()
{
    QuitText->SetColorAndOpacity(FSlateColor(MenuInk));
}

// ===================== 일시정지 =====================

TSharedRef<SWidget> UPauseMenuWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
        BuildWidgetTree();

    return Super::RebuildWidget();
}

void UPauseMenuWidget::BuildWidgetTree()
{
    UCanvasPanel *Root = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Root;

    // 멈춘 게임 화면을 흐리게 + 어둡게 (무기 선택 화면과 같은 방식)
    UBackgroundBlur *Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
    Blur->SetBlurStrength(12.0f);
    UCanvasPanelSlot *BlurSlot = Root->AddChildToCanvas(Blur);
    BlurSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    BlurSlot->SetOffsets(FMargin(0.0f));

    UImage *Dim = WidgetTree->ConstructWidget<UImage>();
    Dim->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
    UCanvasPanelSlot *DimSlot = Root->AddChildToCanvas(Dim);
    DimSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    DimSlot->SetOffsets(FMargin(0.0f));

    // 왼쪽: PAUSED + 버튼 (메인메뉴와 같은 위치/정렬)
    UVerticalBox *Menu = WidgetTree->ConstructWidget<UVerticalBox>();
    UCanvasPanelSlot *MenuSlot = Root->AddChildToCanvas(Menu);
    MenuSlot->SetAnchors(FAnchors(0.08f, 0.5f));
    MenuSlot->SetAlignment(FVector2D(0.0f, 0.5f));
    MenuSlot->SetAutoSize(true);

    // 메인메뉴와 같은 LAST SIGNAL 제목 + 아래 작은 주황 PAUSED
    Menu->AddChildToVerticalBox(MakeMenuText(WidgetTree, TEXT("LAST SIGNAL"), 96, 120))->SetHorizontalAlignment(HAlign_Left);
    UTextBlock *PausedLabel = MakeMenuText(WidgetTree, TEXT("PAUSED"), 20, 450);
    PausedLabel->SetColorAndOpacity(FSlateColor(MenuAmber));
    UVerticalBoxSlot *PausedSlot = Menu->AddChildToVerticalBox(PausedLabel);
    PausedSlot->SetHorizontalAlignment(HAlign_Left);
    PausedSlot->SetPadding(FMargin(4.0f, 10.0f, 0.0f, 0.0f));

    FButtonStyle TextButtonStyle;
    TextButtonStyle.SetNormal(FSlateNoResource());
    TextButtonStyle.SetHovered(FSlateNoResource());
    TextButtonStyle.SetPressed(FSlateNoResource());
    TextButtonStyle.SetNormalPadding(FMargin(0.0f));
    TextButtonStyle.SetPressedPadding(FMargin(0.0f));

    UButton *Resume = WidgetTree->ConstructWidget<UButton>();
    Resume->SetStyle(TextButtonStyle);
    ResumeText = MakeMenuText(WidgetTree, TEXT("RESUME"), 48, 200);
    Resume->AddChild(ResumeText);
    Resume->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleResumeClicked);
    Resume->OnHovered.AddDynamic(this, &UPauseMenuWidget::HandleResumeHovered);
    Resume->OnUnhovered.AddDynamic(this, &UPauseMenuWidget::HandleResumeUnhovered);
    UVerticalBoxSlot *ResumeSlot = Menu->AddChildToVerticalBox(Resume);
    ResumeSlot->SetHorizontalAlignment(HAlign_Left);
    ResumeSlot->SetPadding(FMargin(0.0f, 50.0f, 0.0f, 0.0f));

    UButton *MainMenu = WidgetTree->ConstructWidget<UButton>();
    MainMenu->SetStyle(TextButtonStyle);
    MainMenuText = MakeMenuText(WidgetTree, TEXT("MAIN MENU"), 48, 200);
    MainMenu->AddChild(MainMenuText);
    MainMenu->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleMainMenuClicked);
    MainMenu->OnHovered.AddDynamic(this, &UPauseMenuWidget::HandleMainMenuHovered);
    MainMenu->OnUnhovered.AddDynamic(this, &UPauseMenuWidget::HandleMainMenuUnhovered);
    UVerticalBoxSlot *MainMenuSlot = Menu->AddChildToVerticalBox(MainMenu);
    MainMenuSlot->SetHorizontalAlignment(HAlign_Left);
    MainMenuSlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));

    SetIsFocusable(true); // P/ESC 키 입력을 받기 위해 (PlayerController가 이 위젯에 포커스를 줌)
}

FReply UPauseMenuWidget::NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent)
{
    // UIOnly 상태라 게임 입력(IA_Pause)이 안 들어오므로 여기서 직접 P/ESC를 받는다
    if (InKeyEvent.GetKey() == EKeys::P || InKeyEvent.GetKey() == EKeys::Escape)
    {
        HandleResumeClicked();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPauseMenuWidget::HandleResumeClicked()
{
    if (ALastSignalPlayerController *PlayerController = Cast<ALastSignalPlayerController>(GetOwningPlayer()))
        PlayerController->TogglePauseMenu(); // 일시정지 해제 + 이 위젯 제거 + 게임 입력 복구
}

void UPauseMenuWidget::HandleMainMenuClicked()
{
    UGameplayStatics::OpenLevel(this, TEXT("L_MainMenu"));
}

void UPauseMenuWidget::HandleResumeHovered()
{
    ResumeText->SetColorAndOpacity(FSlateColor(MenuAmber));
}

void UPauseMenuWidget::HandleResumeUnhovered()
{
    ResumeText->SetColorAndOpacity(FSlateColor(MenuInk));
}

void UPauseMenuWidget::HandleMainMenuHovered()
{
    MainMenuText->SetColorAndOpacity(FSlateColor(MenuAmber));
}

void UPauseMenuWidget::HandleMainMenuUnhovered()
{
    MainMenuText->SetColorAndOpacity(FSlateColor(MenuInk));
}

// ===================== 엔딩 크레딧 =====================

namespace
{
    constexpr float StatsFadeIn = 1.0f;
    constexpr float StatsHoldUntil = 5.0f;
    constexpr float StatsFadeOutEnd = 6.0f;

    // 한글(팀원 이름)은 Black Ops One에 없어서 Pretendard로
    UTextBlock *MakeBodyText(UWidgetTree *Tree, const FString &String, int32 Size, const FLinearColor &Color)
    {
        UTextBlock *Text = Tree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(String));
        FSlateFontInfo Font = Text->GetFont();
        if (UFont *Pretendard = LoadObject<UFont>(nullptr, TEXT("/Game/UI/Fonts/Pretendard_Bold_Font.Pretendard_Bold_Font")))
        {
            Font.FontObject = Pretendard;
            Font.TypefaceFontName = NAME_None;
        }
        Font.Size = Size;
        Text->SetFont(Font);
        Text->SetColorAndOpacity(FSlateColor(Color));
        Text->SetJustification(ETextJustify::Center);
        return Text;
    }

    struct FCreditMember
    {
        const TCHAR *Name;
        const TCHAR *Role;
    };

    const FCreditMember TeamMembers[] = {
        {TEXT("한누리"), TEXT("Team Lead · Game Systems · UI · Cinematics & Narration · Character")},
        {TEXT("이영빈"), TEXT("Sub Lead · Weapons & Gunplay · Combat Animation · VFX")},
        {TEXT("이승현"), TEXT("Zombie AI · Environment Art · Level Design")},
        {TEXT("이원창"), TEXT("Player Skills · Combat · Items · Audio")},
        {TEXT("곽성은"), TEXT("Lead Level Design · Level Layout · Level Building · VFX")},
        {TEXT("신나린"), TEXT("HUD")},
    };

    const TCHAR *AssetLines[] = {
        TEXT("M4 / M4A1 Carbine – Modular AR-15 Tactical Rifle"),
        TEXT("SR-25 / M110 Designated Marksman Rifle – Fully Modular"),
        TEXT("Semi-auto Shotgun Black Lake 153 – Polymer"),
        TEXT("Game-Ready Modular Rigged SWAT Lowpoly 3D Model"),
        TEXT("Zombies – 6 Characters, All Animations"),
        TEXT("Modern Pistols SFX · Modern Assault Rifles SFX · Modern Shotguns SFX"),
        TEXT("Derelict Corridor (Megascans) · Subway Train · Metro Maintenance Station"),
        TEXT("Hospital Corridor · Abandoned Power Plant · Warehouse Storage"),
        TEXT("Scene Warehouse · Scene Unfinished Building · Soul: City"),
        TEXT("Zombie Female · Zombie Animation Pack · Dead Bodies Poses"),
        TEXT("First Person Rifle Animations · M1911 · Trace VFX · Footprints FX"),
        TEXT("Realistic Starter VFX Pack Vol.2 · Film & VHS Filters · Muzzle Flash"),
        TEXT("HeartBeats · Essential Footsteps · Free Game Sounds Vol.1 · ORPH Tools"),
    };
}

TSharedRef<SWidget> UCreditsWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
        BuildWidgetTree();

    return Super::RebuildWidget();
}

void UCreditsWidget::BuildWidgetTree()
{
    UCanvasPanel *Root = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Root;

    UImage *Black = WidgetTree->ConstructWidget<UImage>();
    Black->SetColorAndOpacity(FLinearColor::Black);
    UCanvasPanelSlot *BlackSlot = Root->AddChildToCanvas(Black);
    BlackSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    BlackSlot->SetOffsets(FMargin(0.0f));

    // ① 기록: 가운데에 라벨(주황, 작게) + 값(크게)
    UVerticalBox *Stats = WidgetTree->ConstructWidget<UVerticalBox>();
    UCanvasPanelSlot *StatsSlot = Root->AddChildToCanvas(Stats);
    StatsSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    StatsSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    StatsSlot->SetAutoSize(true);

    auto AddStat = [&](const TCHAR *Label, const FString &Value, float TopPadding)
    {
        UTextBlock *LabelText = MakeMenuText(WidgetTree, Label, 22, 450);
        LabelText->SetColorAndOpacity(FSlateColor(MenuAmber));
        LabelText->SetJustification(ETextJustify::Center);
        UVerticalBoxSlot *LabelSlot = Stats->AddChildToVerticalBox(LabelText);
        LabelSlot->SetHorizontalAlignment(HAlign_Center);
        LabelSlot->SetPadding(FMargin(0.0f, TopPadding, 0.0f, 0.0f));

        UTextBlock *ValueText = MakeMenuText(WidgetTree, Value, 80, 100);
        ValueText->SetJustification(ETextJustify::Center);
        UVerticalBoxSlot *ValueSlot = Stats->AddChildToVerticalBox(ValueText);
        ValueSlot->SetHorizontalAlignment(HAlign_Center);
        ValueSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
    };
    const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(PlayTimeSeconds));
    AddStat(TEXT("PLAY TIME"), FString::Printf(TEXT("%02d:%02d"), TotalSeconds / 60, TotalSeconds % 60), 0.0f);
    AddStat(TEXT("ZOMBIES KILLED"), FString::FromInt(KillCount), 60.0f);
    Stats->SetRenderOpacity(0.0f);
    StatsPanel = Stats;

    // ② 크레딧: 화면 아래 끝에서 시작해서 위로 올라감
    UVerticalBox *Scroll = WidgetTree->ConstructWidget<UVerticalBox>();
    UCanvasPanelSlot *ScrollSlot = Root->AddChildToCanvas(Scroll);
    ScrollSlot->SetAnchors(FAnchors(0.5f, 1.0f));
    ScrollSlot->SetAlignment(FVector2D(0.5f, 0.0f));
    ScrollSlot->SetAutoSize(true);

    auto AddLine = [&](UTextBlock *Text, float TopPadding)
    {
        UVerticalBoxSlot *LineSlot = Scroll->AddChildToVerticalBox(Text);
        LineSlot->SetHorizontalAlignment(HAlign_Center);
        LineSlot->SetPadding(FMargin(0.0f, TopPadding, 0.0f, 0.0f));
    };
    auto AddHeader = [&](const TCHAR *Header)
    {
        UTextBlock *HeaderText = MakeMenuText(WidgetTree, Header, 22, 450);
        HeaderText->SetColorAndOpacity(FSlateColor(MenuAmber));
        HeaderText->SetJustification(ETextJustify::Center);
        AddLine(HeaderText, 120.0f);
    };
    const FLinearColor Soft(0.7f, 0.72f, 0.74f, 1.0f);

    UTextBlock *Logo = MakeMenuText(WidgetTree, TEXT("LAST SIGNAL"), 96, 120);
    Logo->SetJustification(ETextJustify::Center);
    AddLine(Logo, 0.0f);

    AddHeader(TEXT("DEVELOPED BY"));
    for (const FCreditMember &Member : TeamMembers)
    {
        AddLine(MakeBodyText(WidgetTree, Member.Name, 34, MenuInk), 44.0f);
        AddLine(MakeBodyText(WidgetTree, Member.Role, 20, Soft), 6.0f);
    }

    AddHeader(TEXT("VOICE"));
    AddLine(MakeBodyText(WidgetTree, TEXT("Narration generated with ElevenLabs"), 24, MenuInk), 36.0f);
    AddLine(MakeBodyText(WidgetTree, TEXT("elevenlabs.io"), 20, Soft), 6.0f);

    AddHeader(TEXT("ASSETS VIA FAB"));
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(AssetLines); ++Index)
        AddLine(MakeBodyText(WidgetTree, AssetLines[Index], 20, Soft), Index == 0 ? 36.0f : 12.0f);

    AddHeader(TEXT("ADDITIONAL"));
    AddLine(MakeBodyText(WidgetTree, TEXT("Vintage Radio Transceiver – Poly Haven (CC0)"), 20, Soft), 36.0f);
    AddLine(MakeBodyText(WidgetTree, TEXT("Fonts: Pretendard · Black Ops One · DSEG7 (SIL Open Font License)"), 20, Soft), 12.0f);
    AddLine(MakeBodyText(WidgetTree, TEXT("Made with Unreal Engine 5"), 20, Soft), 12.0f);

    UTextBlock *Thanks = MakeMenuText(WidgetTree, TEXT("THANK YOU FOR PLAYING"), 44, 200);
    Thanks->SetJustification(ETextJustify::Center);
    AddLine(Thanks, 200.0f);

    Scroll->SetVisibility(ESlateVisibility::HitTestInvisible);
    ScrollPanel = Scroll;
}

void UCreditsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetIsFocusable(true);
    if (APlayerController *PlayerController = GetOwningPlayer())
    {
        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(TakeWidget());
        PlayerController->SetInputMode(InputMode);
        PlayerController->bShowMouseCursor = false; // 영화 크레딧처럼 커서 숨김 (클릭/키로 넘기기는 됨)
    }
}

void UCreditsWidget::NativeTick(const FGeometry &MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ElapsedTime += InDeltaTime;

    if (!bScrolling)
    {
        // ① 기록: 1초 동안 나타남 → 5초까지 유지 → 6초까지 사라짐
        const float FadeIn = FMath::Clamp(ElapsedTime / StatsFadeIn, 0.0f, 1.0f);
        const float FadeOut = FMath::Clamp((StatsFadeOutEnd - ElapsedTime) / (StatsFadeOutEnd - StatsHoldUntil), 0.0f, 1.0f);
        StatsPanel->SetRenderOpacity(FMath::Min(FadeIn, FadeOut));
        if (ElapsedTime >= StatsFadeOutEnd)
            bScrolling = true;
        return;
    }

    // ② 크레딧: 화면 아래에서 위로. 전부 화면 위로 빠져나가면 메인메뉴
    ScrollOffset += ScrollSpeed * InDeltaTime;
    ScrollPanel->SetRenderTranslation(FVector2D(0.0f, -ScrollOffset));

    const float ScreenHeight = MyGeometry.GetLocalSize().Y;
    const float ContentHeight = ScrollPanel->GetDesiredSize().Y;
    if (ScreenHeight > 0.0f && ContentHeight > 0.0f && ScrollOffset > ScreenHeight + ContentHeight)
        GoToMainMenu();
}

void UCreditsWidget::HandleSkipInput()
{
    if (!bScrolling)
    {
        bScrolling = true; // 기록 → 바로 크레딧
        StatsPanel->SetRenderOpacity(0.0f);
    }
    else if (ScrollSpeed < 100.0f)
    {
        ScrollSpeed *= 6.0f; // 빨리 감기
    }
    else
    {
        GoToMainMenu();
    }
}

FReply UCreditsWidget::NativeOnMouseButtonDown(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent)
{
    HandleSkipInput();
    return FReply::Handled();
}

FReply UCreditsWidget::NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent)
{
    HandleSkipInput();
    return FReply::Handled();
}

void UCreditsWidget::GoToMainMenu()
{
    if (bLeaving)
        return;
    bLeaving = true;
    UGameplayStatics::OpenLevel(this, TEXT("L_MainMenu"));
}
