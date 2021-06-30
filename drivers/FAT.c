#include "FAT.h"
#include <string.h>
#include <stdbool.h>

#include "../int_utils.h"


void FAT_Init(FatState* self) {
  self->lastResult = FR_OK;
  memset(self, 0, sizeof(FatState));
}

bool FAT_Load(FatState* self, partition_entry_t partition, mailbox_t responder) {
  if (self->flow_state != FF_IDLE) {
    return false;
  }

  self->flow_type = FF_LOAD;
  self->flow_state = 0;

  self->flow.load.boot_sector = partition.boot_sector;

  self->responder = responder;
  self->lastResult = FR_INPROGRESS;

  RawComm_ReadSector(self, partition.boot_sector);

  return true;
}

bool FAT_Lookup(FatState* self, fat_meta_t* meta, fat_meta_t directory, mailbox_t responder) {
  if (self->flow_state != FF_IDLE) {
    return false;
  }

  if (!(directory.attributes & 0x10)) {
  // Not a subdirectory
    self->lastResult = FR_FILETYPE;
    Mailbox_Deliver(responder);
  } else {
    self->flow_type = FF_LOOKUP;
    self->flow_state = 0;

    self->flow.lookup.firstEmptyEntry = 0xFFFFFFFF;
    self->flow.lookup.currentSector = directory.currentSector;
    self->flow.lookup.bytesLeft = directory.bytesLeft;
    self->flow.lookup.result = meta;

    self->responder = responder;
    self->lastResult = FR_INPROGRESS;

    RawComm_ReadSector(self, self->flow.lookup.currentSector);
  }

  return true;
}

bool FAT_Create(FatState* self, fat_meta_t* meta, fat_meta_t directory, mailbox_t responder) {
  if (self->flow_state != FF_IDLE) {
    return false;
  }

  if (!(directory.attributes & 0x10)) {
  // Not a subdirectory
    self->lastResult = FR_FILETYPE;
    Mailbox_Deliver(responder);
  } else if (directory.attributes & 0x01) {
  // Read-only
    self->lastResult = FR_READONLY;
    Mailbox_Deliver(responder);
  } else {
    self->flow_type = FF_CREATE;
    self->flow_state = 0;

    self->flow.lookup.firstEmptyEntry = 0xFFFFFFFF;
    self->flow.lookup.currentSector = directory.currentSector;
    self->flow.lookup.bytesLeft = directory.bytesLeft;
    self->flow.lookup.result = meta;

    self->responder = responder;
    self->lastResult = FR_INPROGRESS;

    RawComm_ReadSector(self, self->flow.lookup.currentSector);
  }

  return true;
}

bool FAT_NextSector(FatState* self, fat_meta_t* meta, mailbox_t responder) {
  if (self->flow_state != FF_IDLE) {
    return false;
  }

  // If the next sector is within the same cluster, just return that one.
  uint32_t x = meta->currentSector % self->sectors_per_cluster;
  if (x + 1 < self->sectors_per_cluster) {
    meta->currentSector += 1;
    meta->bytesLeft -= 512;

    self->lastResult = FR_OK;
    Mailbox_Deliver(responder);
    return true;
  }

  //The +2 is b/c the first 2 location on the FAT are reserved.
  uint16_t cluster = (meta->currentSector - self->dataStartSector)/self->sectors_per_cluster + 2;
  uint16_t fatPage = cluster / 256;
  uint16_t fatIndex = 2 * (cluster % 256);

  if (fatPage >= self->fatSectorCount) {
    self->lastResult = FR_EOF;
    Mailbox_Deliver(responder);
  } else {
    self->flow_type = FF_NEXTSECTOR;
    self->flow_state = 0;
    self->flow.next_sector.fatIndex = fatIndex;
    self->flow.next_sector.result = meta;

    self->responder = responder;
    self->lastResult = FR_INPROGRESS;

    RawComm_ReadSector(self, self->fatStartSector + fatPage);
  }

  return true;
}

