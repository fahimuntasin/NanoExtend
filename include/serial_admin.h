#pragma once

// USB-UART JSON admin channel for PC setup without SoftAP/phone.
// Host requests:  NE>{"v":1,"id":1,"cmd":"ping"}\n
// Device replies: NE{"v":1,"id":1,"ok":true,"result":{...}}\n

class SerialAdmin {
public:
  static void begin();
  static void loop();
};
