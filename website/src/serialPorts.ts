/** Common ESP32 USB-UART bridge chips shown in Chrome's port picker. */
export const ESP_SERIAL_FILTERS: SerialPortFilter[] = [
  { usbVendorId: 0x1a86 }, // QinHeng CH340 / CH341
  { usbVendorId: 0x10c4 }, // Silicon Labs CP210x
  { usbVendorId: 0x0403 }, // FTDI
  { usbVendorId: 0x303a }, // Espressif native USB
  { usbVendorId: 0x067b }, // Prolific
];

export const SERIAL_PORT_HINT =
  "Choose the USB Serial / CH340 / CP210x device. Ignore ttyS0–ttyS15 — those are PC motherboard ports, not the ESP32.";

export async function requestEspSerialPort(): Promise<SerialPort> {
  try {
    return await navigator.serial.requestPort({ filters: ESP_SERIAL_FILTERS });
  } catch (error) {
    const name = error instanceof DOMException ? error.name : "";
    // Empty filtered list / no USB serial visible — fall back so a stuck cable
    // still has a path, but the UI already warns against ttyS*.
    if (name === "NotFoundError") {
      return navigator.serial.requestPort();
    }
    throw error;
  }
}
