// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Paintable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UPaintable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SHADER_API IPaintable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Painting")
	void OnRayHit(const FVector& HitPosition, const FVector2D& HitUV, UTexture*& OutHitSrcTexRT);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Painting")
	void OnRayExit();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Painting")
	void SetBrushSize(float SizeOffset);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Painting")
	void SetBrushHardness(float HardnessOffset);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Painting")
	void SetBrushOpacity(float OpacityOffset);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Painting")
	void Paint();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Painting")
	void PaintEnd();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Painting")
	void Capture(UTexture*& OutTex, bool& bSuccess);
};