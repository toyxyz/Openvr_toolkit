# AGENTS.md

## Purpose

This file defines the operating rules for AI coding agents working in this repository.

The goal is to keep the codebase modular, maintainable, explicit, and resilient across long-running AI-assisted development sessions. Agents must follow these rules unless the user explicitly overrides them.

---

## 1. Core Development Principles

### 1.1 Build modular first

- Keep modules small, focused, and replaceable.
- Do not allow any manually written code file to exceed **300 lines**.
- Documentation, design notes, plans, generated files, vendored dependencies, and logs may exceed this limit when justified.
- Do not reduce implementation quality or scope because of the line limit.
- If more code is required, split the implementation into smaller files, modules, classes, or functions.

### 1.2 Keep entrypoints thin and stable

Entrypoints should coordinate application startup only.

They should not contain:

- business logic
- UI layout details
- rendering logic
- device polling logic
- serialization logic
- platform-specific implementation details
- large configuration procedures

Entrypoints should delegate to clearly named modules.

Preferred pattern:

```text
main / app entrypoint
  -> application bootstrap
  -> service initialization
  -> UI layer
  -> domain logic
  -> platform adapters
```

### 1.3 Think ahead before writing code

Do not write code that is already known to require structural replacement later.

When a future change is likely:

- isolate the unstable part behind a function, class, interface, adapter, or module boundary
- keep public entrypoints stable
- record important architectural choices in `CONTINUITY.md`
- avoid mixing temporary implementation details with long-term APIs

Temporary code is allowed only when it is clearly marked and tracked.

Use comments such as:

```text
TODO(date, owner/context): reason and intended replacement path
```

Do not leave vague comments such as:

```text
TODO: fix later
```

---

## 2. File Size and Modularity Rules

### 2.1 300-line rule

No manually written code file should exceed **300 physical lines**.

This includes:

- source files
- headers
- scripts
- UI implementation files
- tests, unless explicitly exempted

This excludes:

- generated files
- vendored third-party code
- lockfiles
- documentation
- data files
- build artifacts

If a file must exceed the limit temporarily, record the exception in `CONTINUITY.md` under `Open Questions`, `Decisions`, or `Incidents`.

### 2.2 Split by responsibility, not arbitrarily

Do not split files mechanically just to satisfy the line limit.

Split by clear responsibility:

```text
ui/
  panels/
  widgets/
  view_models/

domain/
  models/
  services/
  use_cases/

platform/
  win32/
  openvr/
  filesystem/

rendering/
  renderer/
  shaders/
  resources/

io/
  import/
  export/
  serialization/
```

### 2.3 Avoid god objects and god files

Do not create files, classes, or modules that become catch-all containers.

Avoid names like:

```text
Utils
Helpers
Manager
Common
Misc
Everything
AppState
```

These names are allowed only when the contained responsibility is narrow and documented.

Prefer specific names:

```text
PathNormalizer
OpenVrDevicePoller
FbxAsciiWriter
TrackerPoseBuffer
ImGuiDevicePanel
```

---

## 3. Failure Handling Rules

### 3.1 Fail fast during development

During development, do not hide failures with silent fallbacks.

Avoid fallback behavior for:

- missing required configuration
- failed initialization
- failed file IO
- missing devices
- invalid data
- dependency loading failures
- graphics or runtime setup failures

Bad:

```cpp
auto config = loadConfig().value_or(DefaultConfig{});
```

Better:

```cpp
auto config = loadRequiredConfig(configPath);
```

Allowed defaults must be:

- intentional
- explicit
- documented
- tested

### 3.2 No empty catch blocks

Never leave empty `try-catch` blocks.

Bad:

```cpp
try {
    loadPlugin(path);
} catch (...) {
}
```

Acceptable:

```cpp
try {
    loadPlugin(path);
} catch (const PluginLoadError& error) {
    logger.warn("Optional plugin skipped: {}", error.what());
}
```

or:

```cpp
try {
    loadRequiredConfig(path);
} catch (const std::exception& error) {
    logger.error("Failed to load required config: {}", error.what());
    throw;
}
```

Every caught exception must be handled by at least one of the following:

- log with useful context
- rethrow
- convert to a domain-specific error
- recover intentionally
- report to the user
- mark the feature as unavailable with a visible reason

### 3.3 Do not suppress diagnostics

Do not discard error values, return codes, logs, compiler warnings, or failed test output.

If an error is intentionally ignored, explain why.

Example:

```cpp
// Intentionally ignored: failure only means the optional cache did not exist.
std::error_code ec;
std::filesystem::remove(cachePath, ec);
```

---

## 4. Dependency and Library Rules

### 4.1 Do not reinvent the wheel

Prefer mature open-source, self-hostable libraries when they reduce risk, complexity, or maintenance cost.

Before adding a dependency, evaluate:

- license
- maintenance activity
- platform support
- build integration cost
- binary size impact
- security implications
- long-term stability
- whether the dependency can be self-hosted or vendored if needed

