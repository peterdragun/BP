# Home Security System  - Sensor unit

## Setup

Set sensor type by running:

```
idf.py menuconfig
```

Choose `Sensor configuration` and set value of `Select type of sensor`. Currently PIR and door sensors are supported. PIR sets wakeup of ESP on high voltage. Door sensors sets ESP wakeup on low voltage.

## Pin Assignment:


| ESP32  | Device      |
| ------ | ----------- |
| GPIO32 | Sensor      |
| GPIO33 | Alarm LED   |

![schema_PIR](doska_PIR.png)
![schema_door](doska_door.png)
