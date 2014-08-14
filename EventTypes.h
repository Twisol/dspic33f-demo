#ifndef EVENTTYPES_H
#define	EVENTTYPES_H

typedef enum event_class_t {
  EVT_DEBUG=1,
  EVT_BUTTON,
  EVT_UART,
  EVT_RAWCOMM,
  EVT_SD,
  EVT_SD_OVERFLOW,

  EVT_TIMER1,
  EVT_SD_INIT,
} event_class_t;

#endif	/* EVENTTYPES_H */

