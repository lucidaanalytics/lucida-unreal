#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LucidaSubsystem.generated.h"

struct FLucidaState;

/**
 * Lucida for Unreal: one client per game instance. Call Setup once (C++ or Blueprint), then Capture.
 * All logic lives in the engine-free lucida::Client (sdks/cpp); this class only adapts HTTP (FHttpModule),
 * storage (GameUserSettings ini, written only after analytics consent) and the ticker / app lifecycle.
 */
UCLASS()
class LUCIDA_API ULucidaSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** bOptInMode: nothing is captured or stored before OptIn(). */
	UFUNCTION(BlueprintCallable, Category = "Lucida")
	void Setup(const FString& ApiKey, const FString& Host = TEXT("https://app.lucida.dev"), bool bOptInMode = false);

	UFUNCTION(BlueprintCallable, Category = "Lucida")
	void Capture(const FString& Event, const TMap<FString, FString>& Properties);

	UFUNCTION(BlueprintCallable, Category = "Lucida")
	void Screen(const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "Lucida")
	void Identify(const FString& UserId);

	UFUNCTION(BlueprintCallable, Category = "Lucida")
	void Reset();

	UFUNCTION(BlueprintCallable, Category = "Lucida")
	void OptIn();

	UFUNCTION(BlueprintCallable, Category = "Lucida")
	void OptOut();

	UFUNCTION(BlueprintCallable, Category = "Lucida")
	void Flush();

	UFUNCTION(BlueprintPure, Category = "Lucida")
	bool HasConsent() const;

	UFUNCTION(BlueprintPure, Category = "Lucida")
	FString GetDistinctId() const;

private:
	bool Tick(float DeltaTime);
	void OnBackground();
	void OnForeground();

	TSharedPtr<FLucidaState> State;
	FTSTicker::FDelegateHandle TickHandle;
	FDelegateHandle BackgroundHandle;
	FDelegateHandle ForegroundHandle;
};
