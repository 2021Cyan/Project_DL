// Fill out your copyright notice in the Description page of Project Settings.


#include "Doorman/Doorway/DoorwayActor.h"
#include "Doorman/Doorway/DoorwayCameraTransitionComponent.h"
#include "Doorman/Doorway/DoorwayPortalRenderComponent.h"
#include "Doorman/Doorway/DoorwayTeleportComponent.h"

#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

ADoorwayActor::ADoorwayActor()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DoorMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(SceneRoot);
	DoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ForwardDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("ForwardDirection"));
	ForwardDirection->SetupAttachment(SceneRoot);

	PortalCamera = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortalCamera"));
	PortalCamera->SetupAttachment(SceneRoot);

	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(SceneRoot);
	PlayerCamera->SetActive(false);

	PlaneIntersectionTest = CreateDefaultSubobject<UBoxComponent>(TEXT("PlaneIntersectionTest"));
	PlaneIntersectionTest->SetupAttachment(ForwardDirection);
	PlaneIntersectionTest->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PlaneIntersectionTest->SetCollisionResponseToAllChannels(ECR_Ignore);
	PlaneIntersectionTest->SetCollisionResponseToChannel(DoorwayPortalMath::TraversalTraceChannel, ECR_Block);
	PlaneIntersectionTest->SetGenerateOverlapEvents(false);

	TeleportDetection = CreateDefaultSubobject<UBoxComponent>(TEXT("TeleportDetection"));
	TeleportDetection->SetupAttachment(SceneRoot);
	TeleportDetection->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TeleportDetection->SetCollisionResponseToAllChannels(ECR_Overlap);
	TeleportDetection->SetGenerateOverlapEvents(true);

	PlayerNearbyBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("PlayerNearbyBounds"));
	PlayerNearbyBounds->SetupAttachment(SceneRoot);
	PlayerNearbyBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PlayerNearbyBounds->SetCollisionResponseToAllChannels(ECR_Overlap);
	PlayerNearbyBounds->SetGenerateOverlapEvents(true);

	CameraTransitionComponent = CreateDefaultSubobject<UDoorwayCameraTransitionComponent>(
		TEXT("DoorwayCameraTransitionComponent"));
	PortalRenderComponent = CreateDefaultSubobject<UDoorwayPortalRenderComponent>(
		TEXT("DoorwayPortalRenderComponent"));
	TeleportComponent = CreateDefaultSubobject<UDoorwayTeleportComponent>(
		TEXT("DoorwayTeleportComponent"));
}

void ADoorwayActor::BeginPlay()
{
	Super::BeginPlay();
	if (PlaneIntersectionTest)
	{
		PlaneIntersectionTest->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	if (PlayerNearbyBounds)
	{
		PlayerNearbyBounds->OnComponentBeginOverlap.AddDynamic(
			this, &ADoorwayActor::OnPlayerNearbyBeginOverlap);
		PlayerNearbyBounds->OnComponentEndOverlap.AddDynamic(
			this, &ADoorwayActor::OnPlayerNearbyEndOverlap);

		TArray<AActor*> InitiallyOverlappingActors;
		PlayerNearbyBounds->GetOverlappingActors(InitiallyOverlappingActors);
		for (AActor* Actor : InitiallyOverlappingActors)
		{
			if (TeleportComponent)
			{
				TeleportComponent->AddInitialOverlap(Actor);
			}
		}
	}

	if (GetNetMode() != NM_DedicatedServer)
	{
		InitializePortal();
	}
}

void ADoorwayActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetLinkedDoorway(nullptr);
	if (CameraTransitionComponent)
	{
		CameraTransitionComponent->FinishOwnerIfAny();
	}

	Super::EndPlay(EndPlayReason);
}

void ADoorwayActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (CameraTransitionComponent)
		{
			CameraTransitionComponent->TickCameraTransition(*this, DeltaSeconds);
		}
		if (PortalRenderComponent && IsValid(LinkedDoorway))
		{
			PortalRenderComponent->TickPortalRenderer(*this, DeltaSeconds);
		}
	}
	if (HasAuthority() && TeleportComponent)
	{
		TeleportComponent->TickServer(*this);
	}
}

void ADoorwayActor::InitializeDoorwayEndpoint(EDoorwayEndpointType InEndpointType, ADoorwayActor* InLinkedDoorway)
{
	EndpointType = InEndpointType;
	LinkedDoorway = InLinkedDoorway;
	OnRep_EndpointType();
	RefreshPortalLink();
}

void ADoorwayActor::SetLinkedDoorway(ADoorwayActor* InLinkedDoorway)
{
	LinkedDoorway = InLinkedDoorway;
	RefreshPortalLink();
	if (HasAuthority()) ForceNetUpdate();
}

void ADoorwayActor::RefreshPortalLink()
{
	if (ADoorwayActor* Previous = AppliedLinkedDoorway.Get())
	{
		if (Previous != LinkedDoorway && Previous->PortalCamera
			&& Previous->PortalCamera->TextureTarget == PortalRT)
		{
			Previous->PortalCamera->TextureTarget = nullptr;
		}
	}
	AppliedLinkedDoorway = LinkedDoorway;
	if (PortalRT && IsValid(LinkedDoorway) && LinkedDoorway->PortalCamera)
		LinkedDoorway->PortalCamera->TextureTarget = PortalRT;
}

void ADoorwayActor::PlayConnectedMontage()
{
	if (HasAuthority())
	{
		++ConnectedMontagePlayCount;
		OnRep_PlayConnectedMontage();
		ForceNetUpdate();
		return;
	}

	TryPlayConnectedMontage();
}

void ADoorwayActor::TryPlayConnectedMontage()
{
	if (!ConnectedMontage || !DoorMesh)
	{
		return;
	}

	DoorMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	DoorMesh->bPauseAnims = false;
	DoorMesh->SetAnimation(ConnectedMontage);
	DoorMesh->SetPosition(0.f, false);
	DoorMesh->Play(false);
}

void ADoorwayActor::OnRep_EndpointType()
{
	const FName TagName = EndpointType == EDoorwayEndpointType::Entrance? TEXT("Doorway.Entrance") : TEXT("Doorway.Exit");

	Tags.Remove(TEXT("Doorway.Entrance"));
	Tags.Remove(TEXT("Doorway.Exit"));
	Tags.AddUnique(TagName);
}

void ADoorwayActor::OnRep_LinkedDoorway()
{
	RefreshPortalLink();
}

void ADoorwayActor::OnRep_PlayConnectedMontage()
{
	if (LastPlayedConnectedMontageCount == ConnectedMontagePlayCount) return;
	LastPlayedConnectedMontageCount = ConnectedMontagePlayCount;
	TryPlayConnectedMontage();
}

void ADoorwayActor::MulticastPlayConnectedMontage_Implementation()
{
	TryPlayConnectedMontage();
}

void ADoorwayActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADoorwayActor, EndpointType);
	DOREPLIFETIME(ADoorwayActor, LinkedDoorway);
	DOREPLIFETIME(ADoorwayActor, ConnectedMontagePlayCount);
}
