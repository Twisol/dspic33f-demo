#ifndef EVENTTYPES_H
#define	EVENTTYPES_H

typedef enum event_class_t {
  EVT_DEBUG=1,
  EVT_BUTTON,
  EVT_UART,
  EVT_RAWCOMM,

  EVT_TIMER1,
} event_class_t;

#endif	/* EVENTTYPES_H */

