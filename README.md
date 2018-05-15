# IMULogger
Reading mpu-9250 IMU data with timestamp, and sending data to Serial.

features:
- support GMT+8 time zone.
- integrate time and IMU data(Liner Acc/Gyro/Mag/Pose).

Data is started by a string "START" and ended by a "END".


How to install:
- copy folder **RTIMULib** and **Time** to your arduino library.
- download the IMULogger sketch to your Arduino uno.

when it starts, you should calibrate the time by inputting "T+calendarTime" through serial.
you can got it by command: `date +T%s` on Linux.


Using libraries from [RTIMULib](https://github.com/richardstechnotes/RTIMULib-Arduino) and [Time](https://github.com/PaulStoffregen/Time).
