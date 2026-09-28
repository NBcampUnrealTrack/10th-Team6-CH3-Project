#include "LastSignalGameInstance.h"

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
	const UObject *Defaults = GetClass()->GetDefaultObject();
	for (TFieldIterator<FProperty> It(ULastSignalGameInstance::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
		It->CopyCompleteValue_InContainer(this, Defaults);
}