bool FAT_Extend(FatState* self, fat_meta_t* meta, uint32_t bytesExtension, mailbox_t responder) {
  if (self->flow_state != FF_IDLE) {
    return false;
  }

  if (meta->attributes & 0x01) {
  // Read-only
    self->lastResult = FR_READONLY;
    Mailbox_Deliver(responder);
  } else {
    self->flow_type = FF_EXTEND;
    self->flow.extend.fatPageWritten = false;
    self->flow.extend.lastAllocatedCluster = 0xFFFF;
    self->flow.extend.remainingExtension = bytesExtension;
    self->flow.extend.result = meta;

    self->responder = responder;
    self->lastResult = FR_INPROGRESS;

    // Immediately account for the excess space available in the current cluster.
    uint32_t bytesUsedInCluster = (512*(meta->currentSector - self->dataStartSector) + meta->bytesLeft) % (512*self->sectors_per_cluster);
    uint32_t unusedBytesInCluster = 512*self->sectors_per_cluster - bytesUsedInCluster;
    uint32_t initialExtension = min(unusedBytesInCluster, self->flow.extend.remainingExtension);

    self->flow.extend.result->bytesLeft  += initialExtension;
    self->flow.extend.result->totalBytes += initialExtension;
    self->flow.extend.remainingExtension -= initialExtension;

    uint32_t stepExtension = min(512*self->sectors_per_cluster, self->flow.extend.remainingExtension);
    if (0 < stepExtension && stepExtension <= UINT32_MAX - self->flow.extend.result->totalBytes) {
    // It's safe to allocate a new cluster
      self->flow.extend.currentSector = self->fatStartSector;
      self->flow.extend.bytesLeft = 512*self->fatSectorCount;

      self->flow_state = 0;
      RawComm_ReadSector(self, self->fatStartSector);
    } else if (initialExtension == 0) {
    // We haven't changed anything, so don't bother the storage device.
      self->lastResult = (self->flow.extend.remainingExtension == 0) ? FR_OK : FR_EOF;
      Mailbox_Deliver(responder);
    } else {
    // We need to update the file's entry.
      self->flow_state = 3;
      RawComm_ReadSector(self, meta->metaSector);
    }
  }

  return true;
}

bool FAT_Truncate(FatState* self, fat_meta_t* meta, uint32_t byteLength, mailbox_t responder) {
  if (self->flow_state != FF_IDLE) {
    return false;
  }

  // TODO

  return true;
}

extern inline fat_meta_t FAT_GetRootDirectory(FatState* self);
extern inline fat_result_t FAT_GetLastResult(FatState* self);


static
void FAT_LoadFlow(FatState* self, sector_t* sector) {
  switch (self->flow_state) {
  case 0: {
    uint16_t bytes_per_sector = read_uint16(&sector->data[0x0B]);
    uint16_t reserved_sector_count = read_uint16(&sector->data[0x0E]);
    uint16_t root_entry_count = read_uint16(&sector->data[0x11]);
    uint8_t fat_count = sector->data[0x10];
    uint16_t fat_sector_count = read_uint16(&sector->data[0x16]);
    uint32_t sector_count = read_uint16(&sector->data[0x13]);
    if (sector_count == 0) {
      sector_count = read_uint32(&sector->data[0x20]);
    }

    if (bytes_per_sector != 512) {
      self->flow_type = FF_IDLE;
      self->flow_state = 0;

      self->lastResult = FR_COMPAT;
      Mailbox_Deliver(self->responder);
      break;
    }

    fat_meta_t root;
    memset(&root.filename, 0, sizeof(fat_filename_t));
    root.attributes = 0x17;
    root.currentSector = self->flow.load.boot_sector + reserved_sector_count + fat_sector_count * fat_count;
    root.bytesLeft = 32ul * root_entry_count;
    root.metaSector = 0;
    root.metaOffset = 0;

    self->sectors_per_cluster = sector->data[0x0D];
    self->fatStartSector = self->flow.load.boot_sector + reserved_sector_count;
    self->fatSectorCount = fat_sector_count;
    self->rootDirectory = root;
    self->dataStartSector = root.currentSector + (32ul * root_entry_count) / bytes_per_sector;

    self->flow_type = FF_IDLE;
    self->flow_state = 0;

    self->lastResult = FR_OK;
    Mailbox_Deliver(self->responder);
    break;
  }

  default:
    break;
  }
}

