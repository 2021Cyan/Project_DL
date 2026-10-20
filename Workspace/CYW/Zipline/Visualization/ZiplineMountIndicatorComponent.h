#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZiplineMountIndicatorComponent.generated.h"

class ACharacter;
class UMaterialInterface;
class USplineMeshComponent;
class UStaticMesh;

UCLASS(ClassGroup = (Zipline), meta = (BlueprintSpawnableComponent))
class TEAMPROJECT_DL_API UZiplineMountIndicatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZiplineMountIndicatorComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator")
	bool bShowMountIndicator = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator")
	TObjectPtr<UStaticMesh> MountIndicatorDashMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator")
	TObjectPtr<UMaterialInterface> MountIndicatorDashMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator")
	FVector MountIndicatorStartOffset = FVector(0.0f, 0.0f, 50.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator", meta = (ClampMin = "1", ClampMax = "64"))
	int32 MountIndicatorDashCount = 12;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator", meta = (ClampMin = "1.0"))
	float MountIndicatorDashLength = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator", meta = (ClampMin = "1.0"))
	float MountIndicatorDashSpacing = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator", meta = (ClampMin = "0.0"))
	float MountIndicatorDashSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator", meta = (ClampMin = "0.001"))
	FVector2D MountIndicatorDashScale = FVector2D(0.035f, 0.035f);

	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	void Hide();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool TryGetLocalCharacter(ACharacter*& OutCharacter) const;
	bool IsCharacterUsingZipline(const ACharacter* Character) const;
	void EnsureDashComponents(int32 RequiredCount);
	void UpdateDashMeshes(
		const FVector& IndicatorStart,
		const FVector& IndicatorEnd,
		UStaticMesh* DashMesh,
		UMaterialInterface* DashMaterial
	);

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> DashComponents;
};
