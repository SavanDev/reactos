# Stability Validation

## Purpose

This document defines the minimum stability checks expected for this fork's personal validation loop while it is focused on hardening NT 5.x compatibility.

The goal is to catch regressions in visible behavior before prioritizing new functionality.

## Release candidate baseline

Each release candidate should be validated against the following baseline:

- clean boot to desktop
- successful logon and logoff
- explorer stability during normal session use
- start menu, desktop and tray interaction
- keyboard and mouse input correctness
- file copy and move in common shell workflows
- basic networking setup and connectivity
- clean shutdown and reboot

If a regression appears in any of these areas, it should block unrelated feature work until triaged.

## Extended stability checks

The following scenarios should be run for candidate builds and for risky subsystem changes:

- repeated open/close cycles of explorer windows and shell menus
- long idle session in VM
- installation and uninstallation of representative software
- service start, stop and recovery scenarios
- storage mount and file operation smoke tests
- leak monitoring for GDI handles, USER handles and obvious resource growth
- repeated reboot cycles when boot, setup or service infrastructure changes

Where applicable, keep serial logs, debugger output or equivalent diagnostic evidence for failures.

## Target application suite

Each release cycle should define and freeze a small NT 5.x reference application suite.
The suite should be committed to the repository or release notes as a concrete list before release validation starts.

The suite should include at least:

- one installer-heavy desktop application
- one shell-integration-heavy application
- one networked application
- one administrative or system utility
- one legacy application known to exercise NT 5.x behavior

Changes that improve synthetic tests while regressing the reference suite should not be treated as wins.

## Change-level expectations

Changes that touch user-visible or session-critical code should describe:

- which visible NT 5.x behavior is affected
- what manual or automated validation was performed
- whether boot, shell, setup, services, storage or networking paths were exercised

Subsystems that require extra care include:

- `explorer`, `shellmenu`, `comctl32`, `win32k`
- setup, logon, session startup and shutdown paths
- service control paths
- storage and networking foundations

## Regression policy

The following should be treated as high-priority regressions:

- flicker or broken repaint in common shell flows
- focus or input loss
- hangs or crashes in explorer, start menu, tray or desktop
- boot or logon failures
- broken shutdown or reboot
- storage corruption, mount failures or basic networking regressions

Internal refactoring is only acceptable when it preserves observable behavior and improves reproducible stability, diagnosability or maintenance cost.
