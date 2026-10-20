#include "Zipline/ZiplineMountIndicatorComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Common/PlayerViewSupport.h"
#include "GameplayTagContainer.h"
#include "Materials/MaterialInterface.h"
#include "Zipline/ZiplineActor.h"
#include "Zipline/ZiplineInteractionLibrary.h"
#include "Zipline/ZiplineSplineVisualComponent.h"

UZiplineMountIndicatorComponent::UZiplineMountIndicatorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UZiplineMountIndicatorComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const AActor* Owner = GetOwner(); Owner && Owner->GetNetMode() == NM_DedicatedServer)
	{
		SetComponentTickEnabled(false);
	}
}

void UZiplineMountIndicatorComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AZiplineActor* Zipline = Cast<AZiplineActor>(GetOwner());
	if (!Zipline || Zipline->GetNetMode() == NM_DedicatedServer || !bShowMountIndicator || !Zipline->GetSpline())
	{
		Hide();
		return;
	}

	UStaticMesh* DashMesh = MountIndicatorDashMesh;
	UMaterialInterface* DashMaterial = MountIndicatorDashMaterial;
	if (!DashMesh || !DashMaterial)
	{
		if (const UZiplineSplineVisualComponent* VisualComponent = Zipline->GetSplineVisualComponent())
		{
			if (!DashMesh)
			{
				DashMesh = VisualComponent->VisualSplineMesh;
			}

			if (!DashMaterial)
			{
				DashMaterial = VisualComponent->VisualMaterial;
			}
		}
	}

	if (!DashMesh)
	{
		Hide();
		return;
	}

	ACharacter* Character = nullptr;
	if (!TryGetLocalCharacter(Character) || IsCharacterUsingZipline(Character))
	{
		Hide();
		return;
	}

	FZiplineInteractionResult InteractionResult;
	if (!FZiplineInteractionLibrary::BuildInteractionResult(Character, Zipline, InteractionResult))
	{
		Hide();
		return;
	}

	const FVector IndicatorStart = Character->GetActorLocation() + MountIndicatorStartOffset;
	UpdateDashMeshes(IndicatorStart, InteractionResult.MountPoint, DashMesh, DashMaterial);
}

void UZiplineMountIndicatorComponent::Hide()
{
	for (USplineMeshComponent* DashComponent : DashComponents)
	{
		if (IsValid(DashComponent))
		{
			DashComponent->SetVisibility(false);
		}
	}
}

void UZiplineMountIndicatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (USplineMeshComponent* DashComponent : DashComponents)
	{
		if (IsValid(DashComponent))
		{
			DashComponent->DestroyComponent();
		}
	}

	DashComponents.Reset();
	Super::EndPlay(EndPlayReason);
}

bool UZiplineMountIndicatorComponent::TryGetLocalCharacter(ACharacter*& OutCharacter) const
{
	OutCharacter = nullptr;

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	APlayerController* PlayerController = DLPlayerView::GetLocalController(World);
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return false;
	}

	OutCharacter = Cast<ACharacter>(PlayerController->GetPawn());
	return OutCharacter != nullptr;
}

bool UZiplineMountIndicatorComponent::IsCharacterUsingZipline(const ACharacter* Character) const
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Character);
	const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface
		? AbilitySystemInterface->GetAbilitySystemComponent()
		: nullptr;
	if (!AbilitySystemComponent)
	{
		return false;
	}

	const FGameplayTag ZiplineStateTag = FGameplayTag::RequestGameplayTag(TEXT("State.Movement.Zipline"), false);
	return ZiplineStateTag.IsValid() && AbilitySystemComponent->HasMatchingGameplayTag(ZiplineStateTag);
}

