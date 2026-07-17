# MultiplayerSessions

A UE5 plugin that wraps the Online Subsystem session API into a reusable, event-driven pattern. Handles session creation, discovery, joining, and destruction with automatic retry on join failure.

---

## Overview

The plugin provides two core classes that work together:

| Class | Role | Lifetime |
|---|---|---|
| UMultiplayerSessionsSubsystem | Session logic layer | GameInstance (persists across levels) |
| UMenu | UI layer | As needed (created/destroyed per menu) |

UMultiplayerSessionsSubsystem is a UGameInstanceSubsystem. It is created automatically when the game starts and lives until the game ends. UMenu is a UUserWidget that you create and show when needed; it binds to the subsystem broadcast delegates to receive results.

---

## How to Use

### 1. Enable the plugin

Make sure MultiplayerSessions is enabled in your project .uproject Plugins list.

### 2. Create and show the menu widget

`cpp
UMenu* Menu = CreateWidget<UMenu>(GetWorld(), MenuClass);
Menu->SetLobbyMapPath(TEXT("/Game/Maps/MyLobby"));
Menu->MenuSetup(4, TEXT("FreeForAll"));
`

MenuSetup can also take the lobby path directly:

`cpp
Menu->MenuSetup(4, TEXT("FreeForAll"), TEXT("/Game/Maps/MyLobby"));
`

### 3. Done

The menu automatically handles: host button -> create session -> travel to lobby, join button -> find sessions -> auto-join -> travel to server, retry logic, button disable/enable during async ops, cleanup on destroy.

---

## API Reference

### UMultiplayerSessionsSubsystem

Public Functions:

| Function | Description |
|---|---|
| CreateSession(NumPublicConnections, MatchType) | Create a multiplayer session. If one exists, destroy first and defer. |
| FindSessions(MaxSearchResults) | Search for public sessions. |
| JoinSession(SessionResult) | Join a specific session from search results. |
| DestroySession() | Destroy the current session. |
| StartSession() | Stub -- not implemented. |
| GetSessionInterface() | Returns the cached IOnlineSessionPtr. |

Broadcast Delegates (native multicast, bind with AddUObject):

| Delegate | Signature | Fires On |
|---|---|---|
| MultiplayerOnCreateSessionComplete | bool bWasSuccessful | CreateSession finishes |
| MultiplayerOnFindSessionsComplete | bool, const TArray<FOnlineSessionSearchResult>& | FindSessions finishes |
| MultiplayerOnJoinSessionComplete | EOnJoinSessionCompleteResult::Type Result | JoinSession finishes |
| MultiplayerOnDestroySessionComplete | bool bWasSuccessful | DestroySession finishes |

### UMenu

Public Functions:

| Function | Description |
|---|---|
| MenuSetup(NumPublicConnections, MatchType, LobbyPath) | Initialize: store params, add to viewport, bind delegates. |
| SetLobbyMapPath(LobbyPath) | Override lobby map path. listen appended automatically. |
| MenuTearDown() | Remove from viewport, restore game input mode. |

Button callback flow (HostButtonClicked -> Subsystem->CreateSession -> OncreateSession -> ServerTravel on success, enable buttons on failure). JoinButtonClicked -> Subsystem->FindSessions -> OnFindSessions -> cache and auto-join first result -> OnJoinSession -> ClientTravel on success, retry next on failure.

---

## Session Configuration

CreateSession applies these settings automatically: bIsLANMatch auto-detected (true for Null, false for Steam), NumPublicConnections from caller, bAllowJoinInProgress true, bAllowJoinViaPresence true, bShouldAdvertise true, bUsesPresence true, bUseLobbiesIfAvailable true, BuildUniqueId 1. MatchType advertised via ViaOnlineServiceAndPing.

---

## Dependencies

OnlineSubsystem (public), OnlineSubsystemSteam (public), UMG (public), Slate/SlateCore (public via UMG). The uplugin file declares OnlineSubsystem and OnlineSubsystemSteam as plugin-level dependencies.

---

## Edge Cases Handled

| Scenario | Behavior |
|---|---|
| CreateSession when session exists | Destroy old first, then create (deferred via bCreateSessionOnDestroy) |
| JoinSession fails (multiple results) | Auto-retry next cached result |
| All join attempts fail | Enable buttons, show all failed message |
| Search returns 0 results | Enable buttons, show no sessions found |
| Widget destroyed during travel | NativeDestruct -> MenuTearDown -> input mode restored |
| Null subsystem (no Steam) | bIsLANMatch set to true automatically |

---

## Notes

- UMultiplayerSessionsSubsystem is a GameInstanceSubsystem. Access via GetGameInstance()->GetSubsystem.
- UMenu button properties (HostButton, JoinButton) use meta = (BindWidget). Create matching-named buttons in UMG Blueprint.
- All source comments are in English.