static
void FAT_LookupFlow(FatState* self, sector_t* sector) {
  /*
   * Locate file in directory, or unused entry.
   * If file was found, return entry.
   * Otherwise,
   *   Reserve unused cluster in FAT.
   *   If no unused cluster, fail.
   *   Otherwise,
   *     Update unused entry with cluster information.
   */

  switch (self->flow_state) {
  case 0: { // Locate file in directory
    const uint8_t* const endPtr = &sector->data[min(512, self->flow.lookup.bytesLeft)];

    uint8_t* entryPtr;
    for (entryPtr = sector->data; entryPtr + 32 <= endPtr; entryPtr += 32) {
      if (entryPtr[0x00] == 0x00) {
      // No more entries
        if (self->flow.lookup.firstEmptyEntry == 0xFFFFFFFF) {
          self->flow.lookup.firstEmptyEntry = 512*self->flow.lookup.currentSector + (entryPtr - sector->data);
        }

        break;
      } else if (entryPtr[0x00] == 0x2e) {
      // Deleted entry
        if (self->flow.lookup.firstEmptyEntry == 0xFFFFFFFF) {
          self->flow.lookup.firstEmptyEntry = 512*self->flow.lookup.currentSector + (entryPtr - sector->data);
        }

        continue;
      } else if (!memcmp(&entryPtr[0x00], self->flow.lookup.result->filename.name, 8)
          && !memcmp(&entryPtr[0x08], self->flow.lookup.result->filename.extension, 3)) {
      // Matching entry
        self->flow.lookup.result->attributes = entryPtr[0x0B];
        self->flow.lookup.result->totalBytes = read_uint32(&entryPtr[0x1C]);
        self->flow.lookup.result->metaSector = self->flow.lookup.currentSector;
        self->flow.lookup.result->metaOffset = (entryPtr - sector->data);

        self->flow.lookup.result->currentSector = self->dataStartSector + self->sectors_per_cluster * (read_uint16(&entryPtr[0x1A]) - 2);
        self->flow.lookup.result->bytesLeft = self->flow.lookup.result->bytesLeft;

        goto entryFound;
      }
    } noSuchEntry: {
    // Didn't find a matching entry
      if (self->flow.lookup.bytesLeft >= 512) {
      // Scan the next sector of the directory
        self->flow.lookup.currentSector += 1;
        self->flow.lookup.bytesLeft -= 512;

        RawComm_ReadSector(self, self->flow.lookup.currentSector);
      } else if (self->flow_type != FF_CREATE) {
      // Don't try to create an entry, just bail now.
        self->flow_type = FF_IDLE;
        self->flow_state = 0;

        self->lastResult = FR_NOTFOUND;
        Mailbox_Deliver(self->responder);
      } else if (self->flow.lookup.firstEmptyEntry == 0xFFFFFFFF) {
      // No space for a new file; user must Extend the directory first
        self->flow_type = FF_IDLE;
        self->flow_state = 0;

        self->lastResult = FR_EOF;
        Mailbox_Deliver(self->responder);
      } else {
      // Go reserve a cluster in the FAT for the new file
        self->flow_state = 1;

        self->flow.lookup.currentSector = self->fatStartSector;
        self->flow.lookup.bytesLeft = 512 * self->fatSectorCount;
        RawComm_ReadSector(self, self->flow.lookup.currentSector);
      }

      goto dirDone;
    } entryFound: {
    // Found a matching entry
      self->flow_type = FF_IDLE;
      self->flow_state = 0;

      self->lastResult = FR_OK;
      Mailbox_Deliver(self->responder);

      goto dirDone;
    } dirDone:

    break;
  }

  case 1: { // Reserve a cluster in the FAT
    const uint8_t* const endPtr = &sector->data[min(512, self->flow.lookup.bytesLeft)];

    uint8_t* entryPtr;
    for (entryPtr = sector->data; entryPtr + 2 <= endPtr; entryPtr += 2) {
      uint16_t entry = read_uint16(entryPtr);
      if (entry == 0x0000) {
        write_uint16(&entryPtr[0x1A], 0xFFFF);

        uint16_t cluster = 256*(self->flow.lookup.currentSector - self->fatStartSector) + (entryPtr - sector->data);
        self->flow.lookup.result->currentSector = self->dataStartSector + self->sectors_per_cluster*(cluster - 2);
        self->flow.lookup.result->bytesLeft = 0;

        goto clusterFound;
      }
    } noUnusedClusters: {
    // Didn't find an unused cluster
      self->flow.lookup.currentSector += 1;
      self->flow.lookup.bytesLeft -= (endPtr - sector->data);

      if (self->flow.lookup.bytesLeft > 0) {
      // Scan the next sector of the FAT
        RawComm_ReadSector(self, self->flow.lookup.currentSector);
      } else {
      // No cluster in the FAT is unused.
        self->flow_type = FF_IDLE;
        self->flow_state = 0;

        self->lastResult = FR_NOTFOUND;
        Mailbox_Deliver(self->responder);
      }

      goto fatDone;
    } clusterFound: {
    // Found an unused cluster
      self->flow_state = 2;

      RawComm_WriteSector(self, self->flow.lookup.currentSector, sector);
      goto fatDone;
    } fatDone:

    break;
  }

  case 2: { // Read the directory sector with the unused entry
    self->flow.lookup.currentSector = self->flow.lookup.firstEmptyEntry / 512;
    self->flow_state = 3;

    RawComm_ReadSector(self, self->flow.lookup.currentSector);
    break;
  }

  case 3: { // Update the directory with the new entry
    uint8_t* entryPtr = sector->data + (self->flow.lookup.firstEmptyEntry % 512);

    // TODO: creation and modification dates
    memcpy(&entryPtr[0x00], self->flow.lookup.result->filename.name, 8);
    memcpy(&entryPtr[0x08], self->flow.lookup.result->filename.extension, 3);
    entryPtr[0x0B] = self->flow.lookup.result->attributes;
    write_uint16(&entryPtr[0x1A], (self->flow.lookup.result->currentSector - self->dataStartSector) / self->sectors_per_cluster + 2);
    write_uint32(&entryPtr[0x1C], self->flow.lookup.result->bytesLeft);

    self->flow_state = 4;

    RawComm_WriteSector(self, self->flow.lookup.currentSector, sector);
    break;
  }

  case 4: {
    self->flow_type = FF_IDLE;
    self->flow_state = 0;

    self->lastResult = FR_OK;
    Mailbox_Deliver(self->responder);
    break;
  }

  default:
    break;
  }
}