### 4.2 Ask before major dependency decisions

Ask the user before adopting dependencies that affect:

- licensing
- distribution
- runtime requirements
- project architecture
- commercial use
- binary size
- build system complexity

For minor internal utilities, propose a reasonable option and explain the tradeoff.

### 4.3 Isolate third-party APIs

Do not spread third-party API calls across the entire codebase.

Wrap external systems behind adapters or service modules.

Example:

```text
OpenVR SDK
  -> OpenVrRuntime
  -> DevicePoseProvider interface
  -> Application logic
```

This makes later replacement easier.

---

## 5. UI and UX Rules

### 5.1 Design UI for the end-user, not the schema

The interface should reflect what the user is trying to do, not how internal data structures happen to be organized.

Avoid exposing raw internal concepts unless the target user genuinely needs them.

Bad UI structure:

```text
DeviceTable
RawPoseArray
SerializedNodeConfig
ExportFlags
```

Better UI structure:

```text
Connected Trackers
Recording Status
Calibration
Export Settings
Diagnostics
```

### 5.2 Make errors visible and actionable

When something fails, show:

- what failed
- why it likely failed
- what the user can do next
- where logs or diagnostics are located

Do not show vague errors like:

```text
Error occurred.
```

Prefer:

```text
SteamVR runtime was not detected. Start SteamVR, then press Retry.
```

### 5.3 Separate UI from logic

UI code should not directly own core behavior.

Preferred separation:

```text
UI panel
  -> view model / controller
  -> application service
  -> domain logic
  -> platform adapter
```

---

## 6. Continuity Ledger

Maintain a single continuity file for this workspace:

```text
CONTINUITY.md
```

`CONTINUITY.md` is the canonical project briefing. It is designed to survive context compaction and long-running AI sessions.

Do not rely on earlier chat messages, tool output, or memory unless the relevant facts are reflected in `CONTINUITY.md`.

---

## 7. Required Start-of-Turn Behavior

At the start of each assistant turn:

1. Read `CONTINUITY.md` if workspace file access is available.
2. Use it as the authoritative source for:
   - current goal
   - constraints
   - decisions
   - current state
   - open questions
   - working files
   - recent tool outcomes
3. If `CONTINUITY.md` is missing, create it before making substantial changes.
4. If file access is unavailable, state that the ledger cannot be read and continue using only the provided context.

Do not pretend to have read the ledger if it was not available.

---

## 8. When to Update `CONTINUITY.md`

Update `CONTINUITY.md` only when there is a meaningful delta in one or more of the following:

- goal
- success criteria
- invariants or constraints
- decisions
- completed work
- current work
- next step
- open questions
- working set
- important tool outcomes
- recurring incidents
- major errors or mitigations

Do not update the ledger for trivial micro-steps.

---

## 9. Ledger Size Limits

Keep `CONTINUITY.md` short and high-signal.

Recommended caps:

```text
Snapshot:       <= 25 lines
Done recent:    <= 7 bullets
Working set:    <= 12 paths
Receipts:        last 10-20 entries
```

If a section exceeds its cap:

- compress older items into milestone bullets
- point to commits, PRs, log paths, or document paths
- do not paste raw logs
- do not preserve full transcripts

---

## 10. Ledger Anti-Drift Rules

`CONTINUITY.md` must contain facts, not transcripts.

Every factual entry must include:

- date or ISO timestamp
- provenance tag

Allowed provenance tags:

```text
[USER]
[CODE]
[TOOL]
[ASSUMPTION]
```

If something is unknown, write:

```text
UNCONFIRMED
```

Never guess.

If something changes, supersede it explicitly instead of silently rewriting history.

Example:

```text
- 2026-05-27 [USER] D002 SUPERSEDED: Export target was FBX ASCII subset.
- 2026-05-28 [USER] D003 ACTIVE: Export target is now glTF/GLB.
```

---

## 11. Decisions

Record durable choices in `CONTINUITY.md` as ADR-lite entries.

Format:

```text
D001 ACTIVE 2026-05-27 [USER] Use Dear ImGui for the native UI.
- Rationale: lightweight, permissive license, suitable for tool-style UI.
- Consequences: custom styling and accessibility require extra care.
- Supersedes: none.
```

Decision statuses:

```text
ACTIVE
SUPERSEDED
REJECTED
PROPOSED
```

Record decisions for:

- architecture
- major dependencies
- file formats
- licensing-sensitive choices
- platform targets
- build system changes
- public APIs
- long-term workflow rules

---

## 12. Incidents

For recurring weirdness, create a small stable incident capsule in `CONTINUITY.md`.

Format:

```text
I001 ACTIVE 2026-05-27 [TOOL] SteamVR tracker dropouts during recording.
- Symptoms: trackers disappear or freeze during long sessions.
- Evidence: logs/steamvr_dropout_2026-05-27.txt
- Mitigation: increase dongle spacing and expose connection diagnostics.
- Status: under investigation.
```

