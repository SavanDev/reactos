/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _M_ARM
#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(DISK);

#define MaxDriveNumber 0xFF
static PARTITION_STYLE DiskPartitionType[MaxDriveNumber + 1];

static BOOLEAN
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

VOID
DiskDetectPartitionType(
    IN UCHAR DriveNumber)
{
    MASTER_BOOT_RECORD MasterBootRecord;
    ULONG Index;
    ULONG PartitionCount = 0;
    PPARTITION_TABLE_ENTRY ThisPartitionTableEntry;
    BOOLEAN GPTProtect = FALSE;
    /* Probe for Master Boot Record */
    if (DiskReadBootRecord(DriveNumber, 0, &MasterBootRecord))
    {
        DiskPartitionType[DriveNumber] = PARTITION_STYLE_MBR;

        /* Check for GUID Partition Table */
        for (Index = 0; Index < 4; Index++)
        {
            ThisPartitionTableEntry = &MasterBootRecord.PartitionTable[Index];

            if (ThisPartitionTableEntry->SystemIndicator != PARTITION_ENTRY_UNUSED)
            {
                PartitionCount++;

                if (Index == 0 && ThisPartitionTableEntry->SystemIndicator == PARTITION_GPT)
                {
                    GPTProtect = TRUE;
                }
            }
        }

        if (PartitionCount == 1 && GPTProtect)
        {
            DiskPartitionType[DriveNumber] = PARTITION_STYLE_GPT;
        }
        TRACE("Drive 0x%X partition type %s\n", DriveNumber, DiskPartitionType[DriveNumber] == PARTITION_STYLE_MBR ? "MBR" : "GPT");
        return;
    }

    /* Failed to detect partitions, assume partitionless disk */
    DiskPartitionType[DriveNumber] = PARTITION_STYLE_RAW;
    TRACE("Drive 0x%X partition type unknown\n", DriveNumber);
}

BOOLEAN
DiskGetBootPartitionEntry(
    IN UCHAR DriveNumber,
    OUT PPARTITION_TABLE_ENTRY PartitionTableEntry,
    OUT PULONG BootPartition)
{
    switch (DiskPartitionType[DriveNumber])
    {
        case PARTITION_STYLE_MBR:
        {
            return DiskGetActivePartitionEntry(DriveNumber, PartitionTableEntry, BootPartition);
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
    IN UCHAR DriveNumber,
    IN ULONG PartitionNumber,
    OUT PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
    switch (DiskPartitionType[DriveNumber])
    {
        case PARTITION_STYLE_MBR:
        {
            return DiskGetMbrPartitionEntry(DriveNumber, PartitionNumber, PartitionTableEntry);
        }
        case PARTITION_STYLE_GPT:
        {
#ifdef UEFIBOOT
            BOOLEAN UefiGetGptPartitionEntry(
                IN UCHAR DriveNumber,
                IN ULONG PartitionNumber,
                OUT PPARTITION_TABLE_ENTRY PartitionTableEntry);
            return UefiGetGptPartitionEntry(DriveNumber, PartitionNumber, PartitionTableEntry);
#else
            FIXME("DiskGetPartitionEntry() unimplemented for GPT\n");
            return FALSE;
#endif
        }
        case PARTITION_STYLE_RAW:
        {
            FIXME("DiskGetPartitionEntry() unimplemented for RAW\n");
            return FALSE;
        }
        default:
        {
            ERR("Drive 0x%X partition type = %d, should not happen!\n", DriveNumber, DiskPartitionType[DriveNumber]);
            ASSERT(FALSE);
        }
    }
    return FALSE;
}

#endif