static inline
void FAT_CreateFlow(FatState* self, sector_t* sector) {
  // When flow_type == FF_CREATE, LookupFlow will attempt to create a new file.
  FAT_LookupFlow(self, sector);
}

static
void FAT_NextSectorFlow(FatState* self, sector_t* sector) {
  switch (self->flow_state) {
  case 0: {
    uint16_t cluster = read_uint16(&sector->data[self->flow.next_sector.fatIndex]);
    if (cluster == 0xFFFF) {
    // No more clusters
      self->flow_type = FF_IDLE;
      self->flow_state = 0;

      self->lastResult = FR_EOF;
      Mailbox_Deliver(self->responder);
    } else {
      self->flow_type = FF_IDLE;
      self->flow_state = 0;

      self->flow.next_sector.result->currentSector = self->dataStartSector + self->sectors_per_cluster*(cluster - 2);
      self->flow.next_sector.result->bytesLeft -= 512;

      self->lastResult = FR_OK;
      Mailbox_Deliver(self->responder);
    }

    break;
  }

  default:
    break;
  }
}

static
void FAT_ExtendFlow(FatState* self, sector_t* sector) {
  reentry:
  switch (self->flow_state) {
  case 0: {
    const uint8_t* const endPtr = &sector->data[min(512, self->flow.extend.bytesLeft)];
    uint8_t* entryPtr;

    for (entryPtr = sector->data; entryPtr + 2 <= endPtr; entryPtr += 2) {
      uint16_t entry = read_uint16(entryPtr);
      if (entry == 0x0000) {
        uint32_t stepExtension = min(512*self->sectors_per_cluster, self->flow.extend.remainingExtension);

        write_uint16(&entryPtr[0x1A], self->flow.extend.lastAllocatedCluster);

        self->flow.extend.fatPageWritten = true;
        self->flow.extend.lastAllocatedCluster = 256*(self->flow.extend.currentSector - self->fatStartSector) + (entryPtr - sector->data);
        self->flow.extend.result->bytesLeft  += stepExtension;
        self->flow.extend.result->totalBytes += stepExtension;
        self->flow.extend.remainingExtension -= stepExtension;

        uint32_t nextExtension = min(512*self->sectors_per_cluster, self->flow.extend.remainingExtension);
        if (0 < nextExtension && nextExtension <= UINT32_MAX - self->flow.extend.result->totalBytes) {
        // We need to allocate more clusters.
          continue;
        } else {
        // We're done here.
          goto extensionComplete;
        }
      }
    } noUnusedClusters: {
    // No unused clusters found.
      self->flow.extend.currentSector += 1;
      self->flow.extend.bytesLeft -= (endPtr - sector->data);

      if (self->flow.extend.fatPageWritten) {
        self->flow.extend.fatPageWritten = false;

        self->flow_state = 1;
        RawComm_WriteSector(self, self->flow.extend.currentSector, sector);
      } else {
        self->flow_state = 1;
        goto reentry;
      }

      goto fatDone;
    } extensionComplete: {
    // We've finished extending our file.
      self->flow.extend.bytesLeft = 0; // Don't keep traversing the FAT.

      self->flow_state = 1;
      RawComm_WriteSector(self, self->flow.extend.currentSector, sector);

      goto fatDone;
    } fatDone:

    break;
  }

  case 1: { // Need the next sector of the FAT
    if (self->flow.extend.bytesLeft > 0) {
    // Scan the next sector of the FAT
      self->flow_state = 0;
      RawComm_ReadSector(self, self->flow.extend.currentSector);
    } else if (self->flow.extend.lastAllocatedCluster == 0xFFFF) {
    // There are no unused clusters left to allocate,
    // and we never found a single one to start with.
      self->flow_state = 4;
      RawComm_ReadSector(self, self->flow.extend.result->metaSector);
    } else {
    // We found at least one new cluster,
    // so we need to chain it to the end of the existing file.
      uint16_t cluster = (self->flow.extend.result->metaSector - self->dataStartSector)/self->sectors_per_cluster + 2;
      self->flow.extend.currentSector = self->fatStartSector + (cluster / 256);

      self->flow_state = 2;
      RawComm_ReadSector(self, self->flow.extend.currentSector);
    }
    break;
  }

  case 2: { // Chain new allocation to tail of existing file
    uint16_t cluster = (self->flow.extend.result->metaSector - self->dataStartSector)/self->sectors_per_cluster + 2;
    uint32_t fatPage = cluster / 256;
    uint32_t fatIndex = 2 * (cluster % 256);

    while (true) {
      if (self->fatStartSector + fatPage != self->flow.extend.currentSector) {
        self->flow.extend.currentSector = self->fatStartSector + fatPage;

        RawComm_ReadSector(self, self->flow.extend.currentSector);
        break;
      }

      cluster = read_uint16(&sector->data[fatIndex]);
      if (0x0002 <= cluster && cluster <= 0xFFEF) {
        fatPage = cluster / 256;
        fatIndex = 2 * (cluster % 256);
      } else if (0xFFF8 <= cluster && cluster <= 0xFFFF) {
        write_uint16(&sector->data[fatIndex], self->flow.extend.lastAllocatedCluster);

        self->flow_state = 3;
        RawComm_WriteSector(self, self->flow.extend.currentSector, sector);
        break;
      }
    }

    break;
  }

  case 3: { // Request file entry
    self->flow_state = 4;
    RawComm_ReadSector(self, self->flow.extend.result->metaSector);
    break;
  }

  case 4: { // Update file entry with new byte length
    uint8_t* entryPtr = &sector->data[self->flow.extend.result->metaOffset];
    write_uint32(&entryPtr[0x1C], self->flow.extend.result->totalBytes);

    self->flow_state = 5;
    RawComm_WriteSector(self, self->flow.extend.result->metaSector, sector);
    break;
  }

  case 5: { // File entry successfully updated.
    self->flow_type = FF_IDLE;
    self->flow_state = 0;

    self->lastResult = (self->flow.extend.remainingExtension == 0) ? FR_OK : FR_EOF;
    Mailbox_Deliver(self->responder);
    break;
  }

  default:
    break;
  }
}

