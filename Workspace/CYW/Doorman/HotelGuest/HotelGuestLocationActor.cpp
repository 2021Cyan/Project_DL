#include "Doorman/HotelGuest/HotelGuestLocationActor.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

AHotelGuestLocationActor::AHotelGuestLocationActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	HotelStartPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("HotelStartPoint"));
	HotelStartPoint->SetupAttachment(Root);
	HotelStartPoint->ArrowColor = FColor::Cyan;

	EscapePoint = CreateDefaultSubobject<UArrowComponent>(TEXT("EscapePoint"));
	EscapePoint->SetupAttachment(Root);
	EscapePoint->ArrowColor = FColor::Green;
}

FVector AHotelGuestLocationActor::GetHotelStartLocation() const
{
	return HotelStartPoint ? HotelStartPoint->GetComponentLocation() : GetActorLocation();
}

FRotator AHotelGuestLocationActor::GetHotelStartRotation() const
{
	return HotelStartPoint ? HotelStartPoint->GetComponentRotation() : GetActorRotation();
}

FVector AHotelGuestLocationActor::GetEscapeLocation() const
{
	return EscapePoint ? EscapePoint->GetComponentLocation() : GetActorLocation();
}
