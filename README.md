# Acoustic-localization-device
A device that can locate a sound source of a particular frequency in 2-D space.

This project is built based on a TM4C123 microcontroller with an ARM cortex-M processor.
The idea was to have a setup of 3 microphones that detect audio signals, at the vetrexs of an equilateral triangle.
This setup is then mounted on a high torque servo motor. All the microphones have individual narrow bandpass filters.
These filters allow signals in a particulat frequency range (5500Hz-6500Hz) and attenuates all the other signals. 
These filtered signals are received by the MCU. The ADC on the MCU is used to convert the received signals into digital values.
The ADC included in the TM4C123 MCU is a 12 bit precision ADC. So the signal values vary between 0-4096.

The obtained signals are then filtered using a software filter which disposes of erratic values in the signal. 
The RMS value and the peak-to-peak values of the resultant signal are obtaine. These values are used to determine which microphone detects the maximum intensity of the sound source. Once the mic is fixed the servo moves the mic towards that direction in 5 degree increments until the signal is at its maximum. 


Initially the idea was to build a device that can locate sound source in 3-D space. But due to time and money constraints we had to settle for 2-D localization. 

For obtaining the 3rd dimention the following setup and logic need to be implemented:
A fourth mic that is mounted on a pan tilt servo is used to determine the position of the audio signal in 3-D space. Once the horizontal direction of the sound source is obtained, a fourth mic that is mounted on a pan tilt servo is used to determine the position of the audio signal in vertical direction. The pan servo sets the mic toward the horizontal direction of the source. The tilt servo moves the mic vertically in 5 degree increments until it detects the audio signal at maximum intensity. 


The various modules used are:
Timers
Interrupts
PWM
ADC and 
uDMA 
