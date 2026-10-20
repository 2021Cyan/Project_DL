#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZiplineSplineVisualComponent.generated.h"

class UMaterialInterface;
class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;

UCLASS(ClassGroup = (Zipline), meta = (BlueprintSpawnableComponent))
class TEAMPROJECT_DL_API UZiplineSplineVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZiplineSplineVisualComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> VisualSplineMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.001"))
	FVector2D VisualMeshScale = FVector2D(0.05f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "1", ClampMax = "8"))
	int32 VisualSubdivisionsPerSplineSegment = 4;

	void Rebuild(USplineComponent* Spline);
	void Clear();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void CreateSegment(
		USplineComponent* Spline,
		const FVector& StartLocation,
		const FVector& StartTangent,
		const FVector& EndLocation,
		const FVector& EndTangent
	);

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> SplineMeshComponents;
};
