// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkGameObject.h"
#include "NetManager.h" 

int32 UNetworkGameObject::lastLocalID = 0;

int32 UNetworkGameObject::GetGlobalID() {
	return globalID;
}

int32 UNetworkGameObject::GetLocalID() {
	return localID;
}

void UNetworkGameObject::SetGlobalID(int32 gid) {
	globalID = gid;
}

void UNetworkGameObject::SetLocalID(int32 lid) {
	localID = lid;
}


// Sets default values for this component's properties
UNetworkGameObject::UNetworkGameObject()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	isLocallyOwned = false;
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
};


// Called when the game starts
void UNetworkGameObject::BeginPlay()
{
	Super::BeginPlay();
	if (isLocallyOwned) {  //as before, we need a unique local ID
		localID = lastLocalID;
		lastLocalID++;
	}

	Audp_module::singleton->AddNetObject(this); //calls the above method on the singleton network manager
	// ... 

};

FString UNetworkGameObject::ToPacket() {
	AActor* parentActor = GetOwner();
	//UE_LOG(LogTemp, Warning, TEXT("parentActor: %s "), parentActor->GetFName());

	FVector position = parentActor->GetActorLocation();
	FRotator rotation = parentActor->GetActorRotation(); //unreal uses euler angles..
	FQuat quaternionRotation = FQuat(rotation); //so we have to convert to Quaternion for Unity consistence

	FString returnVal = FString::Printf(TEXT("Object data;%i;%f;%f;%f;%f;%f;%f;%f"), globalID,
		position.X,
		position.Y,
		position.Z,
		quaternionRotation.X,
		quaternionRotation.Y,
		quaternionRotation.Z,
		quaternionRotation.W
	);
	return returnVal;
}

int32 UNetworkGameObject::GlobalIDFromPacket(FString packet) {
	TArray<FString> parsed;
	packet.ParseIntoArray(parsed, TEXT(";"), false);
	return FCString::Atoi(*parsed[1]);
}

void UNetworkGameObject::FromPacket(FString packet) { //returns global id
	TArray<FString> parsed;
	packet.ParseIntoArray(parsed, TEXT(";"), false);
	AActor* parentActor = GetOwner();
	FVector position = FVector(FCString::Atof(*parsed[2]), FCString::Atof(*parsed[3]), FCString::Atof(*parsed[4]));
	FQuat rotation = FQuat(FCString::Atof(*parsed[5]), FCString::Atof(*parsed[6]), FCString::Atof(*parsed[7]), FCString::Atof(*parsed[8]));
	parentActor->SetActorLocation(position);
	parentActor->SetActorRotation(rotation);

}


