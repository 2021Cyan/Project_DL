#pragma once

#include "CoreMinimal.h"
#include "Zipline/ZiplineTypes.h"

class ACharacter;
class AZiplineActor;

struct TEAMPROJECT_DL_API FZiplineQueryView
{
	static FZiplineQueryView FromCharacter(const ACharacter& Character);

	FVector EyeLocation = FVector::ZeroVector;
	FVector ViewDirection = FVector::ForwardVector;
};

class TEAMPROJECT_DL_API FZiplineInteractionLibrary
{
public:
	static bool BuildInteractionResult(
		const ACharacter* Character,
		AZiplineActor* Zipline,
		FZiplineInteractionResult& OutResult
	);

	static bool BuildInteractionResult(const ACharacter* Character, AZiplineActor* Zipline,
		const FZiplineQueryView& View, FZiplineInteractionResult& OutResult);

	static bool FindBestZipline(
		const ACharacter* Character,
		FZiplineInteractionResult& OutResult
	);

	static AZiplineActor* FindBestZipline(const ACharacter* Character);
};
