// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridNode.h"
#include "Food.h"
#include "LevelGenerator.h"
#include "GameFramework/Actor.h"
#include "Agent.generated.h"

UCLASS()
class FIT3094_A1_CODE_API AAgent : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAgent();

	// an enum to indicates the type of the agent
	enum AGENT_TYPE
	{
		Carnivore,
		Herbivore,
		TYPE_COUNTER
	};

	static int Counter; // class range counter to count the agent number. It is for debugging/logging purpose
	int ID; // The id of the current agent. It is for debugging/logging purpose
	int Health; // The health of the agent
	float MoveSpeed; // The move speed of the agent
	float Tolerance; // The tolerance to judge if the agent has overlayed with food
	GridNode* StartNode; // The starting node in the current path
	GridNode* GoalNode; // The goal node in the current path
	GridNode* LastNode; // The previous node that the agent used to be in 
	ALevelGenerator* LevelGenerator; // The level generator reference for accesssing some members
	bool HasStart; // The flag to indicate if the agent has started their action
	AFood* CurrentGoal; // The food the agent is going for
	AGENT_TYPE Type; // The type of the agent
	TArray<GridNode*> Path; // The path the agent is following

	// The materials for different types of agent
	UPROPERTY(EditAnywhere, Category = "Mat")
		UMaterial* HerbivoreMat;
	UPROPERTY(EditAnywhere, Category = "Mat")
		UMaterial* CarnivoreMat;
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Timed function that decreases health every 2 seconds;
	void DecreaseHealth();
	
	// Some initialization function
	void SetUpLevelGeneratorRef(); // set up the level generator reference for further calling
	void SetupPreferredFoodType(); // set up the food type that the agent likes
	void SetupMaterial(); // set up the material based on the agent type
	void SetupStartNode(); // set up the start node in the current path

	// Agent Behaviours
	void SearchGoal(); // search the nearest food as the goal 
	void CalculateAStar(); // calculate the path by Astar
	void GeneratePath(); // generate the path based on the calculation
	void Eat(); // Eat the food at the current node
	
	// Some helper functions
	int GetPreferredFoodType(); // based on the agent type, get their preferred food type
	bool CheckNodeAvailablity(GridNode * Node); // check the availability of the node, preventing the game from crashing 
	float EstimateTravelCost(AFood* food); // allows to use AFood pointer as parameter to calculate distance
	
	// Handle for Timer
	FTimerHandle TimerHandle;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};