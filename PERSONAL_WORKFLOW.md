# Personal Workflow

## Purpose

This fork is maintained as a personal project.
The workflow is meant to keep experimentation, study and stabilization moving without pretending this is a multi-maintainer process.

## Default work loop

For each issue:

1. Reproduce the problem
2. Identify the owning subsystem
3. Reduce to the smallest plausible cause
4. Patch minimally
5. Rebuild the smallest useful target
6. Validate visible behavior
7. Record what changed and what remains

Do not carry multiple speculative fixes in one step unless the first fix proved insufficient and the chain is documented.

## Priority rule

Always pick the highest user-visible regression from the highest-priority subsystem in [ROADMAP_0.5.0.md](ROADMAP_0.5.0.md).

In practice, prefer:

- shell and session breakage
- repaint, focus and input bugs
- boot, logon, setup and shutdown failures
- storage and networking regressions

before:

- broad refactors
- new features
- speculative compatibility work

## Patch rule

A patch is preferred when it is:

- local
- understandable
- reversible
- measurable in visible behavior

Avoid wide changes unless:

- the bug cannot be fixed locally
- the subsystem already shows repeated instances of the same faulty pattern
- the wider change removes a class of regressions, not just one symptom

## Validation rule

Every meaningful change should leave behind:

- the scenario tested
- the target built
- the result observed
- the next most likely subsystem if the fix was insufficient

Use [STABILITY_VALIDATION.md](STABILITY_VALIDATION.md) as the baseline for deciding whether a fix is good enough.

## Suggested session structure

For a typical work session:

1. Pick one bug only
2. Investigate and patch it
3. Rebuild the affected target or image
4. Test the visible result
5. Decide whether the issue is fixed, reduced or merely displaced
6. Update notes or roadmap only if priorities actually changed

## Tooling rule

Tools, scripts and assistants should reduce friction, not create fake process.
Use them to shorten the loop, not to invent coordination overhead that this fork does not need.

## Decision discipline

If a task does not clearly improve:

- NT 5.x-visible compatibility
- stability
- diagnosability
- maintenance cost in a critical subsystem
- the current personal direction of the fork

it should usually wait.
