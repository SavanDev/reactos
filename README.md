<p align=center>
  <a href="https://reactos.org/">
    <img alt="ReactOS" src="https://reactos.org/wiki/images/0/02/ReactOS_logo.png">
  </a>
</p>

---

<p align=center>
  <img alt="Fork focus" src="https://img.shields.io/badge/fork-personal%200.5.x-4B5563.svg">
  <a href="https://github.com/reactos/reactos/blob/master/COPYING">
    <img alt="License" src="https://img.shields.io/badge/license-GNU_GPL_2.0-0688CB.svg"></a>
</p>

## Official ReactOS Links

These links point to the upstream ReactOS project, not to this fork.

[Website](https://reactos.org/) &bull;
[Official chat](https://chat.reactos.org/) &bull;
[Wiki](https://reactos.org/wiki/) &bull;
[Forum](https://reactos.org/forum/) &bull;
[Community Discord](https://discord.gg/7knjvhT) &bull;
[JIRA Bug Tracker](https://jira.reactos.org/issues/) &bull;
[ReactOS Git mirror](https://git.reactos.org/) &bull;
[Testman](https://reactos.org/testman/)

## About This Fork

This repository is a fork of the original [ReactOS](https://github.com/reactos/reactos) project, not the official upstream repository.

This fork is primarily a personal workspace for experimenting, doing some vibe coding, studying the ReactOS codebase, and retouching the system to better match the maintainer's own preferences.

The current `0.5.x` line in this repository is a personal product direction: a practical baseline for how this fork should behave while it is being stabilized and reshaped. The priority is visible behavior, regression reduction, and a system that feels better to use while remaining grounded in NT 5.x expectations.

This documentation intentionally reflects that reality. It does not frame the fork as a community-managed effort with formal governance or shared ownership.

## What is ReactOS?

ReactOS™ is an Open Source effort to develop a quality operating system that is compatible with applications and drivers written for the Microsoft® Windows™ NT family of operating systems.

Current development is focused on delivering a stable NT 5.x-compatible system, with Windows 2000, XP and Server 2003 behavior treated as the primary external contract for user-mode software and core system components.

ReactOS accepts pragmatic implementation choices when they improve stability, diagnosability or maintainability, as long as they preserve the expected NT 5.x-visible behavior.

The code of ReactOS is licensed under [GNU GPL 2.0](https://github.com/reactos/reactos/blob/master/COPYING).

### Product quality warning

**ReactOS is currently an Alpha quality operating system.** This means that ReactOS is under heavy development and you have to be ready to encounter some problems. Different things may not work well and it can corrupt the data present on your hard disk. It is HIGHLY recommended to test ReactOS on a virtual machine or on a computer with no sensitive or critical data!

## Building

This section keeps the general ReactOS build references.
Local scripts, directories or personal testing habits in this fork may differ from upstream conventions.

To build the system it is strongly advised to use the _ReactOS Build Environment (RosBE)._
Up-to-date versions for Windows and for Unix/GNU-Linux are available from our download page at: ["Build Environment"](https://reactos.org/wiki/Build_Environment).

Alternatively one can use Microsoft Visual C++ (MSVC) version 2019+. Building with MSVC is covered here: ["Visual Studio or Microsoft Visual C++"](https://reactos.org/wiki/CMake#Visual_Studio_or_Microsoft_Visual_C.2B.2B).

See ["Building ReactOS"](https://reactos.org/wiki/Building_ReactOS) article for more details.

### Binaries

To build ReactOS you must run the `configure` script in the directory you want to have your build files. Choose `configure.cmd` or `configure.sh` depending on your system. Then run `ninja <modulename>` to build a module you want or just `ninja` to build all modules.

### Bootable images

To build a bootable CD image run `ninja bootcd` from the build directory. This will create a CD image with a filename `bootcd.iso`.

## Installing

By default, ReactOS currently can only be installed on a machine that has a FAT16 or FAT32 partition as the active (bootable) partition.
The partition on which ReactOS is to be installed (which may or may not be the bootable partition) must also be formatted as FAT16 or FAT32.
ReactOS Setup can format the partitions if needed.

Starting with 0.4.10, ReactOS can be installed using the BtrFS file system. But consider this as an experimental feature and thus regressions not triggered on FAT setup may be observed.

To install ReactOS from the bootable CD distribution, extract the archive contents. Then burn the CD image, boot from it, and follow the instructions.

See ["Installing ReactOS"](https://reactos.org/wiki/Installing_ReactOS) Wiki page or [INSTALL](INSTALL) for more details.

## Testing

ReactOS development prioritizes visible system stability over feature expansion. Regressions in boot, logon, explorer, desktop, start menu, tray, input, networking, storage or shutdown behavior take priority over new features.

See [Technical Direction](TECHNICAL_DIRECTION.md), [Stability Validation](STABILITY_VALIDATION.md), [0.5.x Personal Roadmap](ROADMAP_0.5.0.md) and [Personal Workflow](PERSONAL_WORKFLOW.md) for the current engineering policy, personal release direction and maintenance workflow for this fork.

If you are checking whether an issue already exists in upstream ReactOS, search JIRA first. If not, report the bug providing logs and as much information as possible.

See ["File Bugs"](https://reactos.org/wiki/File_Bugs) for a guide.

__NOTE:__ The upstream bug tracker is _not_ for discussions. Use the upstream [official chat](https://chat.reactos.org/) or [forum](https://reactos.org/forum/) for that.

## Contributing

This fork is not run like an open team project. The default assumption is that it is maintained for personal experimentation and day-to-day work, not for an ongoing public contribution process.

If you want to understand the direction of this fork or prepare a patch anyway, read [CONTRIBUTING.md](CONTRIBUTING.md) first. Small, focused changes that improve NT 5.x-visible behavior, stability and diagnosability fit this fork better than broad rewrites or architecture work done for its own sake.

__Legal notice__: If you have seen proprietary Microsoft Windows source code (including but not limited to the leaked Windows NT 3.5, NT 4, 2000 source code and the Windows Research Kernel), your contribution won't be accepted because of potential copyright violation.

## More information

This section summarizes ReactOS itself as a project, not the specific workflow of this fork.

ReactOS is a Free and Open Source operating system based on the Windows architecture,
providing support for existing applications and drivers, and an alternative to the current dominant consumer operating system.

It is not another wrapper built on Linux, like WINE. It does not attempt or plan to compete with WINE; in fact, the user-mode part of ReactOS is almost entirely WINE-based and our two teams have cooperated closely in the past.

ReactOS is also not pursuing a moving compatibility target across later Windows releases. The current product direction is to harden the NT 5.x line first, and to judge internal design choices by the quality of the observable result rather than by purity of implementation.

ReactOS is also not "yet another OS". It does not attempt to be a third player like any other alternative OS out there. People are not meant to uninstall Linux and use ReactOS instead; ReactOS is a replacement for Windows users who want a Windows replacement that behaves just like Windows.

More information is available at: [reactos.org](https://reactos.org/).

Also see the [media/doc](/media/doc/) subdirectory for some sparse notes.

## Who is responsible

This fork is maintained as a personal project by its owner.

For upstream ReactOS development, see the [GitHub organization](https://github.com/orgs/reactos/people) and the [CREDITS](CREDITS) file.

## Upstream code mirrors

The main development is done on [GitHub](https://github.com/reactos/reactos). We have an [alternative mirror](https://git.reactos.org/?p=reactos.git) in case GitHub is down.

There is also an obsolete [SVN archive repository](https://svn.reactos.org/reactos/) that is kept for historical purposes.

