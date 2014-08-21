#ifndef EVENTBUS_H
#define	EVENTBUS_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t event_t;

typedef struct EventBus EventBus;
struct EventBus {
  EventBus* master;
  event_t masterEvent;
  uint16_t tableSize;

  volatile bool signalTable[32];
};

typedef bool (*EventHandler)(void* context, event_t ev);
void EventBus_Init(EventBus* self, EventBus* master, event_t masterEvent);
void EventBus_Signal(EventBus* self, event_t ev);
void EventBus_Tick(EventBus* self, EventHandler handler, void* context);


typedef struct mailbox_t {
  EventBus* target;
  event_t event;
} mailbox_t;

inline
mailbox_t mailbox(EventBus* target, event_t event) {
  mailbox_t mailbox = {target, event};
  return mailbox;
}

inline
void Mailbox_Deliver(mailbox_t mailbox) {
  EventBus_Signal(mailbox.target, mailbox.event);
}

#endif	/* EVENTBUS_H */
