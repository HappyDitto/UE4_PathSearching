// Fill out your copyright notice in the Description page of Project Settings.


#include "Agent.h"
#include "EngineUtils.h"

// initialize the counter
int AAgent::Counter = 0;

// Sets default values
AAgent::AAgent()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// counter +1 and set up the agent id for debugging/logging
	Counter++;
	ID = Counter;

	// set up some initial values for the agent
	Health = 50;
	MoveSpeed = 100;
	Tolerance = 20;
	HasStart = false;
	SetupPreferredFoodType();
}

// Called when the game starts or when spawned
void AAgent::BeginPlay()
{
	Super::BeginPlay();

	// Set a timer for every two seconds and call the decrease health function
	GetWorldTimerManager().SetTimer(TimerHandle, this, &AAgent::DecreaseHealth, 2.0f, true, 2.0f);
	
	// Set up the level generator reference
	SetUpLevelGeneratorRef();
	// Set up the material of the agent
	SetupMaterial();
}

void AAgent::DecreaseHealth()
{
	// Decrease health by one and if 0 then destroy object
	Health--;

	if(Health <= 0)
	{
		GetWorldTimerManager().ClearTimer(TimerHandle);
		Destroy();
	}
}

// Called every frame
void AAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// if the agent is the first time starting their action
	if (!HasStart) {
		// search goal, calculate path and set the flag to true
		SearchGoal();
		HasStart = true;
		CalculateAStar();
	}

	// A tricky way to check if the agent has overlayed with the goal food
	// if the agent reaches the end of the path
	if (Path.Num() == 0) {
		// eat the food, search goal and calculate path
		Eat();
		SearchGoal();
		CalculateAStar();
	}

	// If the current goal is not valid (be eaten by other agents, etc.) and the agent has not reached good 
	if (!IsValid(CurrentGoal) && Path.Num() > 0) {
		// search goal and calculate path
		SearchGoal();
		CalculateAStar();
		//UE_LOG(LogClass, Log, TEXT("Agent%d Target Food Lost"), ID);
	}

	// If the agent has not reached the goal
	if(Path.Num() > 0)
	{
		// if the next node is not valid (another agent is at that node, the food at the node is not the one they like, etc.)
		if (!CheckNodeAvailablity(Path[0])) {
			// search goal and calculate path
			SearchGoal();
			CalculateAStar();
			// stop the current tick
			return;
		}

		// A tricky way to deal with the food that generates on the current paths but not the current goal
		// preventing the agent from going through the food
		// If the next node contains a food
		if (AFood* food = Cast<AFood>(Path[0]->ObjectAtLocation)) {
			// If the food is not the current goal and the agent likes this food
			if (food != CurrentGoal && food->Type == GetPreferredFoodType()) {
				// search goal
				SearchGoal();
				// recalculate path
				CalculateAStar();
				// stop the current tick
				return;
			}
		}

		// 'occupy' the next node, preventing agents from crashing
		Path[0]->ObjectAtLocation = this;

		// Move the agent
		FVector CurrentPosition = GetActorLocation();
		
		float TargetXPos = Path[0]->X * ALevelGenerator::GRID_SIZE_WORLD;
		float TargetYPos = Path[0]->Y * ALevelGenerator::GRID_SIZE_WORLD;

		FVector TargetPosition(TargetXPos, TargetYPos, CurrentPosition.Z);

		FVector Direction = TargetPosition - CurrentPosition;
		Direction.Normalize();

		CurrentPosition += Direction * MoveSpeed * DeltaTime;
		SetActorLocation(CurrentPosition);

		UE_LOG(LogClass, Log, TEXT("Agent%d CurrentPosition X: %d Y: %d"), ID, (int)(CurrentPosition.X/ ALevelGenerator::GRID_SIZE_WORLD), (int)(CurrentPosition.Y / ALevelGenerator::GRID_SIZE_WORLD));

		//UE_LOG(LogClass, Log, TEXT("Agent%d CurrentTarget%d X: %d Y: %d"), ID, Path.Num(), Path[0]->X, Path[0]->Y);

		// if the distance between the current position and the target position less than the tolerance
		if(FVector::Dist(CurrentPosition, TargetPosition) <= Tolerance)
		{
			// set the actor current position as the position of the next node
			CurrentPosition = TargetPosition;
			// 'release' the last node, so other agents now can go through that node
			LastNode->ObjectAtLocation = nullptr;
			// the last node now should be the current node which is Path[0]
			LastNode = Path[0];
			// has the agent has already been at Path[0], remove it from the path
			Path.RemoveAt(0);
		}
	}
}

