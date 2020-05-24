# Home Security System  - Main unit

## Pin Assignment:


| ESP32  | Device       |
| ------ | ------------ |
| GPIO18 | Buzzer       |
| GPIO19 | Alarm LED R  |
| GPIO5  | Alert LED Y  |
| GPIO12 | Keypad col 0 |
| GPIO13 | Keypad col 1 |
| GPIO14 | Keypad col 2 |
| GPIO27 | Keypad row 0 |
| GPIO26 | Keypad row 1 |
| GPIO25 | Keypad row 2 |
| GPIO33 | Keypad row 3 |

![schema](doska_main.png)


## API 

This designs several APIs to fetch resources as follows:

| API                   | Method | Resource Example | Description |
| --------------------- | ------ | ---------------- | ------------|
| `/ble/scan`           | `GET`  | resp: { [<br>{<br>address: "11:22:33:44:55:66" <br>name: "device"<br>}<br>] } | Scan nearby BLE devices |
| `/ble/device/add`     | `POST` | {<br> address: "11:22:33:44:55:66" <br>} | Add selected device to recognized devices |
| `/ble/device/remove`  | `POST` | {<br> address: "11:22:33:44:55:66" <br>} | Remove selected device from recognized devices |
| `/ble/device/list`    | `GET`  | resp: {<br>list: [<br>{address: "11:22:33:44:55:66"}<br>]<br>distance: 4.5<br>} | Return all recognized devices |
| `/ble/device/rssi`    | `POST` | {<br> distance: 4.5 <br>} | Change max detection distance |
| `/code/change`        | `POST` | {<br> code: "123456" <br>new_code: "1234" <br>} | Change alarm deactivation code |
| `/system/arm`         | `GET`  | {} | Activate system |
| `/status`             | `GET`  | resp: {<br>status: "Armed"<br>alarm: 1589742868 <br>sensors: 1<br>notResponding: 0<br>} | System status |
| `/sensors/list`       | `GET`  | resp: {<br>list : [ {<br>address: "11:22:33:44:55:66" <br>type: 0<br>last_alarm: 1589742868<br>last_connection: 1589742868<br>} ]<br>unknown: {<br>address: "11:22:33:44:55:77"<br>type: 1<br>}<br>} | Return all sensors and one new sensor if connected |
| `/sensors/remove`     | `POST` | {<br> address: "11:22:33:44:55:66" <br>} | Remove sensor |
| `/sensors/add`        | `POST` | {<br> address: "11:22:33:44:55:66" <br>} | Add new sensor |
