#ifndef EVENTTYPES_H
#define	EVENTTYPES_H

typedef enum event_class_t {
  EVT_IGNORE=0,
  EVT_DEBUG,
  EVTBUS_SD,

  EVT_BUTTON,
  EVT_UART,
  EVT_SD,
  EVT_SD_READY,
  EVT_SD_OVERFLOW,

  EVT_TIMER1,
} event_class_t;

#endif	/* EVENTTYPES_H */

