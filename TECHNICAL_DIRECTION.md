# Technical Direction

This document describes the personal technical direction of this fork.
It is not a statement of governance or roadmap for upstream ReactOS.

## Product target

This fork is currently focused on shaping a more stable NT 5.x-compatible system around the maintainer's preferred `0.5.x` baseline.
The practical compatibility ceiling remains the Windows 2000, XP and Server 2003 line.

This means the project should optimize first for:

- predictable NT 5.x-visible behavior
- stable boot, logon, shell and shutdown flows
- reliable execution of target applications
- maintainable and diagnosable system internals

This fork is not currently targeting Vista-or-later compatibility as a primary engineering goal.

## External contract

The following contract is considered non-negotiable unless explicitly revised:

- Win32 and NTDLL behavior expected by NT 5.x software
- kernel-user ABI exposed to user mode and system components
- classic shell, explorer and start menu behavior expected by users of NT 5.x systems
- setup, services, session startup, storage and networking semantics required by common NT 5.x usage

Internal implementation does not need to replicate historical NT structure if the observable result stays compatible with this contract.

## Design rule

When evaluating technical changes, prefer the option that:

1. preserves or improves observable NT 5.x behavior
2. improves stability, recovery, diagnosability or maintainability
3. minimizes architectural complexity that does not buy visible compatibility

Reject changes whose main value is internal purity if they:

- increase fragility
- make debugging harder
- delay stabilization of core user flows
- chase later Windows behavior at the expense of NT 5.x quality

## Unix-inspired implementation policy

Unix-inspired tools or mechanisms are acceptable when they remain behind NT-facing interfaces and improve engineering outcomes.

Acceptable areas include:

- build, packaging and developer tooling
- diagnostics, logging and system inspection tools
- internal mechanisms in kernel or services that are not exposed as a new public contract
- optional utilities that do not replace the default Windows-compatible workflow

Not acceptable without an explicit fork-level decision:

- changing default Win32, shell, registry, service manager or path semantics to resemble Unix
- introducing new public APIs that move the system outside the NT 5.x contract
- replacing stable Windows-visible behavior with Unix-style behavior in the default product

## Engineering priorities

Priority order for stabilization work:

1. shell and interactive session reliability
2. setup, boot, logon and shutdown
3. storage, file systems and networking
4. service control and recovery
5. internal simplification that reduces races, deadlocks or corruption
6. new features

Subsystems that should be treated as high-risk and user-visible:

- `explorer`, `shellmenu`, `comctl32`, `win32k`
- `services`, `smss`, `csrss`, logon paths and setup
- `ntoskrnl` and base drivers involved in stability-critical paths

Compatibility with third-party Windows drivers is a secondary goal. Existing working scenarios should be preserved when reasonable, but core architecture should not be optimized around maximum historical driver coverage.

When a tradeoff is unclear, prefer the option that best supports this fork's practical day-to-day use, testing rhythm and visible stability.
