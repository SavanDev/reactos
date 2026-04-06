# ReactOS Fork Instructions For Codex

These instructions apply to the whole repository rooted at `F:\SavanDev\reactos`.

## Build Environment

- Use `C:\RosBE\RosBE.cmd` before any ReactOS build command.
- Use build directory `F:\SavanDev\reactos\output-MinGW-i386`
- Final validation artifact is always `bootcd`

Canonical command pattern:

```bat
cmd.exe /c "call C:\RosBE\RosBE.cmd && ninja -C F:\SavanDev\reactos\output-MinGW-i386 <target>"
```

Examples:

```bat
cmd.exe /c "call C:\RosBE\RosBE.cmd && ninja -C F:\SavanDev\reactos\output-MinGW-i386 bootcd"
```

## Build And Run Policy

- Do not start a build just because code changed.
- Do not start a build while the user may be compiling in another thread or terminal.
- Wait for explicit user approval before compiling.
- Once the user approves a compile command/target, you may retry that same build after fixing compile errors without asking again until it compiles successfully.
- If the compile command, target, or validation goal changes, ask for approval again before compiling.
- Wait for explicit user approval before running, booting, or testing an ISO or executable.
- Respect this order: build first, then runtime validation.
- Never try to validate execution against stale assumptions; confirm whether the latest requested build already exists.
- When the user approves a build for testing, build `bootcd`.
- Do not stop at a module-only build when the goal is runtime validation, because the user tests by updating a VM from the generated ISO output.

## Working Defaults

- Prefer the smallest local code change, but for validation builds generate `bootcd`.
- Module-only builds are optional for quick compile checks, but the deliverable for testing is still `bootcd`.
- Report build failures clearly and do not retry long builds automatically without user approval.

## Local Project Context

- This fork is focused on the current `0.5.x` line as a personal working baseline.
- The fork exists to experiment, study ReactOS internals, and reshape parts of the system to the maintainer's taste.
- Prioritize stability and visible behavior over architectural purity.
- Preserve unrelated dirty files and user work.
- Known unrelated local changes may exist under `sdk/cmake/*`; do not revert them unless explicitly requested.
- Technical direction is documented in `TECHNICAL_DIRECTION.md`.
- Release and stabilization scope is documented in `ROADMAP_0.5.0.md`.
- Validation expectations are documented in `STABILITY_VALIDATION.md`.
- Day-to-day working habits are documented in `PERSONAL_WORKFLOW.md`.

## Fork Reality

- Treat this as a personal fork, not as a community process or a shared governance model.
- Optimize for minimal context switching, clear next steps, and low-friction testing.
- Prefer narrow, understandable patches over broad speculative rewrites.
- Keep momentum on visible stability issues first: shell, session, setup, repaint, input, explorer behavior.
- When a risky change could break the current testing loop, pause and surface that risk before proceeding.

## Current Workflow Preference

- The user may work in parallel in other Codex threads.
- Before any compile or execution step, pause and state what would be run.
- Proceed only after the user confirms.
- During a compile-fix loop for the same approved build command, do not re-ask for confirmation on each retry; only pause again once the build succeeds or if the intended command changes.
