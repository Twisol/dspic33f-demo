#include "PushButtons.h"

static PushButtonHandler buttonHandler = 0;
void PushButton_Init(PushButtonHandler handler) {
  buttonHandler = handler;
}

void PushButton_EventHandler(uint8_t buttonId) {
  if (buttonHandler) {
    buttonHandler();
  }
}