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
- Wait for explicit user approval before running, booting, or testing an ISO or executable.
- Respect this order: build first, then runtime validation.
- Never try to validate execution against stale assumptions; confirm whether the latest requested build already exists.
- When the user approves a build for testing, build `bootcd`.
- Do not stop at a module-only build when the goal is runtime validation, because the user tests by updating a VM from the generated ISO output.

## Collaboration Defaults

- Prefer the smallest local code change, but for validation builds generate `bootcd`.
- Module-only builds are optional for quick compile checks, but the deliverable for testing is still `bootcd`.
- Report build failures clearly and do not retry long builds automatically without user approval.

## Local Project Context

- This fork is focused on `0.5.0-dev`.
- Prioritize stability and visible behavior over architectural purity.
- Preserve unrelated dirty files and user work.
- Known unrelated local changes may exist under `sdk/cmake/*`; do not revert them unless explicitly requested.
- Technical direction is documented in `TECHNICAL_DIRECTION.md`.
- Release and stabilization scope is documented in `ROADMAP_0.5.0.md`.
- Validation expectations are documented in `STABILITY_VALIDATION.md`.

## Solo Dev + AI Context

- This fork is being developed by one human maintainer plus Codex.
- Work in a way that supports a solo maintainer: minimal context switching, minimal re-explanation, clear next steps, and low-friction testing.
- Follow the workflow documented in `SOLO_AI_WORKFLOW.md`.
- Prefer narrow, understandable patches over broad speculative rewrites.
- Keep momentum on visible stability issues first: shell, session, setup, repaint, input, explorer behavior.
- When a risky change could break the current testing loop, pause and surface that risk before proceeding.

## Current Workflow Preference

- The user may work in parallel in other Codex threads.
- Before any compile or execution step, pause and state what would be run.
- Proceed only after the user confirms.
