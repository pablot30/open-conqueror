# Tag once, sign offline, flash both boards

> **Published on 2026-11-16.** Written on 9 November 2026 and kept out of the repo until the maintainer decided to publish its possibly novel ideas openly. Unchanged since then apart from links into the repo, the final section, which now points to the [novelties](../novelties/README.md). Read it as background: where a [decision](../decisions/README.md) says otherwise, the decision wins.

Research date: 9 November 2026. Sources are cited inline. **[unverified]** marks a claim taken from general knowledge or a secondary source that no primary source confirmed during this research. **Estimate** marks design arithmetic, not measurement. Nothing in this report was built, flashed or run on the robot.

open-conqueror should ship **one SemVer number for the whole repo**, taken from a git tag such as `v0.3.0`. That number covers both firmwares, the calibration defaults and the Python tool, so a release is one tested set and D15 never has to check a matrix of component versions. Each build reads its version and commit from git, which meets D4. Pull requests are squash-merged under a **Conventional Commits title** that names a scope and the requirement IDs, and the changelog is generated from those titles. To release, the maintainer merges a generated changelog PR and pushes a tag. One Linux job then builds everything into a *draft* GitHub Release with a **project-defined two-board manifest**, a superset of the ESP Web Tools format. The maintainer signs that manifest offline with an **Ed25519 minisign key**, the same key the security report has the ESP32 check before any wireless update, and publishes. While the repo is private on GitHub Free, provenance attestations, environment approval gates and GitHub Pages are all unavailable, so integrity rests on checksums plus the offline signature. Going public adds them with almost no redesign. A release costs about **$0.50 of the $12 monthly CI allowance (estimate)**. Pull-request CI, not releases, is what strains the budget.

## One version number answers D4, D15 and D16 together

