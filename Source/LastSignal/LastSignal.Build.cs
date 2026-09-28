// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LastSignal : ModuleRules
{
	public LastSignal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput","AIModule", "Niagara", "NavigationSystem","StateTreeModule","GameplayStateTreeModule", "UMG" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "MediaAssets" }); // Slate: 코드로 만든 UI(무기 선택, 메인메뉴)의 버튼/브러시 스타일, MediaAssets: 메인메뉴 배경 영상

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
