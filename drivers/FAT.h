#ifndef FAT_H
#define FAT_H

#include <stdint.h>
#include "../EventBus.h"
#include "SD.h"

typedef struct partition_entry_t {
  uint32_t boot_sector;
  uint32_t sector_count;
} partition_entry_t;

typedef struct fat_filename_t {
  uint8_t name[8];
  uint8_t extension[3];
} fat_filename_t;

typedef struct fat_meta_t {
  fat_filename_t filename;
  uint8_t attributes;
  uint32_t totalBytes;
  uint32_t metaSector;
  uint16_t metaOffset;

  // File pointer
  uint32_t currentSector;
  uint32_t bytesLeft;
} fat_meta_t;


typedef enum fat_result_t {
  FR_OK = 0,
  FR_INPROGRESS,
  FR_COMPAT,
  FR_NOTFOUND,
  FR_EOF,
  FR_FILETYPE,
  FR_READONLY,
} fat_result_t;

typedef enum fat_flow_type_t {
  FF_IDLE = 0,
  FF_LOAD,
  FF_LOOKUP,
  FF_CREATE,
  FF_NEXTSECTOR,
  FF_EXTEND,
  FF_TRUNCATE,
} fat_flow_type_t;

typedef struct FatState {
  uint8_t sectors_per_cluster;

  uint32_t fatStartSector;
  uint32_t fatSectorCount;
  uint32_t dataStartSector;
  fat_meta_t rootDirectory;

  mailbox_t responder;
  fat_result_t lastResult;

  // Temporary data for use during action flows
  fat_flow_type_t flow_type;
  uint8_t flow_state;
  union {
    struct {} idle;
    struct {
      uint32_t boot_sector;
    } load;
    struct {
      uint32_t firstEmptyEntry;
      uint32_t currentSector;

      uint32_t bytesLeft;
      fat_meta_t* result;
    } lookup;
    struct {
      uint16_t fatIndex;
      fat_meta_t* result;
    } next_sector;
    struct {
      bool fatPageWritten;
      uint16_t lastAllocatedCluster;
      uint32_t remainingExtension;
      fat_meta_t* result;

      uint32_t currentSector;
      uint32_t bytesLeft;
    } extend;
    struct {
    } truncate;
  } flow;
} FatState;

void FAT_Init(FatState* self);
bool FAT_Load(FatState* self, partition_entry_t partition, mailbox_t responder);
bool FAT_Create(FatState* self, fat_meta_t* meta, fat_meta_t directory, mailbox_t responder);
bool FAT_Lookup(FatState* self, fat_meta_t* meta, fat_meta_t directory, mailbox_t responder);
bool FAT_NextSector(FatState* self, fat_meta_t* meta, mailbox_t responder);
bool FAT_Extend(FatState* self, fat_meta_t* meta, uint32_t bytesExtension, mailbox_t responder);
bool FAT_Truncate(FatState* self, fat_meta_t* meta, uint32_t byteLength, mailbox_t responder);

inline bool FAT_IsBusy(FatState* self) {
  return (self->flow_type != FF_IDLE);
}

inline fat_meta_t FAT_GetRootDirectory(FatState* self) {
  return self->rootDirectory;
}

inline fat_result_t FAT_GetLastResult(FatState* self) {
  return self->lastResult;
}

void FAT_OnSectorRead(FatState* self, sector_t* sector);
void FAT_OnSectorWrite(FatState* self);

// Imported API
void RawComm_ReadSector(FatState* fat, uint32_t id);
void RawComm_WriteSector(FatState* self, uint32_t id, const sector_t* sector);

#endif /* FAT_H */
