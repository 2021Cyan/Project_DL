// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TEAMPROJECT_DL_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWeaponComponent();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void RefreshAmmoFromAttributes(bool bRefillIfEmpty = false);

	bool CanFire();

	bool CanReload();

	void ConsumeAmmo();

	void ReloadAmmo();

	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	int32 GetMagazineSize() const { return MagazineSize; }

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon")
	int32 MagazineSize = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon")
	int32 CurrentAmmo = 0;
};
