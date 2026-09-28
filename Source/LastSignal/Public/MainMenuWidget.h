// MainMenuWidget.h
// LAST SIGNAL - 메인메뉴. 배경 영상(Content/Movies/MainMenuIntro.mp4)을 한 번 재생하고 마지막 장면에서 멈춘 뒤
// LAST SIGNAL 제목과 START GAME / QUIT 버튼을 띄운다. 클릭이나 키 입력으로 영상을 건너뛸 수 있다. (WBP 없이 코드로 구성)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UFileMediaSource;
class UImage;
class UMediaPlayer;
class UMediaTexture;
class UTextBlock;
class UTexture2D;
class UWidget;

UCLASS()
class LASTSIGNAL_API UMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry &MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) override;
    virtual FReply NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent) override;

private:
    void BuildWidgetTree();
    void SkipIntro();

    UFUNCTION()
    void HandleStartClicked();

    UFUNCTION()
    void HandleQuitClicked();

    UFUNCTION()
    void HandleStartHovered();

    UFUNCTION()
    void HandleStartUnhovered();

    UFUNCTION()
    void HandleQuitHovered();

    UFUNCTION()
    void HandleQuitUnhovered();

    UPROPERTY()
    TObjectPtr<UMediaPlayer> MediaPlayer;

    UPROPERTY()
    TObjectPtr<UFileMediaSource> MediaSource;

    UPROPERTY()
    TObjectPtr<UMediaTexture> MediaTexture;

    UPROPERTY()
    TObjectPtr<UImage> BlackFade;

    // 왼쪽 글자 뒤 그림자 (검정 → 투명 그라데이션, 제목과 함께 나타남)
    UPROPERTY()
    TObjectPtr<UImage> LeftShade;

    UPROPERTY()
    TObjectPtr<UTexture2D> LeftShadeTexture;

    UPROPERTY()
    TObjectPtr<UWidget> Title;

    UPROPERTY()
    TObjectPtr<UWidget> StartButton;

    UPROPERTY()
    TObjectPtr<UWidget> QuitButton;

    UPROPERTY()
    TObjectPtr<UTextBlock> StartText;

    UPROPERTY()
    TObjectPtr<UTextBlock> QuitText;

    float ElapsedTime = 0.0f; // 메뉴가 뜬 뒤 흐른 시간 (영상이 안 열렸을 때의 대비용)
    float MenuShownTime = -1.0f; // 버튼이 나타나기 시작한 시각 (-1 = 아직)
    float PauseAtSeconds = 12.3f; // 이 시점에 영상을 멈춤 (12~14초는 마지막 장면 정지 구간)
};

// 일시정지 화면. 메인메뉴와 같은 스타일(Black Ops One, 왼쪽 정렬, 호버 시 주황) + 게임 화면 블러
// P 또는 ESC로도 계속하기. PlayerController::TogglePauseMenu가 생성/제거한다.
UCLASS()
class LASTSIGNAL_API UPauseMenuWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual FReply NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent) override;

private:
    void BuildWidgetTree();

    UFUNCTION()
    void HandleResumeClicked();

    UFUNCTION()
    void HandleMainMenuClicked();

    UFUNCTION()
    void HandleResumeHovered();

    UFUNCTION()
    void HandleResumeUnhovered();

    UFUNCTION()
    void HandleMainMenuHovered();

    UFUNCTION()
    void HandleMainMenuUnhovered();

    UPROPERTY()
    TObjectPtr<UTextBlock> ResumeText;

    UPROPERTY()
    TObjectPtr<UTextBlock> MainMenuText;
};