SemVer 2.0.0 gives MAJOR.MINOR.PATCH numbers, pre-releases such as `1.0.0-rc.1` that sort before the release, and build metadata after a `+` that doesn't affect ordering ([SemVer 2.0.0](https://semver.org/spec/v2.0.0.html) **[unverified, not re-fetched]**). That structure carries compatibility meaning, which D15 needs. Calendar versioning doesn't. ESPHome can use year.month.patch numbers ([esphome/firmware releases](https://github.com/esphome/firmware/releases)) because every user compiles their own single-board image, so nothing has to match. Projects that ship many binaries split their versions instead. ArduPilot tags each vehicle separately (`Copter-4.6.x`, `Plane-4.6.x`) ([ArduPilot tags](https://github.com/ArduPilot/ardupilot/tags) **[unverified, not re-fetched]**), and Tasmota names its assets by variant ([Tasmota releases](https://github.com/arendst/Tasmota/releases)). This robot is one product with two boards that must agree, so a **single repo-wide version** is simpler. Per-component versions would create exactly the compatibility matrix that D15 would then have to police.

The version should come from git, never from a file someone forgets to bump. `git describe --tags --dirty` prints the tag on a tagged commit and `<tag>-<n>-g<sha>` between releases ([git-describe](https://git-scm.com/docs/git-describe) **[unverified, not re-fetched]**), so one string meets D4 on every build. A small Python script run from the `just` file should turn it into a generated header for both boards and the host tests, identically on all three operating systems. On the ESP32-S3, ESP-IDF fills `PROJECT_VER` in the image's app description from `git describe` when no version is set, so the version can be read at runtime or from the file ([ESP-IDF app version](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/misc_system_api.html) **[unverified, not re-fetched]**). The script should set `PROJECT_VER` explicitly so both boards print the same string. On the UNO, arduino-cli accepts extra defines through `--build-property build.extra_flags=...` ([compile.go](https://github.com/arduino/arduino-cli/blob/v1.5.1/internal/cli/compile/compile.go)), but a generated header avoids quoting differences between shells. Flash matters here. The stock UNO image fills 98% of flash, and requirement T8 caps ours at 80% ([requirements](../requirements.md)). So the UNO should store three version bytes, a 4-byte short commit and a dirty flag, not a 30-character string. On the Python side, hatch-vcs and setuptools-scm derive the version from the same tag ([setuptools-scm](https://setuptools-scm.readthedocs.io/en/stable/), [hatch-vcs](https://pypi.python.org/pypi/hatch_vcs)), so `uv build` needs no manual edit. One spelling trap needs a test: Python's PEP 440 writes `1.2.0rc1` where SemVer writes `1.2.0-rc.1` ([PEP 440](https://peps.python.org/pep-0440/) **[unverified, not re-fetched]**). The manifest should record the SemVer form and let the tool normalise.

D4 has a second sentence: each software unit, such as a driver, has its own version too ([requirements](../requirements.md)). Git can't produce that, and it shouldn't try. Each unit should carry a hand-maintained integer **interface version** that changes only when the unit's behaviour or interface changes. The robot reports these as a table alongside the release version. D15's checks then key on three values: the **link protocol version and description hash** for the UNO and ESP32 pair (as the security report specifies), the **tool protocol version** for the computer tool, and the **calibration schema hash** for calibration sets. A patch release then never breaks the tool by accident, and `0.x` versions can move freely while stage 2 settles, as SemVer allows ([SemVer 2.0.0](https://semver.org/spec/v2.0.0.html) **[unverified]**).

## Squash-merged pull request titles become the changelog

Conventional Commits uses the form `type(scope)!: description`, where `fix` maps to a patch, `feat` to a minor and `!` or a `BREAKING CHANGE:` footer to a major ([Conventional Commits 1.0.0](https://www.conventionalcommits.org/en/v1.0.0/) **[unverified, not re-fetched]**). Keep a Changelog adds a human-readable layout with an Unreleased section and Added, Changed, Fixed and Security groups ([Keep a Changelog 1.1.0](https://keepachangelog.com/en/1.1.0/) **[unverified, not re-fetched]**). The two fit together if every merged change produces exactly one conventional commit. **Squash merges** guarantee that. release-please "highly recommend[s]" them, because otherwise a branch's intermediate commits leak into the notes ([release-please](https://github.com/googleapis/release-please)), and GitHub can default the squash message to the PR title and description ([GitHub Docs](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/configuring-pull-request-merges/configuring-commit-squashing-for-pull-requests) **[unverified, not re-fetched]**). Only the PR title then has to be right, which a cheap CI check enforces and which humans and agents get right more easily than every commit on a branch. Q6 traceability comes free: a `Refs: D16, Q4` trailer in the PR description lands in the squash commit, so every changelog line can be traced to a requirement.

| Tool | How it works | Fit here |
|---|---|---|
| **git-cliff** | Single Rust binary. Templates, path filters, reads commit messages ([git-cliff monorepos](https://www.mintlify.com/orhun/git-cliff/usage/monorepos)) | **Recommended now.** Runs from `just`. No bot, token or extra workflow |
| release-please | Bot keeps a standing release PR. Merging it bumps the changelog and version, tags and creates the Release. `simple` type plus `extra-files` markers for any file ([release-please](https://github.com/googleapis/release-please)) | Good later. But its PR gets no CI under `GITHUB_TOKEN` without a personal access token ([release-please-action](https://github.com/googleapis/release-please-action)) |
| semantic-release | Publishes on every qualifying push, with no release PR. Node-based ([semantic-release](https://github.com/semantic-release/semantic-release) **[unverified]**) | Poor. No human gate before release |
| changesets | One change file per PR, aimed at JavaScript monorepos ([ToolRadar](https://toolradar.com/compare/changesets-vs-semantic-release), secondary) | Poor. Wrong ecosystem |
| towncrier | One news-fragment file per PR, Python-based ([towncrier](https://towncrier.readthedocs.io/) **[unverified]**) | Workable, but adds a file that agents forget |

release-please is the more automated choice, and the bot only proposes, so it respects "the maintainer merges". Its catch is a GitHub rule: "events triggered by the `GITHUB_TOKEN` will not create a new workflow run" ([GitHub Docs](https://docs.github.com/en/actions/concepts/security/github_token)). The release PR therefore gets no CI unless the project stores a personal access token, which the action's README recommends, and the repo may also need "Allow GitHub Actions to create and approve pull requests" switched on ([release-please-action](https://github.com/googleapis/release-please-action)). That is one more long-lived secret in a repo with no environment protection (see below). **git-cliff run locally** avoids it. An agent runs `just release-prep 0.3.0`, which writes the changelog section and opens an ordinary PR that CI checks like any other. The maintainer merges it and pushes the tag, and a tag pushed by a person triggers workflows normally. The existing pre-push hook allows it, because it blocks only pushes to `main`, deletions and history rewrites ([pre-push](../../.githooks/pre-push)). The hook also means a mistaken tag can't be quietly deleted, which suits immutable releases.

## A draft release, an offline signature, then the robot

The release has to stop for the maintainer anyway. The security report keeps the Ed25519 release key on an offline, password-protected USB stick, and lists a CI-held key behind a protected GitHub environment only as an optional convenience. That convenience **isn't available while private**: "Users with GitHub Free plans can only configure environments for public repositories" ([GitHub Docs](https://docs.github.com/en/actions/how-tos/deploy/configure-and-manage-deployments/manage-environments)). So CI builds and the maintainer signs. Two features then make the draft trustworthy before it is signed. `SOURCE_DATE_EPOCH`, set from the last commit time, replaces the build date ([reproducible-builds.org](https://reproducible-builds.org/specs/source-date-epoch/) **[unverified]**), and the firmware must avoid `__DATE__` and `__TIME__`. GitHub's **immutable releases** went GA on 28 October 2025 ([GitHub changelog](https://github.blog/changelog/2025-10-28-immutable-releases-are-now-generally-available/)). Once a release is published, its tag is locked to its commit and its assets can't be changed or deleted. GitHub recommends creating a draft, attaching every asset, then publishing, and it generates a release attestation automatically ([GitHub Docs](https://docs.github.com/en/code-security/supply-chain-security/understanding-your-software-supply-chain/immutable-releases)). Whether immutable releases work on a private Free repo is **[unverified]**.

The recommended path from commit to flashed robot:

| # | Step | Who and where | Output |
|---|---|---|---|
| 1 | Branch, commit with `Signed-off-by` (human) or `Assisted-by` (agent), open a PR titled `feat(uno): …` with `Refs:` IDs | Contributor or agent | PR |
| 2 | PR gates: title lint, DCO check, both firmware builds, host tests and coverage, static analysis, manifest schema test | CI, Linux | Pass or fail (Q4) |
| 3 | Squash-merge | Maintainer | One conventional commit on `main` |
| 4 | `just release-prep X.Y.Z`: git-cliff writes the changelog section, PR `chore(release): vX.Y.Z` | Agent, then maintainer merges | Changelog on `main` |
| 5 | `just release-tag`: annotated tag on that merge commit, pushed | Maintainer | `vX.Y.Z` |
| 6 | Tag workflow refuses a dirty tree or a mismatched version. Builds the UNO `.hex`, the ESP32-S3 bootloader, partition table, app and merged factory image (offsets from the build's own flash arguments), the wheel and sdist. Copies calibration defaults. Writes `manifest.json`, `SHA256SUMS` and an ESP Web Tools manifest. Builds firmware twice and compares hashes. Reports flash use against T8 | CI, one Linux job | Draft GitHub Release |
| 7 | `just release-verify X.Y.Z`: download, check `SHA256SUMS`, optionally rebuild locally and compare, then `minisign -S` on `manifest.json` and upload `manifest.json.minisig` | Maintainer, offline key | Signed package |
| 8 | Publish the release | Maintainer | Immutable release (if available) |
| 9 | `oc flash open-conqueror-X.Y.Z.zip`: verify signature and hashes, check compatibility, flash the ESP32-S3 with esptool, guide the switch to Upload, flash the UNO, write calibration defaults, then ask both boards for version and commit and compare with the manifest | Owner's computer, with the maintainer's OK for this robot | Robot running exactly release X.Y.Z (D16) |
| 10 | Later, over Wi-Fi: the ESP32 verifies the same signed manifest and programs itself and the UNO | Robot | Wireless update (D2, D3, S12) |

Step 9 needs care because USB and the ESP32 share the UNO's serial line and the Upload switch needs a hand ([hardware](../hardware/README.md)). The tool should run one guided two-step session and refuse to finish until both boards report the manifest's commit, mirroring how ESP Web Tools uses the name and version a device reports over Improv to recognise installed firmware ([ESP Web Tools](https://esphome.github.io/esp-web-tools/)). esptool is a Python package, so it can be a locked dependency of the tool. The UNO side has two options. avrdude is a native binary that each OS would need to install, and a winget package for it is **[unverified]**. The alternative is for the tool to speak STK500v1, the small serial protocol Optiboot uses. The security report already has the ESP32 implement it as the UNO's wireless programmer, so a Python version could share test vectors with the firmware one.

## Our own manifest extends ESP Web Tools to cover the AVR

The ESP world has settled on two image shapes: a **merged factory image** written at offset 0 for first install, and an **app-only image** for OTA ([ESPHome guide](https://esphome.io/guides/migrate_sonoff_tasmota/), [WLED releases](https://github.com/wled/WLED/releases)). esptool's `merge-bin` produces raw, Intel HEX or UF2 output, and its current docs use hyphenated subcommands, consistent with the esptool v5 rename ([esptool docs](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/esptool/basic-commands.html)). ESP Web Tools defines a JSON manifest with `name`, `version` and `builds[]` keyed by `chipFamily`, each with `parts[]` of `path` and `offset`. ESP32-S3 is a supported family, but every family is an Espressif chip ([ESP Web Tools](https://esphome.github.io/esp-web-tools/)). The format has no AVR target, no checksums, no commit and no compatibility data. ArduPilot's `manifest.json` adds a git SHA, a release type and a format per file ([ArduPilot firmware server](https://firmware.ardupilot.org/) **[unverified, not re-fetched]**), but the notes found **no open manifest standard that covers an AVR and an ESP32 in one release** (the search wasn't exhaustive). The security report already recommends a small signed JSON manifest borrowing SUIT's field names, Uptane's hardware identifier and MCUboot's dependency rule. The sketch below combines that with the ESP Web Tools shape, so the ESP32 part stays familiar:

```json
{
  "schema": "open-conqueror/release-manifest/1",
  "name": "open-conqueror",
  "version": "0.3.0",
  "sequence": 7,
  "created": "2026-12-01T18:04:11Z",
  "git": { "commit": "<40-hex>", "describe": "v0.3.0", "dirty": false },
  "vendor_id": "<uuid>",
  "class_id": "conqueror-tank/v1",
  "compat": {
    "link_protocol": 3,
    "link_description_hash": "<4-byte hex>",
    "tool_protocol": 2,
    "tool_min_version": "0.3.0",
    "calibration_schema_hash": "<hex>"
  },
  "targets": [
    {
      "id": "uno", "hardware_id": "atmega328p/conqueror-shield",
      "mcu": "ATmega328P", "flasher": "stk500v1@115200",
      "parts": [ { "path": "uno/firmware.hex", "format": "ihex",
                   "size": 21504, "sha256": "<hex>" } ],
      "units": { "motor_driver": 2, "line_sensor": 1 }
    },
    {
      "id": "esp32", "hardware_id": "esp32s3/conqueror-camera",
      "chipFamily": "ESP32-S3", "flasher": "esptool",
      "parts": [
        { "path": "esp32/factory.bin", "offset": 0, "format": "bin",
          "size": 1310720, "sha256": "<hex>", "use": "usb" },
        { "path": "esp32/app.bin", "format": "bin",
          "size": 1179648, "sha256": "<hex>", "use": "ota" }
      ],
      "units": { "camera": 1, "uno_programmer": 1 }
    }
  ],
  "data": [
    { "id": "calibration-defaults", "path": "calibration/defaults.json",
      "schema_hash": "<hex>", "sha256": "<hex>" }
  ]
}
```

Next to `manifest.json`, the zip holds `manifest.json.minisig`, `SHA256SUMS`, `CHANGELOG.md`, `LICENSE` and a generated `esp-web-tools.json` that lists only the ESP32-S3 factory image. The Python wheel is a separate release asset. The signature covers the manifest file's exact bytes, so no JSON canonicalisation is needed, and every image is bound to the signature through its hash. `SHA256SUMS` lets anyone check the files with built-in tools on every OS (`shasum -a 256`, `sha256sum`, PowerShell `Get-FileHash`). The separate ESP Web Tools file exists because whether ESP Web Tools tolerates unknown keys is **[unverified]**. Offsets must come from the build's generated flash arguments, never from examples. The ESP32-S3's bootloader sits at 0x0, unlike the classic ESP32's 0x1000 **[unverified, not re-fetched]**, and both the esptool S3 page and the ESP Web Tools page show 0x1000 examples ([esptool docs](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/esptool/basic-commands.html), [ESP Web Tools](https://esphome.github.io/esp-web-tools/)). The ESP32 has to verify the signature on the robot, so its verifier must match the exact minisign signature variant the maintainer's minisign produces. That variant is **[unverified]** and belongs in the negative-test suite the security report asks for. Sizes and the `units` values above are placeholders.

## Going public switches on provenance at almost no cost

GitHub artifact attestations record the workflow, repo, commit and trigger, and on their own they reach **SLSA Build Level 2**. Building in a reusable workflow raises that to Level 3 ([GitHub Docs](https://docs.github.com/en/actions/concepts/security/artifact-attestations)). On Free, Pro or Team they work **only on public repos**, and private repos need Enterprise Cloud ([GitHub Docs](https://docs.github.com/en/actions/how-tos/secure-your-work/use-artifact-attestations/use-artifact-attestations)). Public repos sign through the Sigstore public-good instance, with a public transparency log ([GitHub Docs](https://docs.github.com/en/actions/concepts/security/artifact-attestations)). Keyless cosign also works from a private repo, but it would write the repo and workflow identity to that public log **[unverified]**, which defeats staying private. The offline minisign key stays the robot's root of trust either way. Provenance says *which workflow built it*, and the signature says *the maintainer approved it*. The robot can check only the second.

| Area | While private (until first working firmware, [0005](../decisions/0005-publish-with-first-working-firmware.md)) | Once public |
|---|---|---|
| Version, tags, changelog | As recommended | Same |
| Integrity | `SHA256SUMS` plus per-file SHA-256 in the manifest | Same |
| Authenticity | Offline minisign Ed25519 key | Same. A CI key behind an environment with required reviewers becomes possible |
| Provenance | None (attestations need Enterprise Cloud) | `actions/attest-build-provenance` on the release job. Reusable workflow for SLSA L3 |
| Immutable releases | Turn on if the setting exists **[unverified on private Free]** | Turn on |
| SBOM | Optional file in the package | Attached and attested |
| Downloads | Need GitHub authentication, so the tool uses the owner's `gh` login **[inference]** | Anonymous. Robot-side OTA fetch becomes possible |
| Docs | Built docs zip on the release. Pages needs a paid plan for private repos ([GitHub Docs](https://docs.github.com/en/pages/getting-started-with-github-pages/about-github-pages) **[unverified]**) | GitHub Pages |
| Browser flasher | None. ESP Web Tools needs an HTTPS page ([ESP Web Tools](https://esphome.github.io/esp-web-tools/)) | ESP32-S3 only, via Pages. The UNO still needs the tool |
| Merge gates (Q4) | Pre-push hook and maintainer discipline, recorded as a gap | Apply the prepared ruleset ([protect-main.json](../../.github/rulesets/protect-main.json)) with required checks and CODEOWNERS |
| DCO | Check runs, but only maintainer and agents contribute | DCO app or action as a required check |
| CI matrix | Linux per PR. Windows and macOS for the tool on tags and weekly | All three OSes per PR, free |

## DCO sign-off fits Apache-2.0 and keeps humans accountable for agents

Apache-2.0's section 5 already places intentional contributions under the licence unless stated otherwise ([Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0) **[unverified, not re-fetched]**). A Contributor License Agreement would add a separately signed legal document, a known barrier to entry. The **Developer Certificate of Origin** adds only a `Signed-off-by:` line per commit, which a GitHub app or action checks on every PR ([opensource.com](https://opensource.com/article/18/3/cla-vs-dco-whats-difference), [FINOS](https://community.finos.org/docs/governance/Software-Projects/dco)). Chef and LoopBack both moved from a CLA to the DCO ([Chef](https://www.chef.io/blog/introducing-developer-certificate-of-origin), [LoopBack](https://loopback.io/blog/switching-to-dco)). AI agents raise one question the notes didn't settle. The Linux kernel's guidance says only humans can legally certify the DCO. The newest versions say agents must not add `Signed-off-by`, and agents disclose themselves with an `Assisted-by:` tag, while older versions only make sign-off the human's job ([kernel.org](https://docs.kernel.org/7.1/process/coding-assistants.html); wording varies by version). QEMU went further and banned AI-generated code in 2023 because its maintainers didn't think DCO compliance was credible for it. A 2026 proposal would relax the ban, with sign-off staying the contributor's personal responsibility ([QEMU 2024 patch](https://patchew.org/QEMU/20240516162230.937047-1-berrange@redhat.com/20240516162230.937047-4-berrange@redhat.com), [2026 proposal](https://patchew.org/QEMU/20260529094619.1034458-1-pbonzini@redhat.com/)). Whether that proposal was merged is **[unverified]**. For this project, agents should never sign off. The maintainer (or a human outside contributor) signs off the squash commit for agent work and takes responsibility for it, and the agent's trailer records its part.

The rest stays light: trunk-based short branches, with a `release/0.x` branch only when an old release needs a fix after `main` has moved on. A PR template asks for the requirement IDs, whether a claim was measured on the robot, whether the robot was flashed or run and who approved it, and whether `docs/tools.md` changed. CODEOWNERS sits in `.github/`, and its last matching pattern wins. GitHub enforces code-owner review only through branch protection or rulesets, which private repos get only on paid plans ([GitHub Docs](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-code-owners) **[unverified]**). Required checks should have stable names now, so the ruleset can name them on publication day.

## Releases cost cents. Pull requests spend the budget

GitHub Free includes **$12 of Actions compute a month** for private repos, at $0.002 a minute for Linux 1-core, $0.006 for Linux 2-core, $0.010 for Windows and $0.062 for macOS. Usage is blocked rather than billed when the money runs out unless a payment method is on file. Artifacts and Packages share 500 MB, and each repo gets 10 GB of cache. Standard runners are free on public repos ([GitHub billing](https://docs.github.com/en/billing/concepts/product-billing/github-actions)).

| Job (estimate, not measured) | Runner | Minutes | Cost |
|---|---|---|---|
| PR title lint, DCO, manifest schema | Linux 1-core | 1 | $0.002 |
| Release build, both firmwares built twice, wheel, manifest | Linux 2-core | 20 | $0.12 |
| Tool tests on release tags | Windows | 5 | $0.05 |
| Tool tests on release tags | macOS | 5 | $0.31 |
| **One release** | | | **≈ $0.48 (4% of $12)** |

Even weekly releases would use under a fifth of the allowance. The pressure comes from per-PR builds, which the code-quality and toolchain reports already shape: Linux on every PR, the ESP-IDF build only when its paths change, Windows and macOS less often. The release design adds three habits: cancel superseded runs with `concurrency`, keep CI artifacts for three to seven days to stay under 500 MB, and build firmware only on Linux, since the cross-compilers don't care about the host. Release assets probably don't count against the 500 MB **[unverified]**. The 1-core runner's label wasn't checked. Every step also runs locally through `just`, so CI is a convenience and never stands between the maintainer and a release.

## Gaps stated honestly

Several claims rest on general knowledge rather than fetched sources: SemVer, Conventional Commits, Keep a Changelog, `git describe`, `SOURCE_DATE_EPOCH`, minisign and cosign details, the ESP-IDF version field and the S3 bootloader offset. Each is flagged inline. Whether immutable releases and the 1-core runner are available on a private Free repo is unknown. **Cross-machine reproducibility is untested.** The UNO's avr-gcc runs under Rosetta on the maintainer's Mac and natively on Linux, and nobody has compared their output, so step 7's local rebuild might not match CI byte for byte. If it doesn't, the maintainer verifies CI's checksums and signs those. The ESP Web Tools tolerance for extra keys, Firefox's Web Serial support, esptool v5's old command aliases, avrdude packaging on Windows and the exact minisign variant for the on-robot verifier are all open. The notes found no SBOM generator suited to firmware, and no data on how reliably AI agents follow commit conventions. The two-board manifest has no direct precedent, so its schema needs its own tests.

## Decisions for the maintainer

1. **Version scheme:** one repo-wide SemVer from git tags (recommended), or per-component versions. Also: start at `0.1.0` and reserve `1.0.0` for publication, or some other milestone.
2. **Per-unit versions (D4):** hand-maintained integer interface versions per unit, reported as a table (recommended), or another scheme.
3. **Merge strategy:** squash merges only, with the squash message defaulting to the PR title and description (recommended). Disable merge commits and rebase merges in repo settings.
4. **Changelog tool:** git-cliff run locally through `just release-prep` (recommended now), or release-please with a personal access token. Revisit at publication.
5. **Who tags:** the maintainer only, by hand on the merged release-prep commit (recommended), or a workflow.
6. **Signing flow:** CI builds a draft, then the maintainer verifies and signs offline (recommended). Also decide whether a local rebuild must match CI before signing, or a checksum review is enough.
7. **Manifest:** adopt the two-board superset sketch as schema version 1, generate the separate ESP Web Tools file, and choose the calibration-defaults format and schema hash with the link and calibration work.
8. **UNO flashing in the tool:** our own Python STK500v1 shared with the ESP32 programmer's test vectors (recommended), or avrdude installed through each OS's package manager.
9. **Contributor terms:** DCO with a check (recommended), no agent sign-offs, and `Assisted-by:` for agents. Or a CLA, or nothing.
10. **Immutable releases:** turn them on now if a private Free repo offers the setting, otherwise on publication day.
11. **Publication-day release changes:** provenance attestations, an SBOM, the ruleset with required checks, Pages docs and a browser flasher for the ESP32-S3. Decide which go live the same day.
12. **Release cadence and pre-releases:** tag `-rc.N` builds for robot testing before each release, or release directly. Who approves flashing an RC onto the robot.

## Conclusion

Release engineering turned out to be the same question as security. Once the ESP32 verifies one signed manifest before touching either board, the manifest stops being packaging metadata. It becomes the contract that D4, D15 and D16 all test against, and the build, the changelog and the flashing tool exist to produce it and honour it. That is why one version beats many, and why the maintainer's offline signature matters more than any GitHub feature. Provenance says where bytes came from, but only the signature tells the robot to trust them.

The private phase is also less of a handicap than it looks. Every missing feature (attestations, environments, Pages, rulesets) adds to a design that already works without it, so publication becomes switching on settings rather than redesigning anything. The real risk is quieter. Until GitHub enforces merge gates, Q4 depends on the maintainer's discipline, and several AI agents raising pull requests in parallel will test that discipline before any outside contributor does.

## Possibly novel ideas

None new in this report. Every recommendation combines established tools and practices: SemVer from git tags, Conventional Commits, git-cliff, the DCO, detached Ed25519 signatures, and a manifest that extends ESP Web Tools' public format. The release flow depends on the security report's idea 3, a two-board release that always ends as a matched pair, published as novelty [N3](../novelties/N3-two-boards-that-always-match.md).
