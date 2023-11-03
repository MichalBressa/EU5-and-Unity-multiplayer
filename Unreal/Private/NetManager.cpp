// Fill out your copyright notice in the Description page of Project Settings.


#include "NetManager.h"
#include <string>


TArray<UNetworkGameObject*> Audp_module::localNetObjects;
Audp_module* Audp_module::singleton = nullptr; //declare the pointer

Audp_module::Audp_module()
{
	PrimaryActorTick.bCanEverTick = true;
	Socket = nullptr;
}

Audp_module::~Audp_module()
{
}

void Audp_module::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for (UNetworkGameObject* netObject : Audp_module::localNetObjects) {
		if (netObject->GetIsLocallyOwned() && netObject->GetGlobalID() != 0) {
			UE_LOG(LogTemp, Warning, TEXT("Sending: %s"), *netObject->ToPacket());
			sendMessage(netObject->ToPacket());
		}
	}

	Listen(); // Listen for messages
}

void Audp_module:: AddNetObject(UNetworkGameObject* component) {
	Audp_module::localNetObjects.Add(component);
	if ((component->GetGlobalID() == 0) && (component->GetIsLocallyOwned())) {
		FString t = "I need a UID for local object:" + FString::FromInt(component->GetLocalID());
		sendMessage(t);
	}
}



//Our main logic will be in the BeginPlayand Tick methods.Code in BeginPlay happens when the AActor is initialised, similar to Unity’s start method.Note the similarities with what we accomplished in the C# Unity client :

void Audp_module::PostInitializeComponents() 
{
	Super::PostInitializeComponents();

	if (singleton == nullptr) { singleton = this; } //if it’s null, it becomes the current instance

	SocketSubsystem = nullptr;
	//more macro code. We’re using Unreal’s low level generic networking (as opposed to it’s higher level game-oriented solution).
	if (SocketSubsystem == nullptr)	SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

	//packet and buffer sizes
	SendSize = 2 * 1024 * 1024;
	BufferSize = 2 * 1024 * 1024;

	//local endpoint
	LocalEndpoint = FIPv4Endpoint(FIPv4Address::Any, LocalPort);

	FIPv4Address::Parse(IP, RemoteAddress);
	//server endpoint
	RemoteEndpoint = FIPv4Endpoint(RemoteAddress, RemotePort);

	Socket = nullptr;

	if (SocketSubsystem != nullptr)
	{
		if (Socket == nullptr)
		{
			//similar to C#, we use an API to build the socket based on configuration parameters
			Socket = FUdpSocketBuilder(SocketDescription)
				.AsNonBlocking()
				.AsReusable()
				.BoundToEndpoint(LocalEndpoint)
				.WithSendBufferSize(SendSize)
				.WithReceiveBufferSize(BufferSize)
				.WithBroadcast();
		}
	}
}


void Audp_module::BeginPlay()
{
	Super::BeginPlay();

	
	Listen();

}

bool Audp_module::sendMessage(FString Message)
{
	if (!Socket) return false;
	int32 BytesSent;

	FTimespan waitTime = FTimespan(10);
	//this is where we create the packet, in this case by serialising the character array
	TCHAR* serializedChar = Message.GetCharArray().GetData();
	int32 size = FCString::Strlen(serializedChar);

	bool success = Socket->SendTo((uint8*)TCHAR_TO_UTF8(serializedChar), size, BytesSent, *RemoteEndpoint.ToInternetAddr());
	UE_LOG(LogTemp, Warning, TEXT("Sent message: %s : %s : Address - %s : BytesSent - %d  "), *Message, (success ? TEXT("true") : TEXT("false")), *RemoteEndpoint.ToString(), BytesSent);
	//UE_LOG lets us log error messages, not dissimilar to Debug.Log

	if (success && BytesSent > 0) return true;
	else return false;
}

void Audp_module::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	SocketSubsystem->DestroySocket(Socket);
	Audp_module::localNetObjects.Empty(); //here
	Socket = nullptr;
	SocketSubsystem = nullptr;
	singleton = nullptr; //it becomes null for next time we play (otherwise it’ll point to the previously destroyed version from the last session
	UNetworkGameObject::lastLocalID = 0;
	localNetObjects.Empty(); //this empties the array, so it’s clear next time we start playmode
}

void Audp_module::Listen()
{
	TSharedRef<FInternetAddr> targetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	uint32 Size;
	while (Socket->HasPendingData(Size))
	{

		uint8* Recv = new uint8[Size];
		int32 BytesRead = 0;

		ReceivedData.SetNumUninitialized(FMath::Min(Size, 65507u));
		Socket->RecvFrom(ReceivedData.GetData(), ReceivedData.Num(), BytesRead, *targetAddr);

		char ansiiData[4096];
		memcpy(ansiiData, ReceivedData.GetData(), BytesRead);
		ansiiData[BytesRead] = 0;

		FString data = ANSI_TO_TCHAR(ansiiData);

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, "Message by UDP:" + data);

		if (data.Contains("Assigned UID:")) 
		{

			FString message, info;
			//split off the 'Assigned UID:' bit, by delimiting at the :
			if (data.Split(TEXT(":"), &message, &info)) {
				FString lid, gid;
				//split into local and global ID, by delimiting at the ;
				if (info.Split(TEXT(";"), &lid, &gid)) {

					//the Atoi function is the equivalent of Int32.Parse in C#, converting a string to an int32
					int32 intGlobalID = FCString::Atoi(*gid);
					int32 intLocalID = FCString::Atoi(*lid);

					//iterate netObjects, find the one the local ID corresponds to, and assign the global ID
					for (UNetworkGameObject* netObject : Audp_module::localNetObjects) {
						if (netObject->GetLocalID() == intLocalID) {
							netObject->SetGlobalID(intGlobalID);
							UE_LOG(LogTemp, Warning, TEXT("Assigned: %d"), intGlobalID);
						}

					}

				}
			}

		}
		else if (data.Contains("Object data;")) {
			UE_LOG(LogTemp, Warning, TEXT("parsing state data"));
			bool foundActor = false;
			for (UNetworkGameObject* netObject : Audp_module::localNetObjects) {
				if (netObject->GetGlobalID() == netObject->GlobalIDFromPacket(data)) {
					if (!netObject->GetIsLocallyOwned()) {
						netObject->FromPacket(data);
					}
					foundActor = true;
				}
			}

			if (!foundActor) {
				UE_LOG(LogTemp, Warning, TEXT("spawning"));
				AActor* actor = GetWorld()->SpawnActor<AActor>(OtherPlayerAvatars, FVector::ZeroVector, FRotator::ZeroRotator);
				UNetworkGameObject* netActor = actor->FindComponentByClass<UNetworkGameObject>();
				netActor->SetGlobalID(netActor->GlobalIDFromPacket(data));
				netActor->FromPacket(data);
			}

		}


	}
}



