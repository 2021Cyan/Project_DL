// Fill out your copyright notice in the Description page of Project Settings.
#include "WeaponComponent.h"
#include "GameCore/Debug/DLDebug.h"
#include "GameCore/Struct/ACharacterAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UWeaponComponent::UWeaponComponent()
{
	SetIsReplicatedByDefault(true);
}

void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshAmmoFromAttributes(true);
}

void UWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWeaponComponent, CurrentAmmo);
	DOREPLIFETIME(UWeaponComponent, MagazineSize);
}

void UWeaponComponent::RefreshAmmoFromAttributes(bool bRefillIfEmpty)
{
	// Server attributes are authoritative; clients observe replicated display caches.
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (!ASC)
	{
		return;
	}

	const int32 StatMaxAmmo = FMath::Max(0, FMath::RoundToInt(ASC->GetNumericAttribute(UACharacterAttributeSet::GetMaxAmmoAttribute())));
	if (StatMaxAmmo <= 0)
	{
		return;
	}

	MagazineSize = StatMaxAmmo;

	int32 AttributeAmmo = FMath::RoundToInt(ASC->GetNumericAttribute(UACharacterAttributeSet::GetAmmoAttribute()));
	if (bRefillIfEmpty && CurrentAmmo <= 0 && AttributeAmmo <= 0)
	{
		AttributeAmmo = MagazineSize;
		ASC->SetNumericAttributeBase(UACharacterAttributeSet::GetAmmoAttribute(), AttributeAmmo);
	}

	CurrentAmmo = FMath::Clamp(AttributeAmmo, 0, MagazineSize);
}

bool UWeaponComponent::CanFire()
{
	RefreshAmmoFromAttributes();
	return CurrentAmmo > 0;
}

bool UWeaponComponent::CanReload()
{
	RefreshAmmoFromAttributes();
	return CurrentAmmo < MagazineSize;
}

void UWeaponComponent::ConsumeAmmo()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	RefreshAmmoFromAttributes();
	if (CurrentAmmo > 0) {
		CurrentAmmo--;
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
		{
			ASC->SetNumericAttributeBase(UACharacterAttributeSet::GetAmmoAttribute(), CurrentAmmo);
		}
	}
}

void UWeaponComponent::ReloadAmmo()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	RefreshAmmoFromAttributes();
	DLDebug::Screen(FString::Printf(TEXT("[WeaponReload] ReloadAmmo before Ammo=%d/%d Owner=%s"), CurrentAmmo, MagazineSize, *GetNameSafe(GetOwner())), FColor::Cyan);
	CurrentAmmo = MagazineSize;
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		ASC->SetNumericAttributeBase(UACharacterAttributeSet::GetAmmoAttribute(), CurrentAmmo);
	}
	DLDebug::Screen(FString::Printf(TEXT("[WeaponReload] ReloadAmmo after Ammo=%d/%d Owner=%s"), CurrentAmmo, MagazineSize, *GetNameSafe(GetOwner())), FColor::Green);
}
