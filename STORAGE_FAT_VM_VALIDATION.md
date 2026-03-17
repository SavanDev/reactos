# FAT VM Validation

This validation is intended for guest testing in VirtualBox after updating the installed system with a new build from `output-MinGW-i386\bootcd.iso`.

The guest should expose two non-system test volumes:

- one FAT16 volume
- one FAT32 volume

Do not run these tests against the system volume.

## Guest-side tools

Copy these scripts into the guest, for example through a shared folder:

- `modules\rostests\win32\fs\manual\fat_smoke.cmd`
- `modules\rostests\win32\fs\manual\fat_dirty_loop.cmd`

## Matrix

Run the same sequence for the FAT16 and FAT32 test volumes.
Examples below use `D:` and `E:` only as placeholders.

### 1. Smoke test

Run:

```bat
fat_smoke.cmd D:
fat_smoke.cmd E:
```

Pass criteria:

- script exits with code `0`
- log contains `RESULT: PASS`
- file create/copy/rename/move/delete operations succeed

Logs are written to `%TEMP%` as `fatvm_smoke_<drive>_<id>.log`.

### 2. Clean reboot

Run the smoke test, reboot normally, then run it again.

Pass criteria:

- desktop comes back cleanly
- tested volume mounts without visible errors
- smoke test still passes after reboot

### 3. Dirty volume recovery

Start a write loop on the target volume:

```bat
fat_dirty_loop.cmd D:
```

Let it run for at least 30 seconds, then force power off the VM.
Boot the VM again and observe the tested volume.

Pass criteria:

- guest boots again
- the tested volume mounts
- `autochk` may run if the volume stayed dirty
- after recovery, `fat_smoke.cmd` passes again on the tested volume

Logs are written to `%TEMP%` as `fatvm_dirty_<drive>_<id>.log`.

### 4. Basic shell coverage

After the smoke test passes, do a short manual shell pass on the same volume:

- create a folder from Explorer
- copy a file into it
- rename the file
- delete the file
- empty Recycle Bin if applicable

Pass criteria:

- no hang or crash in Explorer
- file state matches the shell action

## What to report back

For each tested filesystem, send:

- whether smoke passed before reboot
- whether smoke passed after clean reboot
- whether the dirty-loop recovery booted and mounted correctly
- whether `autochk` ran
- whether smoke passed after the dirty test
- any visible corruption, stale directory contents, rename failures or shutdown anomalies