static
void FAT_TruncateFlow(FatState* self, sector_t* sector) {
  switch (self->flow_state) {
  case 0: {
    // TODO

    break;
  }

  default:
    break;
  }
}


void FAT_OnSectorRead(FatState* self, sector_t* sector) {
  switch (self->flow_type) {
  case FF_LOAD:
    FAT_LoadFlow(self, sector);
    break;

  case FF_LOOKUP:
    FAT_LookupFlow(self, sector);
    break;

  case FF_CREATE:
    FAT_CreateFlow(self, sector);
    break;

  case FF_NEXTSECTOR:
    FAT_NextSectorFlow(self, sector);
    break;

  case FF_EXTEND:
    FAT_ExtendFlow(self, sector);
    break;

  case FF_TRUNCATE:
    FAT_TruncateFlow(self, sector);
    break;

  case FF_IDLE:
  default:
    break;
  }
}

void FAT_OnSectorWrite(FatState* self) {
  switch (self->flow_type) {
  case FF_LOAD:
    FAT_LoadFlow(self, NULL);
    break;

  case FF_LOOKUP:
    FAT_LookupFlow(self, NULL);
    break;

  case FF_CREATE:
    FAT_CreateFlow(self, NULL);
    break;

  case FF_NEXTSECTOR:
    FAT_NextSectorFlow(self, NULL);
    break;

  case FF_EXTEND:
    FAT_ExtendFlow(self, NULL);
    break;

  case FF_TRUNCATE:
    FAT_TruncateFlow(self, NULL);
    break;

  case FF_IDLE:
  default:
    break;
  }
}