// set up the level generator reference
void AAgent::SetUpLevelGeneratorRef() {
	// nullify the pointer
	LevelGenerator = nullptr;
	// loop through all the actors in the world
	for (TActorIterator<AActor> ActorItr(GetWorld()); ActorItr; ++ActorItr)
	{
		// if a actor can successfully cast to level generator
		if (ALevelGenerator* temp = Cast<ALevelGenerator>(*ActorItr)) {
			// set up the reference
			LevelGenerator = temp;
		}
	}
}

// Set up the start node
void AAgent::SetupStartNode() {
	// calculate the node information
	int X = GetActorLocation().X / ALevelGenerator::GRID_SIZE_WORLD;
	int Y = GetActorLocation().Y / ALevelGenerator::GRID_SIZE_WORLD;
	StartNode = LevelGenerator->WorldArray[X][Y];
	
	// 'occupy' the start node, preventing other agents from going through 
	StartNode->ObjectAtLocation = this;
	// set the start node as last node because the agent has already at the node
	LastNode = StartNode;
}

// set up the preferred food type of the agent
void AAgent::SetupPreferredFoodType() {
	int selector = FMath::RandRange(0, TYPE_COUNTER - 1);
	switch (selector) {
	case 0:
		Type = AAgent::Carnivore;
		return;
	case 1:
		Type = AAgent::Herbivore;
		return;
	default:
		Type = AAgent::Carnivore;
	}
}

