// Fill out your copyright notice in the Description page of Project Settings.


#include "Food.h"

// Sets default values
AFood::AFood()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	SetupFoodType();
}

// Called when the game starts or when spawned
void AFood::BeginPlay()
{
	Super::BeginPlay();

	SetupMaterial();

}

// Called in the contructor to set up the food type
void AFood::SetupFoodType() {
	int selector = FMath::RandRange(0, TYPE_COUNTER-1);
	switch (selector) {
	case 0:
		Type = AFood::Meat;
		return;
	case 1:
		Type = AFood::Vegetation;
		return;
	default:
		Type = AFood::Meat;
	}
}

// Set up the Material for the Cylinder child actor, based on the food type
void AFood::SetupMaterial() {
	TArray<UActorComponent*> children;
	this->GetComponents(children);

	for (UActorComponent* child : children) {
		FString name = child->GetName();
		if (child->GetName() == "Cylinder")
		{
			UStaticMeshComponent* mesh = Cast<UStaticMeshComponent>(child);
			// If its a vegetable, set it to green, otherwise set it to green
			switch (Type) {
			case 0:
				mesh->SetMaterial(0, CarnivoreMat);
				return;
			case 1:
				mesh->SetMaterial(0, HerbivoreMat);
				return;
			default:
				mesh->SetMaterial(0, CarnivoreMat);
				return;
			}
		}
	}
}

// Called every frame
void AFood::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

