# ReactOS 0.5.x Personal Roadmap

## Objective

The `0.5.x` line in this fork is a personal stabilization track, not a public promise of upstream release management.

It is the current baseline used to shape the system toward the maintainer's preferred behavior while studying and retouching ReactOS.

The target user experience is:

- boot to desktop reliably
- log on and log off reliably
- use explorer, desktop, start menu and tray without common visual or input regressions
- install and run a small fixed set of NT 5.x target applications
- shut down and reboot cleanly

## Direction rules

This line should be judged by visible system quality, not by architectural ambition.

Changes are in scope when they:

- improve observable NT 5.x behavior
- remove common crashes, hangs, flicker, focus bugs or repaint bugs
- improve setup, session startup, storage, networking or shutdown reliability
- improve diagnosability of stability-critical failures

Changes are out of scope when they:

- chase Vista-or-later compatibility
- add new public APIs outside the NT 5.x contract
- restructure internals mainly for purity without visible stability gain
- expand low-value feature surface while shell or session regressions remain

## Priority order

1. Shell and interactive session
2. Boot, setup, logon and shutdown
3. Storage and networking reliability
4. Services and recovery paths
5. Kernel simplifications that reduce deadlocks, races or corruption
6. New features

## First stabilization backlog

### P0: Must stabilize first

- `explorer`, `shellmenu`, `comctl32`, `win32k`
- start menu, tray, desktop, taskbar and window interaction
- repaint, focus, input and flicker regressions
- setup and first boot usability
- shutdown and reboot correctness

### Current completed stabilization work

- start menu hover and submenu flicker reduced in `shellmenu`, `toolbar` and button repaint paths
- tray and show-desktop redraw paths narrowed to avoid unnecessary background erase
- desktop marquee selection stabilized in `listview` for transparent, double-buffered desktop mode
- desktop icon select/deselect and drag ghosting fixed by explicit dirty-region invalidation
- desktop view now honors `FWF_NOSCROLL` via `LVS_NOSCROLL`
- welcome/setup UI paint path narrowed and double-buffered to reduce visible flicker

### P1: Session and system reliability

- `smss`, `csrss`, winlogon/session transitions
- service startup ordering and recovery
- storage mount reliability and common file operations
- basic networking bring-up and common failure paths

### P2: Structural hardening

- add higher value logs and assertions in stability-critical paths
- reduce non-diagnosable failure states
- simplify internal mechanisms that repeatedly cause races or deadlocks
- preserve working driver scenarios, but do not optimize architecture around maximum historical driver compatibility

### P3: Deferred

- new subsystems
- broad compatibility expansion beyond NT 5.x
- internal refactors with no near-term stability payoff
- optional Unix-like tooling that does not help current stabilization work

## Exit criteria for this line

This line is in a good state only when all of the following are true on the reference build:

- no common crash/hang path remains in boot, logon, explorer or shutdown
- start menu, tray and desktop are usable without obvious visual corruption in normal use
- file copy/move and basic networking work in smoke tests
- the reference application suite completes its agreed smoke scenarios
- release validation in [STABILITY_VALIDATION.md](STABILITY_VALIDATION.md) passes without a blocking regression

## Initial target application suite

This list should stay small and fixed for the current `0.5.x` cycle in this fork.

- 7-Zip
- Notepad++
- Firefox 52 ESR or another frozen browser baseline chosen once
- WinRAR or another shell-integration-heavy utility chosen once
- one legacy NT 5.x administrative or diagnostic utility

If the project later chooses different applications, the list should be updated in one commit and then frozen again.
