// LastSignalKillFeedWidget.cpp

#include "LastSignalKillFeedWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "TimerManager.h"

void UKillFeedEntryWidget::SetupEntry(const FText& VictimName, float LifeTime)
{
	if (EntryText)
	{
		// 예: "적 처치: 인간 좀비"
		EntryText->SetText(FText::Format(NSLOCTEXT("LastSignal", "KillFeedFormat", "적 처치: {0}"), VictimName));
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FadeTimerHandle, this, &UKillFeedEntryWidget::OnLifeTimeExpired, LifeTime, false);
	}
}

void UKillFeedEntryWidget::OnLifeTimeExpired()
{
	// 실제 페이드아웃 애니메이션 재생은 WBP_KillFeedEntry 이벤트그래프에서 구현
	PlayFadeOutAndRemove();
}

void UKillFeedListWidget::AddKillEntry(const FText& VictimName)
{
	if (!EntryWidgetClass || !KillFeedBox)
	{
		return;
	}

	UKillFeedEntryWidget* NewEntry = CreateWidget<UKillFeedEntryWidget>(this, EntryWidgetClass);
	if (!NewEntry)
	{
		return;
	}

	NewEntry->SetupEntry(VictimName, EntryLifeTime);
	KillFeedBox->AddChildToVerticalBox(NewEntry);

	// 최대 줄 수를 넘으면 가장 오래된(위쪽) 항목부터 제거
	while (KillFeedBox->GetChildrenCount() > MaxVisibleEntries)
	{
		KillFeedBox->RemoveChildAt(0);
	}
}
