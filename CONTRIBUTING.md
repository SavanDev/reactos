# Contributing To This Fork

This fork is not organized as an always-open collective project.
It is primarily a personal workspace for experimenting, studying ReactOS, and shaping the `0.5.x` line to the fork owner's preferences.

That means:

- there is no promise of active PR triage, review cadence or merge timing
- broad feature proposals are usually less useful than small, concrete fixes
- changes that do not fit the current personal direction may be ignored even if they are technically valid
- if your real goal is to contribute to ReactOS as a public project, upstream is the better place to do it

## If You Still Want To Send Something

The most likely changes to fit this fork are:

- fixes for visible regressions in boot, logon, shell, explorer, desktop, input, storage, networking or shutdown
- narrow cleanups that reduce fragility in stability-critical paths
- documentation updates that match the actual direction of the fork

The least likely changes to fit this fork are:

- wide architectural rewrites
- compatibility work aimed mainly at Vista or later
- feature expansion that does not improve the current `0.5.x` experience
- process or governance documents written for a multi-maintainer project

## Direction

When deciding whether a change belongs here, use these rules:

- preserve or improve observable NT 5.x behavior first
- prefer visible stability over architectural purity
- keep changes small, local and easy to reason about
- preserve working scenarios when reasonable, but do not optimize the fork around maximum historical driver coverage

See [TECHNICAL_DIRECTION.md](TECHNICAL_DIRECTION.md), [ROADMAP_0.5.0.md](ROADMAP_0.5.0.md) and [STABILITY_VALIDATION.md](STABILITY_VALIDATION.md) for the current direction.

## Clean Room Requirement

If you have seen proprietary Microsoft Windows source code, your contribution cannot be accepted because of potential copyright violation.
Before contributing, you must be able to affirm the following:

> I have not used or seen the source code to any version of the Windows operating system,
> nor any Microsoft product source code that would make this contribution legally unsafe,
> including but not limited to the leaked Windows 2000 source code and the Windows Research Kernel.

## Practical Notes

- Use the commit template in [.gitmessage](.gitmessage) if you want to match the existing style.
- Patches related to third-party code such as Wine or BtrFS should usually go upstream to those projects first. See [media/doc](media/doc), [README.WINE](/media/doc/README.WINE) and [README.FSD](/media/doc/README.FSD).
- If you are fixing something user-visible, describe the scenario affected and the validation you performed.

## Upstream ReactOS

If you want the normal public contribution flow for ReactOS itself, use the official upstream project and its community channels instead of treating this fork as the main intake path.
