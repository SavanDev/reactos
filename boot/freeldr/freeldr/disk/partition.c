/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Block Device partition management
 * COPYRIGHT:   Copyright 2002-2003 Brian Palmer <brianp@sginet.com>
 *              Copyright 2019 Stanislav Motylkov <x86corez@gmail.com>
 *              Copyright 2025-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#ifndef _M_ARM
#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(DISK);

#define MaxDriveNumber 0xFF
static PARTITION_STYLE DiskPartitionType[MaxDriveNumber + 1];

<<<<<<< HEAD
#include "part_mbr.h" // FIXME: For MASTER_BOOT_RECORD

BOOLEAN
=======
static BOOLEAN
>>>>>>> 213a20b506d (storage: remove FATX/xbox paths and harden fastfat cleanup)
DiskReadBootRecord(
    IN UCHAR DriveNumber,
    IN ULONGLONG LogicalSectorNumber,
    OUT PMASTER_BOOT_RECORD BootRecord)
{
    ULONG Index;

    /* Read master boot record */
    if (!MachDiskReadLogicalSectors(DriveNumber, LogicalSectorNumber, 1, DiskReadBuffer))
    {
        return FALSE;
    }
    RtlCopyMemory(BootRecord, DiskReadBuffer, sizeof(MASTER_BOOT_RECORD));

    TRACE("Dumping partition table for drive 0x%x:\n", DriveNumber);
    TRACE("Boot record logical start sector = %d\n", LogicalSectorNumber);
    TRACE("sizeof(MASTER_BOOT_RECORD) = 0x%x.\n", sizeof(MASTER_BOOT_RECORD));

    for (Index = 0; Index < 4; Index++)
    {
        TRACE("-------------------------------------------\n");
        TRACE("Partition %d\n", (Index + 1));
        TRACE("BootIndicator: 0x%x\n", BootRecord->PartitionTable[Index].BootIndicator);
        TRACE("StartHead: 0x%x\n", BootRecord->PartitionTable[Index].StartHead);
        TRACE("StartSector (Plus 2 cylinder bits): 0x%x\n", BootRecord->PartitionTable[Index].StartSector);
        TRACE("StartCylinder: 0x%x\n", BootRecord->PartitionTable[Index].StartCylinder);
        TRACE("SystemIndicator: 0x%x\n", BootRecord->PartitionTable[Index].SystemIndicator);
        TRACE("EndHead: 0x%x\n", BootRecord->PartitionTable[Index].EndHead);
        TRACE("EndSector (Plus 2 cylinder bits): 0x%x\n", BootRecord->PartitionTable[Index].EndSector);
        TRACE("EndCylinder: 0x%x\n", BootRecord->PartitionTable[Index].EndCylinder);
        TRACE("SectorCountBeforePartition: 0x%x\n", BootRecord->PartitionTable[Index].SectorCountBeforePartition);
        TRACE("PartitionSectorCount: 0x%x\n", BootRecord->PartitionTable[Index].PartitionSectorCount);
    }

    /* Check the partition table magic value */
    return (BootRecord->MasterBootRecordMagic == 0xaa55);
}

<<<<<<< HEAD
#include "part_mbr.c"
#include "part_gpt.c"
#include "part_brfr.c"
=======
static BOOLEAN
DiskGetFirstPartitionEntry(
    IN PMASTER_BOOT_RECORD MasterBootRecord,
    OUT PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
    ULONG Index;

    for (Index = 0; Index < 4; Index++)
    {
        /* Check the system indicator. If it's not an extended or unused partition then we're done. */
        if ((MasterBootRecord->PartitionTable[Index].SystemIndicator != PARTITION_ENTRY_UNUSED) &&
            (MasterBootRecord->PartitionTable[Index].SystemIndicator != PARTITION_EXTENDED) &&
            (MasterBootRecord->PartitionTable[Index].SystemIndicator != PARTITION_XINT13_EXTENDED))
        {
            RtlCopyMemory(PartitionTableEntry, &MasterBootRecord->PartitionTable[Index], sizeof(PARTITION_TABLE_ENTRY));
            return TRUE;
        }
    }

    return FALSE;
}

