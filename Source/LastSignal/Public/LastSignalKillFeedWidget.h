// LastSignalKillFeedWidget.h
// LAST SIGNAL - 킬확정 표시(킬로그) 위젯.
// "몬스터 처치 시 킬로그 형태로 노출 후 일정 시간 뒤 Fade-out" 요구사항을 구현.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LastSignalKillFeedWidget.generated.h"

class UTextBlock;
class UVerticalBox;

/** 킬로그 한 줄(항목) */
UCLASS(Abstract)
class LASTSIGNAL_API UKillFeedEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 텍스트 세팅 + LifeTime 이후 자동 Fade-out 예약
	UFUNCTION(BlueprintCallable, Category = "LastSignal|KillFeed")
	void SetupEntry(const FText& VictimName, float LifeTime);

protected:
	// WBP_KillFeedEntry 디자이너에서 이름이 같은 TextBlock과 자동 바인딩됨
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EntryText;

	// 블루프린트에서 Fade-out 위젯 애니메이션을 재생하고, 끝나면 RemoveFromParent 호출
	UFUNCTION(BlueprintImplementableEvent, Category = "LastSignal|KillFeed")
	void PlayFadeOutAndRemove();

private:
	void OnLifeTimeExpired();
	FTimerHandle FadeTimerHandle;
};

/** 킬로그 리스트 컨테이너 (좌상단 미션 목표 아래쪽) */
UCLASS(Abstract)
class LASTSIGNAL_API UKillFeedListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "LastSignal|KillFeed")
	void AddKillEntry(const FText& VictimName);

	UPROPERTY(EditDefaultsOnly, Category = "LastSignal|KillFeed")
	TSubclassOf<UKillFeedEntryWidget> EntryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "LastSignal|KillFeed")
	float EntryLifeTime = 3.f;

	// 화면에 동시에 보이는 최대 줄 수 (넘으면 제일 오래된 항목부터 제거)
	UPROPERTY(EditDefaultsOnly, Category = "LastSignal|KillFeed")
	int32 MaxVisibleEntries = 5;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> KillFeedBox;
};