void UZiplineMountIndicatorComponent::EnsureDashComponents(int32 RequiredCount)
{
	AZiplineActor* Zipline = Cast<AZiplineActor>(GetOwner());
	if (!Zipline || !Zipline->GetSpline())
	{
		return;
	}

	while (DashComponents.Num() < RequiredCount)
	{
		USplineMeshComponent* DashComponent = NewObject<USplineMeshComponent>(GetOwner());
		DashComponent->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		DashComponent->SetMobility(EComponentMobility::Movable);
		DashComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DashComponent->SetCastShadow(false);
		DashComponent->SetupAttachment(Zipline->GetSpline());
		DashComponent->RegisterComponent();
		DashComponents.Add(DashComponent);
	}
}

void UZiplineMountIndicatorComponent::UpdateDashMeshes(
	const FVector& IndicatorStart,
	const FVector& IndicatorEnd,
	UStaticMesh* DashMesh,
	UMaterialInterface* DashMaterial
)
{
	AZiplineActor* Zipline = Cast<AZiplineActor>(GetOwner());
	if (!Zipline || !Zipline->GetSpline())
	{
		Hide();
		return;
	}

	const FVector IndicatorVector = IndicatorEnd - IndicatorStart;
	const float IndicatorLength = IndicatorVector.Size();
	if (!GetWorld() || !FMath::IsFinite(IndicatorLength) || IndicatorLength <= KINDA_SMALL_NUMBER
		|| !FMath::IsFinite(MountIndicatorDashSpacing) || MountIndicatorDashSpacing <= KINDA_SMALL_NUMBER
		|| !FMath::IsFinite(MountIndicatorDashLength) || MountIndicatorDashLength <= 0.f
		|| !FMath::IsFinite(MountIndicatorDashSpeed) || MountIndicatorDashCount <= 0)
	{
		Hide();
		return;
	}

	const int32 SafeDashCount = FMath::Clamp(MountIndicatorDashCount, 1, 256);
	EnsureDashComponents(SafeDashCount);

	const FTransform& SplineTransform = Zipline->GetSpline()->GetComponentTransform();
	const FVector Direction = IndicatorVector / IndicatorLength;
	const float AnimatedOffset = FMath::Fmod(
		GetWorld()->GetTimeSeconds() * MountIndicatorDashSpeed,
		MountIndicatorDashSpacing
	);

	for (int32 DashIndex = 0; DashIndex < DashComponents.Num(); ++DashIndex)
	{
		USplineMeshComponent* DashComponent = DashComponents[DashIndex];
		if (!IsValid(DashComponent))
		{
			continue;
		}

		if (DashIndex >= SafeDashCount)
		{
			DashComponent->SetVisibility(false);
			continue;
		}

		const float DashStartDistance = DashIndex * MountIndicatorDashSpacing + AnimatedOffset;
		const float DashEndDistance = DashStartDistance + MountIndicatorDashLength;
		if (DashStartDistance >= IndicatorLength)
		{
			DashComponent->SetVisibility(false);
			continue;
		}

		const FVector DashStartWorld = IndicatorStart + Direction * DashStartDistance;
		const FVector DashEndWorld = IndicatorStart + Direction * FMath::Min(DashEndDistance, IndicatorLength);
		const FVector DashStartLocal = SplineTransform.InverseTransformPosition(DashStartWorld);
		const FVector DashEndLocal = SplineTransform.InverseTransformPosition(DashEndWorld);
		const FVector DashTangentLocal = SplineTransform.InverseTransformVectorNoScale(
			Direction * FMath::Max(MountIndicatorDashLength, 1.0f)
		);

		if (DashComponent->GetStaticMesh() != DashMesh) DashComponent->SetStaticMesh(DashMesh);
		DashComponent->SetForwardAxis(ESplineMeshAxis::Z, false);
		DashComponent->SetStartAndEnd(DashStartLocal, DashTangentLocal, DashEndLocal, DashTangentLocal, false);
		DashComponent->SetStartScale(MountIndicatorDashScale, false);
		DashComponent->SetEndScale(MountIndicatorDashScale, false);

		if (DashMaterial)
		{
			if (DashComponent->GetMaterial(0) != DashMaterial) DashComponent->SetMaterial(0, DashMaterial);
		}

		DashComponent->SetVisibility(true);
		DashComponent->UpdateMesh();
	}
}
