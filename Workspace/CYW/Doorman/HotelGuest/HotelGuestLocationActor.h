#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HotelGuestLocationActor.generated.h"

class UArrowComponent;
class USceneComponent;

UCLASS()
class TEAMPROJECT_DL_API AHotelGuestLocationActor : public AActor
{
	GENERATED_BODY()

public:
	AHotelGuestLocationActor();

	FVector GetHotelStartLocation() const;
	FRotator GetHotelStartRotation() const;
	FVector GetEscapeLocation() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hotel Guest")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hotel Guest")
	TObjectPtr<UArrowComponent> HotelStartPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hotel Guest")
	TObjectPtr<UArrowComponent> EscapePoint;
};
