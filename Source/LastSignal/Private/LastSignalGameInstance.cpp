#include "LastSignalGameInstance.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"

void ULastSignalGameInstance::Init()
{
	Super::Init();

	// 화면 왼쪽 위 디버그 글자(BP의 Print String, AddOnScreenDebugMessage)를 게임 전체에서 끔
	// 디버깅할 때는 콘솔(~)에 EnableAllScreenMessages 입력하면 다시 보임
	GAreScreenMessagesEnabled = false;
}

void ULastSignalGameInstance::ResetSaveData()
{
	// 이 클래스에 선언된 UPROPERTY만 CDO(기본값)에서 복사 → 저장값이 추가돼도 여기 안 고쳐도 됨
	// 단, 이름이 Audio로 시작하는 것(재생 중인 음악/환경음)은 건너뜀
	const UObject *Defaults = GetClass()->GetDefaultObject();
	for (TFieldIterator<FProperty> It(ULastSignalGameInstance::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		if (It->GetName().StartsWith(TEXT("Audio")))
			continue;
		It->CopyCompleteValue_InContainer(this, Defaults);
	}
}

// ===================== 사운드 =====================

namespace
{
	// 채널 하나에 새 소리를 교차 재생: 이전 소리는 서서히 꺼지고, 새 소리는 서서히 커짐 (레벨이 바뀌어도 유지)
	UAudioComponent *CrossFadeTo(UObject *WorldContext, UAudioComponent *Current, USoundBase *Sound, float FadeSeconds, float Volume)
	{
		if (Current && Current->Sound == Sound && Current->IsPlaying())
			return Current; // 같은 곡이면 그대로

		if (Current)
		{
			Current->bAutoDestroy = true;
			Current->FadeOut(FadeSeconds, 0.0f);
		}

		UAudioComponent *Next = UGameplayStatics::CreateSound2D(WorldContext, Sound, Volume, 1.0f, 0.0f, nullptr,
																 /*bPersistAcrossLevelTransition*/ true, /*bAutoDestroy*/ false);
		if (Next)
			Next->FadeIn(FadeSeconds, 1.0f);
		return Next;
	}

	void FadeAndRelease(UAudioComponent *&Component, float FadeSeconds)
	{
		if (!Component)
			return;
		Component->bAutoDestroy = true;
		Component->FadeOut(FadeSeconds, 0.0f);
		Component = nullptr;
	}
}

USoundBase *ULastSignalGameInstance::LoadLoopingSound(const TCHAR *Path)
{
	USoundBase *Sound = LoadObject<USoundBase>(nullptr, Path);
	if (USoundWave *Wave = Cast<USoundWave>(Sound))
		Wave->bLooping = true; // mp3로 가져온 음악은 기본이 한 번 재생이라, 코드에서 반복으로
	return Sound;
}

void ULastSignalGameInstance::PlayMusic(USoundBase *Sound, float FadeSeconds, float Volume)
{
	if (!Sound || !GetWorld())
		return;
	UAudioComponent *Next = CrossFadeTo(GetWorld(), AudioMusicComponent, Sound, FadeSeconds, Volume);
	if (Next == AudioMusicComponent && Next)
		Next->AdjustVolume(FadeSeconds, Volume); // 같은 곡이면 볼륨만 맞춤 (예: 인트로 30% → 50%)
	AudioMusicComponent = Next;
}

void ULastSignalGameInstance::StopMusic(float FadeSeconds)
{
	UAudioComponent *Component = AudioMusicComponent.Get();
	FadeAndRelease(Component, FadeSeconds);
	AudioMusicComponent = nullptr;
}

void ULastSignalGameInstance::PlayAmbience(USoundBase *Sound, float Volume, float FadeAwaySeconds, float VolumeAfterFade)
{
	if (!Sound || !GetWorld())
		return;
	AudioAmbienceComponent = CrossFadeTo(GetWorld(), AudioAmbienceComponent, Sound, 1.0f, Volume);
	if (AudioAmbienceComponent && FadeAwaySeconds > 0.0f)
		AudioAmbienceComponent->AdjustVolume(FadeAwaySeconds, VolumeAfterFade); // 점점 멀어짐
}

void ULastSignalGameInstance::StopAmbience(float FadeSeconds)
{
	UAudioComponent *Component = AudioAmbienceComponent.Get();
	FadeAndRelease(Component, FadeSeconds);
	AudioAmbienceComponent = nullptr;
}