Use incidents for:

- recurring build failures
- flaky tests
- device connection problems
- export corruption
- rendering glitches
- toolchain instability
- unexplained runtime behavior

---

## 13. Plan Tool vs Ledger

Use a plan tool, when available, for short-term execution scaffolding.

Use `CONTINUITY.md` for long-running continuity.

The plan tool is for:

- immediate task steps
- 3-7 item execution plans
- short-term progress tracking

The ledger is for:

- durable facts
- decisions
- current project state
- continuity across sessions
- constraints and invariants

If no plan tool is available, provide a short in-reply execution plan when useful.

Keep the plan and ledger consistent at the intent/progress level.

---

## 14. Reply Format

When working on this repository, start replies with a brief ledger snapshot:

```text
Ledger Snapshot
- Goal: ...
- Now: ...
- Next: ...
- Open Questions: ...
```

Only print the full ledger when:

- it materially changed
- the user asks for it
- a new ledger was created
- a decision or incident needs explicit review

Keep normal replies concise and focused on the current task.

---

## 15. Recommended `CONTINUITY.md` Template

Use this template when creating a new ledger.

```md
# CONTINUITY.md

## Snapshot
- 2026-05-27 [USER] Goal: UNCONFIRMED
- 2026-05-27 [USER] Success criteria: UNCONFIRMED
- 2026-05-27 [ASSUMPTION] Current phase: planning
- 2026-05-27 [CODE] Current architecture: UNCONFIRMED
- 2026-05-27 [TOOL] Last verified state: UNCONFIRMED

## Invariants / Constraints
- 2026-05-27 [USER] Code files must stay under 300 LOC unless explicitly exempted.
- 2026-05-27 [USER] No silent fallbacks during development.
- 2026-05-27 [USER] No empty try-catch blocks.
- 2026-05-27 [USER] Prefer open-source, self-hostable libraries when appropriate.
- 2026-05-27 [USER] UI must be designed for end users, not schemas.

## Decisions
- D001 ACTIVE 2026-05-27 [USER] Maintain this file as the canonical continuity ledger.
  - Rationale: preserve project context across compaction and long-running work.
  - Supersedes: none.

## State

### Done (recent)
- 2026-05-27 [USER] Initial operating rules defined.

### Now
- 2026-05-27 [ASSUMPTION] Establishing project structure and workflow rules.

### Next
- 2026-05-27 [ASSUMPTION] Confirm project goal, target platform, and initial working set.

## Open Questions
- 2026-05-27 [ASSUMPTION] Does the 300 LOC limit include comments and blank lines?
- 2026-05-27 [ASSUMPTION] Are generated files exempt?
- 2026-05-27 [ASSUMPTION] Which build/test commands are canonical?

## Working Set
- 2026-05-27 [ASSUMPTION] CONTINUITY.md

## Incidents
- None.

## Receipts
- 2026-05-27 [USER] Operating rules supplied in chat.
```

---

## 16. Code Review Checklist

Before completing any code task, verify:

- no manually written code file exceeds 300 lines
- entrypoints remain thin
- responsibilities are separated
- no empty catch blocks exist
- no silent fallback hides a real failure
- errors include useful context
- third-party APIs are isolated
- UI reflects user tasks, not internal schemas
- important decisions are recorded
- `CONTINUITY.md` is updated only if meaningful state changed
- tests or verification steps were run when possible

---

## 17. Build and Test Discipline

When modifying code:

1. Run the smallest relevant verification first.
2. Run broader tests before finalizing if available.
3. Record important test/build outcomes in `CONTINUITY.md` receipts.
4. If tests cannot be run, state why.
5. Do not claim verification that was not performed.

Example receipt:

```text
- 2026-05-27T12:44Z [TOOL] Ran `cmake --build build`; succeeded.
- 2026-05-27T12:48Z [TOOL] Ran `ctest --test-dir build`; 2 tests failed, see logs/test_2026-05-27.txt.
```

---

## 18. Documentation Discipline

When adding or changing behavior, update documentation if the behavior affects:

- setup
- build process
- user workflow
- command-line usage
- configuration
- file formats
- architecture
- troubleshooting
- public API

Do not let documentation drift away from code.

---

## 19. Security and Safety

Do not commit or expose:

- API keys
- tokens
- passwords
- private certificates
- personal data
- machine-specific secrets
- local absolute paths unless necessary for diagnostics

If a secret appears in code or logs:

1. stop using it
2. notify the user
3. remove it from tracked files
4. recommend rotation if exposure is plausible

---

## 20. Final Response Requirements

When finishing a task, include:

- what changed
- where it changed
- how it was verified
- any known limitations
- whether `CONTINUITY.md` was updated

Do not over-report trivial details.

If the task is incomplete, clearly state:

- what was completed
- what remains
- what blocked progress
- the best next action
