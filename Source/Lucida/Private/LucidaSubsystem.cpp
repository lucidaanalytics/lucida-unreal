#include "LucidaSubsystem.h"

#include <atomic>
#include <memory>

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "lucida/lucida.hpp"

IMPLEMENT_MODULE(FDefaultModuleImpl, Lucida)

namespace
{
std::string Utf8(const FString& S) { return std::string(TCHAR_TO_UTF8(*S)); }

/** lucida::Transport on FHttpModule. Completions after Deinitialize are ignored (the client is gone). */
class FLucidaTransport final : public lucida::Transport
{
public:
	std::shared_ptr<std::atomic<bool>> Alive = std::make_shared<std::atomic<bool>>(true);

	void post(const std::string& Url, const std::string& Body, std::function<void(int)> Done) override
	{
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
		Req->SetURL(UTF8_TO_TCHAR(Url.c_str()));
		Req->SetVerb(TEXT("POST"));
		Req->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		Req->SetContentAsString(UTF8_TO_TCHAR(Body.c_str()));
		std::shared_ptr<std::atomic<bool>> Flag = Alive;
		Req->OnProcessRequestComplete().BindLambda(
			[Flag, Done](FHttpRequestPtr, FHttpResponsePtr Resp, bool bOk)
			{
				if (*Flag) Done(bOk && Resp.IsValid() ? Resp->GetResponseCode() : 0);
			});
		Req->ProcessRequest();
	}
};

/** lucida::Storage on the GameUserSettings ini, section [Lucida]. The core writes only after consent. */
class FLucidaStorage final : public lucida::Storage
{
public:
	std::string get(const std::string& Key) override
	{
		FString Value;
		GConfig->GetString(TEXT("Lucida"), UTF8_TO_TCHAR(Key.c_str()), Value, GGameUserSettingsIni);
		return Utf8(Value);
	}
	void set(const std::string& Key, const std::string& Value) override
	{
		GConfig->SetString(TEXT("Lucida"), UTF8_TO_TCHAR(Key.c_str()), UTF8_TO_TCHAR(Value.c_str()), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
	void remove(const std::string& Key) override
	{
		GConfig->RemoveKey(TEXT("Lucida"), UTF8_TO_TCHAR(Key.c_str()), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
};
} // namespace

struct FLucidaState
{
	FLucidaTransport Transport;
	FLucidaStorage Storage;
	TUniquePtr<lucida::Client> Client;
};

void ULucidaSubsystem::Setup(const FString& ApiKey, const FString& Host, bool bOptInMode)
{
	if (State.IsValid() || ApiKey.IsEmpty()) return;
	State = MakeShared<FLucidaState>();
	lucida::Config Cfg;
	Cfg.api_key = Utf8(ApiKey);
	Cfg.host = Utf8(Host);
	Cfg.consent = bOptInMode ? lucida::ConsentMode::OptIn : lucida::ConsentMode::OptOut;
	Cfg.lib = "lucida-unreal";
	Cfg.super_props["$os"] = Utf8(UGameplayStatics::GetPlatformName());
	Cfg.super_props["$app_version"] = Utf8(FApp::GetBuildVersion());
	State->Client = MakeUnique<lucida::Client>(Cfg, State->Transport, State->Storage);
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ULucidaSubsystem::Tick), 0.5f);
	BackgroundHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &ULucidaSubsystem::OnBackground);
	ForegroundHandle = FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &ULucidaSubsystem::OnForeground);
	State->Client->capture("$app_opened");
}

void ULucidaSubsystem::Deinitialize()
{
	if (State.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
		FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundHandle);
		State->Client->capture("$app_closed");
		State->Client->shutdown(); // best effort: the request outlives us, its completion is ignored
		*State->Transport.Alive = false;
		State.Reset();
	}
	Super::Deinitialize();
}

void ULucidaSubsystem::Capture(const FString& Event, const TMap<FString, FString>& Properties)
{
	if (!State.IsValid()) return;
	lucida::Props P;
	for (const TPair<FString, FString>& Kv : Properties) P[Utf8(Kv.Key)] = Utf8(Kv.Value);
	State->Client->capture(Utf8(Event), P);
}

void ULucidaSubsystem::Screen(const FString& Name) { if (State.IsValid()) State->Client->screen(Utf8(Name)); }
void ULucidaSubsystem::Identify(const FString& UserId) { if (State.IsValid()) State->Client->identify(Utf8(UserId)); }
void ULucidaSubsystem::Reset() { if (State.IsValid()) State->Client->reset(); }
void ULucidaSubsystem::OptIn() { if (State.IsValid()) State->Client->opt_in(); }
void ULucidaSubsystem::OptOut() { if (State.IsValid()) State->Client->opt_out(); }
void ULucidaSubsystem::Flush() { if (State.IsValid()) State->Client->flush(); }
bool ULucidaSubsystem::HasConsent() const { return State.IsValid() && State->Client->has_consent(); }

FString ULucidaSubsystem::GetDistinctId() const
{
	return State.IsValid() ? FString(UTF8_TO_TCHAR(State->Client->distinct_id().c_str())) : FString();
}

bool ULucidaSubsystem::Tick(float)
{
	if (State.IsValid()) State->Client->tick();
	return true;
}

void ULucidaSubsystem::OnBackground()
{
	if (!State.IsValid()) return;
	State->Client->capture("$app_backgrounded");
	State->Client->flush();
}

void ULucidaSubsystem::OnForeground()
{
	if (State.IsValid()) State->Client->capture("$app_opened", {{"from_background", true}});
}
