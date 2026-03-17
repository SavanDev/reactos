# FASTFAT Hardening Backlog

This fork treats FAT16/FAT32 as a product-critical path for PC targets.
`fastfat` is the only supported FAT implementation in the active product path.

## Immediate priorities

1. Mount dirty and clean FAT16/FAT32 volumes reliably.
2. Run `autochk` only when the dirty bit or recovery state requires it.
3. Propagate failures in rename, delete, truncate and growth paths without silent metadata drift.
4. Guarantee shutdown and reboot only clear dirty state after all buffered metadata is flushed.
5. Validate recovery after unclean shutdown on both FAT16 and FAT32.

## Validation matrix

### FAT16

- Format volume.
- Install and boot when applicable.
- Create, open, read and write files.
- Rename, move and delete files and directories.
- Reboot after clean shutdown.
- Reboot with a dirty volume and verify `autochk`.

### FAT32

- All FAT16 scenarios.
- Repeated truncate and grow operations.
- Large files.
- Large directories.
- Recovery after unclean shutdown.
- Verify dirty bit is cleared only after a successful flush path.

## Test guidance

- Prefer system tests and boot-to-desktop scenarios over isolated unit tests.
- Keep checks focused on externally visible NT 5.x behavior.
- Treat silent corruption or dirty-bit regressions as release blockers for this area.
- Use `STORAGE_FAT_VM_VALIDATION.md` for the current guest validation flow.
