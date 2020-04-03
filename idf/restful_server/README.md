# Home Security System  - Main unit

## API 

This designs several APIs to fetch resources as follows:

| API                        | Method | Resource Example                                      | Description                                                                              |
| -------------------------- | ------ | ------------------------------------------------------------------- | ----------------------------------------------------------------------------------------|
| `/ble/scan`           | `GET`  | {} | Scan nearby BLE devices |
| `/ble/device/add`     | `POST` | {<br> } | Add selected device to recognized devices |
| `/ble/device/remove`  | `POST` | {<br> address: "11:22:33:44:55:66" <br>} | Remove selected device from recognized devices |
| `/ble/device/list`    | `GET`  | {} | Return all recognized devices |
| `/code/change`        | `POST` | {<br> code: "123456" <br>new_code: "1234" <br>} | Change alarm deactivation code |
| `/system/arm`         | `GET`  | {} | Activate System |

#### Pin Assignment:


| ESP32  | Device  |
| ------ | ------- |
| GPIO18 | Buzzer       |
| GPIO19 | Alarm LED    |
| GPIO12 | Keypad col 0 |
| GPIO13 | Keypad col 1 |
| GPIO14 | Keypad col 2 |
| GPIO27 | Keypad row 0 |
| GPIO26 | Keypad row 1 |
| GPIO25 | Keypad row 2 |
| GPIO33 | Keypad row 3 |

