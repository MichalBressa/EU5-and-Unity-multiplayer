// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h" //default UE library
#include "GameFramework/Actor.h" //class we’re extending
#include "Networking.h" //networking module – read above if this causes compile errors

#include "NetworkGameObject.h"
#include "NetManager.generated.h"

UCLASS() //insert boilerplate UE code
class Audp_module : public AActor
{
	GENERATED_BODY() //insert boilerplate UE code
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> OtherPlayerAvatars;

	 
public:
	Audp_module();
	~Audp_module();
	//declare a bunch of variables we’ll be using in the .cpp. Normally you’d switch back to the header file and add as you go, but we’ll define them up-front.
	static TArray<UNetworkGameObject*> localNetObjects;
	FSocket* Socket;
	static Audp_module* singleton; //declare the class to contain a pointer to an instance of itself

	//notice the similarity in the types with the C# implementation. Primarily, we need a socket and endpoints, and arrays for data
	// Local Endpoint
	FString SocketDescription = "UDP Listen Socket";
	FIPv4Endpoint LocalEndpoint;
	uint16 LocalPort = 0;
	int32 SendSize;
	TArray<uint8> ReceivedData;

	// Remote Endpoint
	FIPv4Endpoint RemoteEndpoint;
	FIPv4Address RemoteAddress;
	uint16 RemotePort = 9050;
	int32 BufferSize;
	FString IP = "10.0.74.148";

	ISocketSubsystem* SocketSubsystem;

	//the methods we’ll use are similar to Unity’s Start (BeginPlay()) and Update (Tick()). We’ll also declare a method to listen to the socket for inbound data.

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void PostInitializeComponents();
public:
	virtual void Tick(float DeltaTime) override;
	void Listen();
	void AddNetObject(UNetworkGameObject* component);
	//we’ll think about blueprint integration later, but this is the starting bit
	UFUNCTION(BlueprintCallable, Category = "UDPNetworking")
		bool sendMessage(FString Message);

	void messageQueue();
};

