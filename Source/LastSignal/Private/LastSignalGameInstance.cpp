#include "LastSignalGameInstance.h"

void ULastSignalGameInstance::Init()
{
	Super::Init();
}

void ULastSignalGameInstance::ResetSaveData()
{
	// 이 클래스에 선언된 UPROPERTY만 CDO(기본값)에서 복사 → 저장값이 추가돼도 여기 안 고쳐도 됨
	const UObject *Defaults = GetClass()->GetDefaultObject();
	for (TFieldIterator<FProperty> It(ULastSignalGameInstance::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
		It->CopyCompleteValue_InContainer(this, Defaults);
}