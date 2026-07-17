// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPSCPPCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "TPSCPP.h"

ATPSCPPCharacter::ATPSCPPCharacter():
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		SessionInterface = OnlineSub->GetSessionInterface();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,                           
				15.f,                         
				FColor::Blue,                
				FString::Printf(TEXT("Found subsystem %s"),
					*OnlineSub->GetSubsystemName().ToString()) 
			);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No OnlineSubsystem found. Online features will be unavailable."));
	}
}

void ATPSCPPCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATPSCPPCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ATPSCPPCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATPSCPPCharacter::Look);
	}
	else
	{
		UE_LOG(LogTPSCPP, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ATPSCPPCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ATPSCPPCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ATPSCPPCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ATPSCPPCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ATPSCPPCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void ATPSCPPCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void ATPSCPPCharacter::CreateGameSession()
{
	//Called when pressing the 1 key
	if (!SessionInterface.IsValid())
	{
		return;
	}
	//                  
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}

	SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);
	//    SessionSettings      Session
	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
	SessionSettings->bIsLANMatch = false;
	SessionSettings->NumPublicConnections = 4;
	SessionSettings->bAllowJoinInProgress = true;
	SessionSettings->bAllowJoinViaPresence = true;
	SessionSettings->bShouldAdvertise = true;
	SessionSettings->bUsesPresence = true;
	SessionSettings->bUseLobbiesIfAvailable = true;
	SessionSettings->Set(FName("MatchType"),FString("FreeForAll"),EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);


	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateGameSession: LocalPlayer is null!"));
		return;
	}
	if (!LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("CreateGameSession: UniqueNetId is invalid! Steam may not be logged in."));
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("CreateGameSession: LocalPlayer=%s, NetId=%s"), *LocalPlayer->GetName(), *LocalPlayer->GetPreferredUniqueNetId()->ToString());
	SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings);
	
	// Travel to the lobby map as a listen server, so other players can join.
	// ServerTravel is async -- it starts loading the map while the session is being created in the OSS backend.
	if (GetWorld())
	{
		GetWorld()->ServerTravel("/Game/Maps/Lobby?listen");
	}
}

void ATPSCPPCharacter::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Blue, FString::Printf(TEXT("Create session: %s"),*SessionName.ToString()));
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("MatchType: FreeForAll")));
		}
		UE_LOG(LogTemp, Log, TEXT("Session [%s] created."), *SessionName.ToString());
	}
	else
	{
		// ---- Debug: failure info ----
		UE_LOG(LogTemp, Error, TEXT("OnCreateSessionComplete: Failed [%s]."), *SessionName.ToString());
		IOnlineSubsystem* Sub = IOnlineSubsystem::Get();
		FString SubName = Sub ? Sub->GetSubsystemName().ToString() : TEXT("null");
		UE_LOG(LogTemp, Error, TEXT("  OSS: %s"), *SubName);
		if (GetWorld())
		{
			const ULocalPlayer* LP = GetWorld()->GetFirstLocalPlayerFromController();
			if (LP)
				{
					int bNetIdValid = LP->GetPreferredUniqueNetId().IsValid() ? 1 : 0;
					UE_LOG(LogTemp, Error, TEXT("  LocalPlayer: %s, NetId valid: %d"), *LP->GetName(), bNetIdValid);
				}
			else
				UE_LOG(LogTemp, Error, TEXT("  LocalPlayer: NULL"));
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("Failed to create session!")));
		}
		// ---------------------------
	}

	FDelegateHandle Handle = CreateSessionCompleteDelegate.GetHandle();
	SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(Handle);
}

void ATPSCPPCharacter::JoinGameSession()
{
	// ==================== Find and join a multiplayer session ====================
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinGameSession: SessionInterface not valid."));
		return;
	}

	// Register the find-sessions completion delegate
	SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

	// Configure the search parameters
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->MaxSearchResults = 10000;
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(FName(TEXT("SEARCH_PRESENCE")), true, EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(FName("MatchType"), FString("FreeForAll"), EOnlineComparisonOp::Equals);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("JoinGameSession: LocalPlayer is null!"));
		return;
	}
	if (!LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("JoinGameSession: UniqueNetId is invalid!"));
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("JoinGameSession: starting search for LocalPlayer=%s"), *LocalPlayer->GetName());
	SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef());
}

void ATPSCPPCharacter::JoinSession()
{
	// ==================== Join a found session ====================
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinSession: SessionInterface not valid."));
		return;
	}

	// Register the join-completion delegate
	SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

	// Take the first search result and attempt to join it
	if (!SessionSearch.IsValid() || SessionSearch->SearchResults.Num() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSession: No search results to join."));
		FDelegateHandle H = JoinSessionCompleteDelegate.GetHandle();
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(H);
		return;
	}

	const FOnlineSessionSearchResult& SearchResult = SessionSearch->SearchResults[0];
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("JoinSession: LocalPlayer is null!"));
		FDelegateHandle H = JoinSessionCompleteDelegate.GetHandle();
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(H);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("JoinSession: attempting to join session [%s]"), *SearchResult.GetSessionIdStr());
	SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SearchResult);
}

void ATPSCPPCharacter::OnFindSessionsComplete(bool bWasSuccessful)
{
	// ==================== Session search completion callback ====================
	UE_LOG(LogTemp, Log, TEXT("OnFindSessionsComplete: bWasSuccessful=%d, results=%d"),
		bWasSuccessful ? 1 : 0,
		SessionSearch.IsValid() ? SessionSearch->SearchResults.Num() : 0);

	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("OnFindSessionsComplete: search failed or SessionSearch is null."));
		// Cleanup delegate
		FDelegateHandle Handle = FindSessionsCompleteDelegate.GetHandle();
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(Handle);
		return;
	}

	for (const auto& Result : SessionSearch->SearchResults)
	{
		FString Id = Result.GetSessionIdStr();
		FString User = Result.Session.OwningUserName;
		UE_LOG(LogTemp, Log, TEXT("  Found session: Id=%s, Owner=%s"), *Id, *User);
		FString MatchTypeValue;
		Result.Session.SessionSettings.Get(FName("MatchType"), MatchTypeValue);
		UE_LOG(LogTemp, Log, TEXT("    MatchType: %s"), *MatchTypeValue);
	}

	// Cleanup delegate
	FDelegateHandle Handle = FindSessionsCompleteDelegate.GetHandle();
	SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(Handle);
}

void ATPSCPPCharacter::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	// ==================== Join session completion callback ====================
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Log, TEXT("OnJoinSessionComplete: Successfully joined session [%s]."), *SessionName.ToString());

		// Resolve the connection string and travel to the server
		FString ConnectString;
		if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
		{
			UE_LOG(LogTemp, Log, TEXT("  Connect string: %s"), *ConnectString);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("Joining server: %s"), *ConnectString));
			}
			GetWorld()->GetFirstPlayerController()->ClientTravel(ConnectString, TRAVEL_Absolute);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("OnJoinSessionComplete: Failed to join session [%s]. Result=%d"), *SessionName.ToString(), (int32)Result);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("Failed to join session!")));
		}
	}

	// Cleanup delegate
	FDelegateHandle Handle = JoinSessionCompleteDelegate.GetHandle();
	SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(Handle);
}
