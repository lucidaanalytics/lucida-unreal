# Lucida for Unreal Engine 5

Copy `Lucida/` into your project's `Plugins/` folder. Inside this repo the plugin compiles the header-only core from
`sdks/cpp/include`; outside it, copy `sdks/cpp/include` to `Plugins/Lucida/Source/Lucida/ThirdParty/include`.

```cpp
ULucidaSubsystem* Lucida = GetGameInstance()->GetSubsystem<ULucidaSubsystem>();
Lucida->Setup(TEXT("lc_pk_..."), TEXT("https://app.lucida.dev"), /*bOptInMode=*/true);
Lucida->OptIn();                                   // opt-in mode: nothing captured or stored before this
Lucida->Capture(TEXT("level_complete"), {{TEXT("level"), TEXT("3")}});
Lucida->Identify(TEXT("user-42"));
```

Every function is Blueprint-callable (category Lucida). The subsystem is thin glue: HTTP through `FHttpModule`,
storage in the GameUserSettings ini (`[Lucida]`, written only after analytics consent), a 0.5 s core ticker that
flushes, `$app_opened` / `$app_backgrounded` (flushes) / `$app_closed` from the core delegates. Batching, retry,
consent and uuid v7 live in `lucida::Client` (see `sdks/cpp`), which is what the tests cover.

Tests: the core is tested with `make -C sdks/cpp test`. The glue has an automation test
(`Source/Lucida/Private/Tests`, run in the editor with `Automation RunTests Lucida`).

## CI with the engine

The `lucida-unreal` repo's CI always checks the manifest and compiles the vendored core. Its `engine` job runs
`RunUAT BuildPlugin` (Linux Editor and Game targets) and the automation tests headless inside Epic's container
`ghcr.io/epicgames/unreal-engine:dev-slim-5.4`, once the `GHCR_EPIC_TOKEN` secret is set; without it the job only
prints a notice.

Epic GitHub access (the image is private to Epic's GitHub org):

1. Create an Epic Games account and, under Epic account settings > Apps and Accounts, connect your GitHub account.
2. Accept the email invitation to the `EpicGames` GitHub organization (it can take a few minutes after linking).
3. On GitHub, create a classic personal access token with the `read:packages` scope (fine-grained tokens cannot read
   another org's container packages).
4. In the `lucida-unreal` repo add the secret `GHCR_EPIC_TOKEN` (the token) and, when that GitHub account is not the
   one pushing, `GHCR_EPIC_USER` (its login). The Unreal EULA applies to the image; it is pulled, never published.

---

This repository is a read-only mirror of the Lucida monorepo (synced by a script, never edited by hand); releases and provenance attestations are built here by GitHub Actions. Issues are welcome, changes land upstream.
