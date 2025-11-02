#pragma once
#include <Arduino.h>


struct ButtonEvent
{
  enum EventType {
    Up,       // button state unchanged:  wasUp   | isUp
    Release,  // button state changed:    wasDown | isUp
    Press,    // button state changed:    wasUp   | isDown
    Down,     // button state unchanged:  wasDown | isDown
  };

  EventType type;
  long millis, prevMillis;
};


class Button
{
public:
  explicit Button(uint8_t pin)
    : pin_(pin)
  {
  }

  void begin()
  {
    wasDown_ = digitalRead(pin_) == LOW;
    lastStateChangeTime_ = millis();
  }

  ButtonEvent getEvent()
  {
    const long now = millis();
    const bool isDown = digitalRead(pin_) == LOW;

    ButtonEvent event;
    event.type = isDown
      ? (wasDown_ ? ButtonEvent::Down : ButtonEvent::Press)
      : (wasDown_ ? ButtonEvent::Release : ButtonEvent::Up);
    event.millis = now - lastStateChangeTime_;
    event.prevMillis = lastEventTime_ - lastStateChangeTime_;

    if (wasDown_ != isDown) {
      wasDown_ = isDown;
      lastStateChangeTime_ = now;
    }

    lastEventTime_ = now;
    return event;
  }

private:
  long lastStateChangeTime_ = 0;
  long lastEventTime_ = 0;
  bool wasDown_ = false;
  uint8_t pin_;
};
