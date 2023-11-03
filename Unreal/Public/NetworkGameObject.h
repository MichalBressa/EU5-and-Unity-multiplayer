// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NetworkGameObject.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class LABTASK_API UNetworkGameObject : public UActorComponent
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere)
	bool isLocallyOwned;

	UPROPERTY(EditAnywhere)
	int32 globalID;

	UPROPERTY(EditAnywhere)
	int32 localID;


public:	
	// Sets default values for this component's properties
	UNetworkGameObject();
	static int32 lastLocalID;

	int32 GetGlobalID();
	int32 GetLocalID();
	void SetGlobalID(int32 gId);
	void SetLocalID(int32 lId);


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	bool GetIsLocallyOwned() { return isLocallyOwned; }
	void FromPacket(FString packet);
	FString ToPacket();
	int32 GlobalIDFromPacket(FString packet);

		
};
