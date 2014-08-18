#ifndef EVENTTYPES_H
#define	EVENTTYPES_H

typedef enum event_class_t {
  EVT_IGNORE=0,
  EVT_DEBUG,

  EVT_BUTTON,
  EVT_UART,
  EVT_TIMER1,

  EVT_SD_READY,
  EVT_SD_SECTOR,
} event_class_t;

#endif	/* EVENTTYPES_H */