static BOOLEAN
DiskGetFirstExtendedPartitionEntry(
    IN PMASTER_BOOT_RECORD MasterBootRecord,
    OUT PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
    ULONG Index;

    for (Index = 0; Index < 4; Index++)
    {
        /* Check the system indicator. If it an extended partition then we're done. */
        if ((MasterBootRecord->PartitionTable[Index].SystemIndicator == PARTITION_EXTENDED) ||
            (MasterBootRecord->PartitionTable[Index].SystemIndicator == PARTITION_XINT13_EXTENDED))
        {
            RtlCopyMemory(PartitionTableEntry, &MasterBootRecord->PartitionTable[Index], sizeof(PARTITION_TABLE_ENTRY));
            return TRUE;
        }
    }

    return FALSE;
}

static BOOLEAN
DiskGetActivePartitionEntry(
    IN UCHAR DriveNumber,
    OUT PPARTITION_TABLE_ENTRY PartitionTableEntry,
    OUT PULONG ActivePartition)
{
    ULONG BootablePartitionCount = 0;
    ULONG CurrentPartitionNumber;
    ULONG Index;
    MASTER_BOOT_RECORD MasterBootRecord;
    PPARTITION_TABLE_ENTRY ThisPartitionTableEntry;

    *ActivePartition = 0;

    /* Read master boot record */
    if (!DiskReadBootRecord(DriveNumber, 0, &MasterBootRecord))
    {
        return FALSE;
    }

    CurrentPartitionNumber = 0;
    for (Index = 0; Index < 4; Index++)
    {
        ThisPartitionTableEntry = &MasterBootRecord.PartitionTable[Index];

        if (ThisPartitionTableEntry->SystemIndicator != PARTITION_ENTRY_UNUSED &&
            ThisPartitionTableEntry->SystemIndicator != PARTITION_EXTENDED &&
            ThisPartitionTableEntry->SystemIndicator != PARTITION_XINT13_EXTENDED)
        {
            CurrentPartitionNumber++;

            /* Test if this is the bootable partition */
            if (ThisPartitionTableEntry->BootIndicator == 0x80)
            {
                BootablePartitionCount++;
                *ActivePartition = CurrentPartitionNumber;

                /* Copy the partition table entry */
                RtlCopyMemory(PartitionTableEntry,
                              ThisPartitionTableEntry,
                              sizeof(PARTITION_TABLE_ENTRY));
            }
        }
    }

    /* Make sure there was only one bootable partition */
    if (BootablePartitionCount == 0)
    {
        ERR("No bootable (active) partitions found.\n");
        return FALSE;
    }
    else if (BootablePartitionCount != 1)
    {
        ERR("Too many bootable (active) partitions found.\n");
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN
DiskGetMbrPartitionEntry(
    IN UCHAR DriveNumber,
    IN ULONG PartitionNumber,
    OUT PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
    MASTER_BOOT_RECORD MasterBootRecord;
    PARTITION_TABLE_ENTRY ExtendedPartitionTableEntry;
    ULONG ExtendedPartitionNumber;
    ULONG ExtendedPartitionOffset;
    ULONG Index;
    ULONG CurrentPartitionNumber;
    PPARTITION_TABLE_ENTRY ThisPartitionTableEntry;

    /* Read master boot record */
    if (!DiskReadBootRecord(DriveNumber, 0, &MasterBootRecord))
    {
        return FALSE;
    }

    CurrentPartitionNumber = 0;
    for (Index = 0; Index < 4; Index++)
    {
        ThisPartitionTableEntry = &MasterBootRecord.PartitionTable[Index];

        if (ThisPartitionTableEntry->SystemIndicator != PARTITION_ENTRY_UNUSED &&
            ThisPartitionTableEntry->SystemIndicator != PARTITION_EXTENDED &&
            ThisPartitionTableEntry->SystemIndicator != PARTITION_XINT13_EXTENDED)
        {
            CurrentPartitionNumber++;
        }

        if (PartitionNumber == CurrentPartitionNumber)
        {
            RtlCopyMemory(PartitionTableEntry, ThisPartitionTableEntry, sizeof(PARTITION_TABLE_ENTRY));
            return TRUE;
        }
    }

    /*
     * They want an extended partition entry so we will need
     * to loop through all the extended partitions on the disk
     * and return the one they want.
     */
    ExtendedPartitionNumber = PartitionNumber - CurrentPartitionNumber - 1;

    /*
     * Set the initial relative starting sector to 0.
     * This is because extended partition starting
     * sectors a numbered relative to their parent.
     */
    ExtendedPartitionOffset = 0;

    for (Index = 0; Index <= ExtendedPartitionNumber; Index++)
    {
        /* Get the extended partition table entry */
        if (!DiskGetFirstExtendedPartitionEntry(&MasterBootRecord, &ExtendedPartitionTableEntry))
        {
            return FALSE;
        }

        /* Adjust the relative starting sector of the partition */
        ExtendedPartitionTableEntry.SectorCountBeforePartition += ExtendedPartitionOffset;
        if (ExtendedPartitionOffset == 0)
        {
            /* Set the start of the parrent extended partition */
            ExtendedPartitionOffset = ExtendedPartitionTableEntry.SectorCountBeforePartition;
        }
        /* Read the partition boot record */
        if (!DiskReadBootRecord(DriveNumber, ExtendedPartitionTableEntry.SectorCountBeforePartition, &MasterBootRecord))
        {
            return FALSE;
        }

        /* Get the first real partition table entry */
        if (!DiskGetFirstPartitionEntry(&MasterBootRecord, PartitionTableEntry))
        {
            return FALSE;
        }

        /* Now correct the start sector of the partition */
        PartitionTableEntry->SectorCountBeforePartition += ExtendedPartitionTableEntry.SectorCountBeforePartition;
    }

    /*
     * When we get here we should have the correct entry already
     * stored in PartitionTableEntry, so just return TRUE.
     */
    return TRUE;
}
>>>>>>> 213a20b506d (storage: remove FATX/xbox paths and harden fastfat cleanup)

VOID
DiskDetectPartitionType(
    _In_ UCHAR DriveNumber)
{
    MASTER_BOOT_RECORD MasterBootRecord;
<<<<<<< HEAD

=======
    ULONG Index;
    ULONG PartitionCount = 0;
    PPARTITION_TABLE_ENTRY ThisPartitionTableEntry;
    BOOLEAN GPTProtect = FALSE;
>>>>>>> 213a20b506d (storage: remove FATX/xbox paths and harden fastfat cleanup)
    /* Probe for Master Boot Record */
    if (DiskReadBootRecord(DriveNumber, 0, &MasterBootRecord))
    {
#if 0
        ULONG Index, PartitionCount = 0;
        BOOLEAN GPTProtect = FALSE;
#else
        GPT_TABLE_HEADER GptHeader;
#endif

        DiskPartitionType[DriveNumber] = PARTITION_STYLE_MBR;

#if 0
        /* Check for GUID Partition Table */
        for (Index = 0; Index < 4; Index++)
        {
            PPARTITION_TABLE_ENTRY PartitionTableEntry = &MasterBootRecord.PartitionTable[Index];

            if (PartitionTableEntry->SystemIndicator != PARTITION_ENTRY_UNUSED)
            {
                PartitionCount++;
                if (Index == 0 && PartitionTableEntry->SystemIndicator == PARTITION_GPT)
                    GPTProtect = TRUE;
            }
        }

        if (PartitionCount == 1 && GPTProtect)
#else
        if (DiskReadGptHeader(DriveNumber, &GptHeader))
#endif
        {
            DiskPartitionType[DriveNumber] = PARTITION_STYLE_GPT;
        }
        TRACE("Drive 0x%X partition type %s\n", DriveNumber, DiskPartitionType[DriveNumber] == PARTITION_STYLE_MBR ? "MBR" : "GPT");
        return;
    }

<<<<<<< HEAD
    /* Probe for Xbox-BRFR partitioning */
    if (DiskIsBrfr(DriveNumber))
    {
        DiskPartitionType[DriveNumber] = PARTITION_STYLE_BRFR;
        TRACE("Drive 0x%X partition type Xbox-BRFR\n", DriveNumber);
        return;
    }

    /* Failed to detect partitions, assume non-partitioned disk */
=======
    /* Failed to detect partitions, assume partitionless disk */
>>>>>>> 213a20b506d (storage: remove FATX/xbox paths and harden fastfat cleanup)
    DiskPartitionType[DriveNumber] = PARTITION_STYLE_RAW;
    TRACE("Drive 0x%X partition type unknown\n", DriveNumber);
}

// FIXME: This function is specific to BIOS-based PC platform.
BOOLEAN
DiskGetBootPartitionEntry(
    _In_ UCHAR DriveNumber,
    _Out_opt_ PPARTITION_INFORMATION PartitionEntry,
    _Out_ PULONG BootPartition)
{
#if 0
    GEOMETRY Geometry;
    if (!MachDiskGetDriveGeometry(DriveNumber, &Geometry))
        return FALSE;
#endif

    switch (DiskPartitionType[DriveNumber])
    {
        case PARTITION_STYLE_MBR:
        {
            return DiskGetActivePartitionEntry(DriveNumber,
            /* MBR partition table always uses 512-byte sectors per specification */
                                               512, // Geometry.BytesPerSector
                                               PartitionEntry, BootPartition);
        }
        case PARTITION_STYLE_GPT:
        {
            FIXME("DiskGetBootPartitionEntry() unimplemented for GPT\n");
            return FALSE;
        }
        case PARTITION_STYLE_RAW:
        {
            FIXME("DiskGetBootPartitionEntry() unimplemented for RAW\n");
            return FALSE;
        }
<<<<<<< HEAD
        case PARTITION_STYLE_BRFR:
        {
            PARTITION_INFORMATION TempPartitionEntry;
            if (!PartitionEntry)
                PartitionEntry = &TempPartitionEntry;
            if (DiskGetBrfrPartitionEntry(DriveNumber,
                                          512, // Geometry.BytesPerSector
                                          FATX_DATA_PARTITION,
                                          PartitionEntry))
            {
                *BootPartition = FATX_DATA_PARTITION;
                return TRUE;
            }
            return FALSE;
        }
=======
>>>>>>> 213a20b506d (storage: remove FATX/xbox paths and harden fastfat cleanup)
        default:
        {
            ERR("Drive 0x%X partition type = %d, should not happen!\n", DriveNumber, DiskPartitionType[DriveNumber]);
            ASSERT(FALSE);
        }
    }
    return FALSE;
}

BOOLEAN
DiskGetPartitionEntry(
    _In_ UCHAR DriveNumber,
    _In_opt_ ULONG SectorSize,
    _In_ ULONG PartitionNumber,
    _Out_ PPARTITION_INFORMATION PartitionEntry)
{
    if (SectorSize == 0)
    {
        GEOMETRY Geometry;
        if (!MachDiskGetDriveGeometry(DriveNumber, &Geometry))
            return FALSE;
        SectorSize = Geometry.BytesPerSector;
    }
    if (SectorSize < 512)
    {
        ERR("Drive 0x%X: Invalid sector size %lu\n", DriveNumber, SectorSize);
        return FALSE;
    }

    switch (DiskPartitionType[DriveNumber])
    {
        case PARTITION_STYLE_MBR:
        {
            return DiskGetMbrPartitionEntry(DriveNumber,
            /* MBR partition table always uses 512-byte sectors per specification */
                                            512, // SectorSize
                                            PartitionNumber,
                                            PartitionEntry);
        }
        case PARTITION_STYLE_GPT:
        {
            return DiskGetGptPartitionEntry(DriveNumber,
                                            SectorSize,
                                            PartitionNumber,
                                            PartitionEntry);
        }
        case PARTITION_STYLE_RAW:
        {
            FIXME("DiskGetPartitionEntry() unimplemented for RAW\n");
            return FALSE;
        }
<<<<<<< HEAD
        case PARTITION_STYLE_BRFR:
        {
            return DiskGetBrfrPartitionEntry(DriveNumber,
                                             512, // SectorSize
                                             PartitionNumber,
                                             PartitionEntry);
        }
=======
>>>>>>> 213a20b506d (storage: remove FATX/xbox paths and harden fastfat cleanup)
        default:
        {
            ERR("Drive 0x%X partition type = %d, should not happen!\n", DriveNumber, DiskPartitionType[DriveNumber]);
            ASSERT(FALSE);
        }
    }
    return FALSE;
}

#endif
