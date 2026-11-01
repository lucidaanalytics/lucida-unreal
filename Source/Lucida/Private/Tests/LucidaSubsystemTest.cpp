#include "LucidaSubsystem.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Run in the editor: Automation RunTests Lucida (the plugin CI does this headless with -nullrhi).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLucidaSubsystemOptInTest, "Lucida.Subsystem.OptInGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLucidaSubsystemOptInTest::RunTest(const FString&)
{
	ULucidaSubsystem* Lucida = NewObject<ULucidaSubsystem>();
	TestFalse(TEXT("no consent before Setup"), Lucida->HasConsent());
	TestTrue(TEXT("no id before Setup"), Lucida->GetDistinctId().IsEmpty());

	Lucida->Setup(TEXT("lc_pk_test"), TEXT("http://127.0.0.1:9"), /*bOptInMode=*/true);
	TestFalse(TEXT("opt-in mode starts without consent"), Lucida->HasConsent());
	Lucida->Capture(TEXT("ignored"), {}); // dropped: no consent yet
	Lucida->OptIn();
	TestTrue(TEXT("consent after OptIn"), Lucida->HasConsent());
	TestFalse(TEXT("an anonymous id exists"), Lucida->GetDistinctId().IsEmpty());
	Lucida->Identify(TEXT("user-42"));
	TestEqual(TEXT("identify sets the distinct id"), Lucida->GetDistinctId(), FString(TEXT("user-42")));
	Lucida->OptOut();
	TestFalse(TEXT("opt out revokes consent"), Lucida->HasConsent());
	Lucida->Deinitialize();
	return true;
}

#endif