// set up the material of the agent
void AAgent::SetupMaterial() {
	TArray<UActorComponent*> children;
	this->GetComponents(children);

	for (UActorComponent* child : children) {
		FString name = child->GetName();
		// set up the material for the child actor "cone"
		if (child->GetName() == "Cone")
		{
			UStaticMeshComponent* mesh = Cast<UStaticMeshComponent>(child);
			// if the agent is herbivore, set it to green, otherwise set it to red
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

// search the nearest goal 
void AAgent::SearchGoal() {
	// nullify the current goal pointer
	CurrentGoal = nullptr;
	// set up the min cost as a very large number, so it will be overwritten later
	float minCost = 9999999999.f;

	// loop through all the foods in the food actors array
	for (AFood* food : LevelGenerator->FoodActors) {
		// if the food has not been eaten and the food is valid and the agent likes the food
		if (!food->IsEaten && IsValid(food) && food->Type == GetPreferredFoodType()) {
			// get the node that the food is at
			int X = food->GetActorLocation().X / ALevelGenerator::GRID_SIZE_WORLD;
			int Y = food->GetActorLocation().Y / ALevelGenerator::GRID_SIZE_WORLD;
			GridNode* tempNode = LevelGenerator->WorldArray[X][Y];
			// if there is no other agents that 'occupies' the node
			if (Cast<AAgent>(tempNode->ObjectAtLocation) == nullptr) {
				// estimate the travel cost to the food
				float cost = EstimateTravelCost(food);
				// if the current cost less than the min cost
				if (cost < minCost) {
					// set the current goal as this food
					minCost = cost;
					CurrentGoal = food;
				}
			}
		}
	}

	// if the current goal has found
	if (CurrentGoal) {
		// set up the goal node for further use
		int X = CurrentGoal->GetActorLocation().X / ALevelGenerator::GRID_SIZE_WORLD;
		int Y = CurrentGoal->GetActorLocation().Y / ALevelGenerator::GRID_SIZE_WORLD;
		GoalNode = LevelGenerator->WorldArray[X][Y];
		//UE_LOG(LogClass, Log, TEXT("Agent%d Goal X: %d, Y: %d"), ID, X, Y);
	}
}

// Astar calculation to find the minimum path to the target
void AAgent::CalculateAStar() {
    // set up the variables and arrays for the calculation
	SetupStartNode();
	GridNode* currentNode = nullptr;
	GridNode* tempNode = nullptr;
	bool isPathCalculated = false;

	TArray<GridNode*> OpenedPath;
	TArray<GridNode*> ClosedPath;
	Path.Empty();
	OpenedPath.Empty();
	ClosedPath.Empty();

	// clear the start node's parent and explore the start node by calculating G, H, F
	StartNode->Parent = nullptr;
	StartNode->G = 0;
	StartNode->H = LevelGenerator->CalculateDistanceBetween(StartNode, GoalNode);
	StartNode->F = StartNode->G + StartNode->H;

	//UE_LOG(LogClass, Log, TEXT("Agent%d StartPosition X: %d Y: %d"), ID, StartNode->X, StartNode->Y);

	// Add the start node to the openList
	OpenedPath.Add(StartNode);
	//if the openList contain nodes
	while (OpenedPath.Num() > 0) {
		// go through each node and find the one that cost least
		float lowestF = 999999999;
		for (int i = 0; i < OpenedPath.Num(); i++) {
			if (OpenedPath[i]->F < lowestF) {
				lowestF = OpenedPath[i]->F;
				currentNode = OpenedPath[i];
			}
		}

		// remove the node from the openList and add it to the closeList
		OpenedPath.Remove(currentNode);
		ClosedPath.Add(currentNode);
		// if the node is the goal node
		if (currentNode == GoalNode) {
			// finish calculation and start generating the path
			isPathCalculated = true;
			break;
		}

		// Check the neighbours
		// Check to ensure not out of range
		if (currentNode->Y - 1 > 0)
		{
			// Get it from the worldArray
			tempNode = LevelGenerator->WorldArray[currentNode->X][currentNode->Y - 1];
			// Check to make sure the node hasnt been visited AND is valid (not wall, no other agent is 'occupying', ect.
			if (CheckNodeAvailablity(tempNode) && !ClosedPath.Contains(tempNode)) {
				// possible G equals curent Node's G adding the next node's G
				int possibleG = currentNode->G + tempNode->GetTravelCost();
				// set up the flag indicates if possible G is better
				bool isPossibleGBetter = false;

				// if the next node is not in the openList
				if (!OpenedPath.Contains(tempNode)) {
					// add it to the list
					OpenedPath.Add(tempNode);
					// set up the H value by calculating the distance between the next node and the goal node
					tempNode->H = LevelGenerator->CalculateDistanceBetween(tempNode, GoalNode);
					// possible G is better
					isPossibleGBetter = true;
				}
				// if possible G less than the next node's G but the next node is in the openList
				else if (possibleG < tempNode->G) {
					// possible G is better
					isPossibleGBetter = true;
				}

				// if possible G is better
				if (isPossibleGBetter) {
					// set up the next node's parent as the current node
					tempNode->Parent = currentNode;
					// next node's G = possible G
					tempNode->G = possibleG;
					// next node's F = G + H
					tempNode->F = tempNode->G + tempNode->H;
				}
			}
		}

		if (currentNode->X + 1 < LevelGenerator->MapSizeX)
		{
			tempNode = LevelGenerator->WorldArray[currentNode->X + 1][currentNode->Y];
			if (CheckNodeAvailablity(tempNode) && !ClosedPath.Contains(tempNode))
			{
				int possibleG = currentNode->G + tempNode->GetTravelCost();
				bool isPossibleGBetter = false;

				if (!OpenedPath.Contains(tempNode)) {
					OpenedPath.Add(tempNode);
					tempNode->H = LevelGenerator->CalculateDistanceBetween(tempNode, GoalNode);
					isPossibleGBetter = true;
				}
				else if (possibleG < tempNode->G) {
					isPossibleGBetter = true;
				}

				if (isPossibleGBetter) {
					tempNode->Parent = currentNode;
					tempNode->G = possibleG;
					tempNode->F = tempNode->G + tempNode->H;
				}
			}
		}

		if (currentNode->Y + 1 < LevelGenerator->MapSizeY)
		{
			tempNode = LevelGenerator->WorldArray[currentNode->X][currentNode->Y + 1];
			if (CheckNodeAvailablity(tempNode) && !ClosedPath.Contains(tempNode))
			{
				int possibleG = currentNode->G + tempNode->GetTravelCost();
				bool isPossibleGBetter = false;

				if (!OpenedPath.Contains(tempNode)) {
					OpenedPath.Add(tempNode);
					tempNode->H = LevelGenerator->CalculateDistanceBetween(tempNode, GoalNode);
					isPossibleGBetter = true;
				}
				else if (possibleG < tempNode->G) {
					isPossibleGBetter = true;
				}

				if (isPossibleGBetter) {
					tempNode->Parent = currentNode;
					tempNode->G = possibleG;
					tempNode->F = tempNode->G + tempNode->H;
				}
			}
		}

		if (currentNode->X - 1 > 0)
		{
			tempNode = LevelGenerator->WorldArray[currentNode->X - 1][currentNode->Y];
			if (CheckNodeAvailablity(tempNode) && !ClosedPath.Contains(tempNode))
			{
				int possibleG = currentNode->G + tempNode->GetTravelCost();
				bool isPossibleGBetter = false;

				if (!OpenedPath.Contains(tempNode)) {
					OpenedPath.Add(tempNode);
					tempNode->H = LevelGenerator->CalculateDistanceBetween(tempNode, GoalNode);
					isPossibleGBetter = true;
				}
				else if (possibleG < tempNode->G) {
					isPossibleGBetter = true;
				}

				if (isPossibleGBetter) {
					tempNode->Parent = currentNode;
					tempNode->G = possibleG;
					tempNode->F = tempNode->G + tempNode->H;
				}
			}
		}

	}

	// if the path has been calculated, generate the path
	if (isPathCalculated) {
		GeneratePath();
	}
}

void AAgent::GeneratePath()
{
	// set the current node as the goal node
	GridNode* CurrentNode = GoalNode;
	
	// count the nodes for debugging
	int pathcount = 0;

	// loop through all the nodes which the parent is not null
	while (CurrentNode->Parent != nullptr)
	{	
		//UE_LOG(LogClass, Log, TEXT("Agent%d Path%d X: %d Y: %d"), ID, pathcount, CurrentNode->X, CurrentNode->Y);
		
		// insert it to the start of the path array
		Path.Insert(CurrentNode, 0);

		// counter +1
		pathcount++;

		// the current node now should be the parent of the current node
		CurrentNode = CurrentNode->Parent;
	}
}

// eat the food
void AAgent::Eat() {
	// guard code preventing the program crashing
	if (CurrentGoal != nullptr) {
		// restore health
		Health = 50;
		// remove the food from the food array
		LevelGenerator->FoodActors.Remove(CurrentGoal);
		// set it as being eaten
		CurrentGoal->IsEaten = true;
		UE_LOG(LogClass, Log, TEXT("Agent%d Reached, Food: %s Consumed"), ID, *(CurrentGoal->GetName()));
		// destroy it
		CurrentGoal->Destroy();
	}
}

// get the food type the agent preferred
int AAgent::GetPreferredFoodType()
{
	switch (Type) {
	case AGENT_TYPE::Carnivore:
		return AFood::Meat;
	case AGENT_TYPE::Herbivore:
		return AFood::Vegetation;
	default:
		return AFood::Meat;
	}
}

// check if the node is avaliable
bool AAgent::CheckNodeAvailablity(GridNode* Node) {
	// the node cant be a wall
	if (Node->GridType == GridNode::Wall) {
		return false;
	}
	
	// if the node contains a object
	if (IsValid(Node->ObjectAtLocation)) {
		// if it is an agent
		if (AAgent* temp = Cast<AAgent>(Node->ObjectAtLocation)) {
			// if the agent is not 'this agent'
			if (temp != this) {
				// then it means the node has been 'occupied' and this agent cannot go through it
				return false;
			}
		}
		// if the object is a food
		if (AFood* temp = Cast<AFood>(Node->ObjectAtLocation)) {
			// if it is a food type that the agent do not like
			if (temp->Type != GetPreferredFoodType()) {
				// the agent should avoid it
				return false;
			}
		}
	}

	// otherwise it is ok to go 
	return true;
}

// estimate the travel cost by food reference
float AAgent::EstimateTravelCost(AFood* food) {
	FVector foodLocation = food->GetActorLocation();
	FVector agentLocation = GetActorLocation();
	FVector distance = FVector(foodLocation.X / ALevelGenerator::GRID_SIZE_WORLD - agentLocation.X / ALevelGenerator::GRID_SIZE_WORLD,
		foodLocation.Y / ALevelGenerator::GRID_SIZE_WORLD - agentLocation.Y / ALevelGenerator::GRID_SIZE_WORLD, 0);
	return distance.Size();
}

// check if the actor pointer is valid
bool IsValid(AActor* actor) {
	// if the pointer is pointing to a null pointer, then no
	if (!actor)
	{
		return false;
	}
	// if the pointer is not valid at low level, then no
	if (!actor->IsValidLowLevel())
	{
		return false;
	}
	// if the object the pointer points to is pending killed, then no
	if (actor->IsPendingKill())
	{
		return false;
	}
	// otherwise its alright
	return true;
}
