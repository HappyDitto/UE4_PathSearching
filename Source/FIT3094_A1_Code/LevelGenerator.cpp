// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelGenerator.h"
#include "Agent.h"
#include "Engine/World.h"

// Sets default values
ALevelGenerator::ALevelGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALevelGenerator::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ALevelGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Should spawn more food if there are not the right number

	// When one food is consumed, immediately generate another one
	while (FoodActors.Num() < NUM_FOOD) {
		int RandXPos = 0;
		int RandYPos = 0;
		bool isFree = false;

		while (!isFree) {
			RandXPos = FMath::RandRange(0, MapSizeX - 1);
			RandYPos = FMath::RandRange(0, MapSizeY - 1);

			if (WorldArray[RandXPos][RandYPos]->GridType != GridNode::Wall && WorldArray[RandXPos][RandYPos]->ObjectAtLocation == nullptr)
			{
				isFree = true;
			}
		}

		FVector Position(RandXPos * GRID_SIZE_WORLD, RandYPos * GRID_SIZE_WORLD, 20);
		AFood* NewFood = GetWorld()->SpawnActor<AFood>(FoodBlueprint, Position, FRotator::ZeroRotator);

		WorldArray[RandXPos][RandYPos]->ObjectAtLocation = NewFood;
		FoodActors.Add(NewFood);
	}
}

void ALevelGenerator::GenerateWorldFromFile(TArray<FString> WorldArrayStrings)
{
	// If empty array exit immediately something is horribly wrong
	if(WorldArrayStrings.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("World Array is empty!"));
		return;
	}

	// Second line is Height (aka X value)
	FString Height = WorldArrayStrings[1];
	Height.RemoveFromStart("height ");
	MapSizeX = FCString::Atoi(*Height);
	UE_LOG(LogTemp, Warning, TEXT("Height: %d"), MapSizeX);

	// Third line is Width (aka Y value)
	FString Width = WorldArrayStrings[2];
	Width.RemoveFromStart("width ");
	MapSizeY = FCString::Atoi(*Width);
	UE_LOG(LogTemp, Warning, TEXT("Width: %d"), MapSizeY);
	
	// After removing top 4 lines this is the map itself so iterate each line
	for (int LineNum = 4; LineNum < WorldArrayStrings.Num(); LineNum++)
	{
		for (int CharNum = 0; CharNum < WorldArrayStrings[LineNum].Len(); CharNum++)
		{
			CharMapArray[LineNum-4][CharNum] = WorldArrayStrings[LineNum][CharNum];
		}
	}

	GenerateNodeGrid();
	SpawnWorldActors();
}

void ALevelGenerator::SpawnWorldActors()
{
	UWorld* World = GetWorld();

	// Make sure that all blueprints are connected. If not then fail
	if(WallBlueprint && OpenBlueprint && WaterBlueprint && SwampBlueprint && TreeBlueprint)
	{
		// For each grid space spawn an actor of the correct type in the game world
		for(int x = 0; x < MapSizeX; x++)
		{
			for (int y = 0; y < MapSizeY; y++)
			{
				float XPos = x * GRID_SIZE_WORLD;
				float YPos = y * GRID_SIZE_WORLD;

				FVector Position(XPos, YPos, 0);

				switch (CharMapArray[x][y])
				{
					case '.':
					case 'G':
						World->SpawnActor(OpenBlueprint, &Position, &FRotator::ZeroRotator);
						break;
					case '@':
					case 'O':
						World->SpawnActor(WallBlueprint, &Position, &FRotator::ZeroRotator);
						break;
					case 'T':
						World->SpawnActor(TreeBlueprint, &Position, &FRotator::ZeroRotator);
						break;
					case 'S':
						World->SpawnActor(SwampBlueprint, &Position, &FRotator::ZeroRotator);
						break;
					case 'W':
						World->SpawnActor(WaterBlueprint, &Position, &FRotator::ZeroRotator);
						break;
					default:
						break;
				}
			}
		}
	}

	// Generate Initial Agent Positions
	if(AgentBlueprint)
	{
		for (int i = 0; i < NUM_AGENTS; i++)
		{
			int RandXPos = 0;
			int RandYPos = 0;
			bool isFree = false;

			while (!isFree) {
				RandXPos = FMath::RandRange(0, MapSizeX - 1);
				RandYPos = FMath::RandRange(0, MapSizeY - 1);

				if (WorldArray[RandXPos][RandYPos]->GridType == GridNode::Open && WorldArray[RandXPos][RandYPos]->ObjectAtLocation == nullptr)
				{
					isFree = true;
				}
			}

			FVector Position(RandXPos * GRID_SIZE_WORLD, RandYPos * GRID_SIZE_WORLD, 20);
			AAgent* Agent = World->SpawnActor<AAgent>(AgentBlueprint, Position, FRotator::ZeroRotator);

			WorldArray[RandXPos][RandYPos]->ObjectAtLocation = Agent;
		}
	}

	// Generate Initial Food Positions
	if(FoodBlueprint)
	{
		for(int i = 0; i < NUM_FOOD; i++)
		{
			int RandXPos = 0;
			int RandYPos = 0;
			bool isFree = false;

			while (!isFree) {
				RandXPos = FMath::RandRange(0, MapSizeX - 1);
				RandYPos = FMath::RandRange(0, MapSizeY - 1);

				if (WorldArray[RandXPos][RandYPos]->GridType != GridNode::Wall && WorldArray[RandXPos][RandYPos]->ObjectAtLocation == nullptr)
				{
					isFree = true;
				}
			}

			FVector Position(RandXPos * GRID_SIZE_WORLD, RandYPos * GRID_SIZE_WORLD, 20);
			AFood* NewFood = World->SpawnActor<AFood>(FoodBlueprint, Position, FRotator::ZeroRotator);

			WorldArray[RandXPos][RandYPos]->ObjectAtLocation = NewFood;
			FoodActors.Add(NewFood);
		}
	}
}

// Generates the grid of nodes used for pathfinding and also for placement of objects in the game world
void ALevelGenerator::GenerateNodeGrid()
{
	for(int X = 0; X < MapSizeX; X++)
	{
		for(int Y = 0; Y < MapSizeY; Y++)
		{
			WorldArray[X][Y] = new GridNode();
			WorldArray[X][Y]->X = X;
			WorldArray[X][Y]->Y = Y;

			// Characters as defined from the map file
			switch(CharMapArray[X][Y])
			{
				case '.':
				case 'G':
					WorldArray[X][Y]->GridType = GridNode::Open;
					break;
				case '@':
				case 'O':
					WorldArray[X][Y]->GridType = GridNode::Wall;
					break;
				case 'T':
					WorldArray[X][Y]->GridType = GridNode::Forest;
					break;
				case 'S':
					WorldArray[X][Y]->GridType = GridNode::Swamp;
					break;
				case 'W':
					WorldArray[X][Y]->GridType = GridNode::Water;
					break;
			}
		}
	}
}

// Reset all node values (F, G, H & Parent)
void ALevelGenerator::ResetAllNodes()
{
	for (int X = 0; X < MapSizeX; X++)
	{
		for (int Y = 0; Y < MapSizeY; Y++)
		{
			WorldArray[X][Y]->F = 0;
			WorldArray[X][Y]->G = 0;
			WorldArray[X][Y]->H = 0;
			WorldArray[X][Y]->Parent = nullptr;
		}
	}
}

float ALevelGenerator::CalculateDistanceBetween(GridNode* first, GridNode* second)
{
	FVector distToTarget = FVector(second->X - first->X,
		second->Y - first->Y, 0);
	return distToTarget.Size();
}