// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Food.generated.h"

UCLASS()
class FIT3094_A1_CODE_API AFood : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFood();

	// an enum which lists all types of food
	enum FOOD_TYPE
	{
		Meat,
		Vegetation,
		TYPE_COUNTER
	};

	// the type of the the food
	FOOD_TYPE Type;

	// If the food has been eaten
	bool IsEaten;

	// Two materials for different type of food 
	UPROPERTY(EditAnywhere, Category = "Mat")
		UMaterial* HerbivoreMat;
	UPROPERTY(EditAnywhere, Category = "Mat")
		UMaterial* CarnivoreMat;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// randomly set up the type of food
	void SetupFoodType();
	// Based on the food type, set up the corresponding material
	void SetupMaterial();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
