## Valid specs for the 6022BE/BL with [custom firmware by Ho-Ro](https://github.com/Ho-Ro/Hantek6022API).

   - The device has no internal storage (except for a small fifo buffer), all data is streamed in real time as USB bulk transfer.
   - Supports 48, 30, 24, 16, 15, 12, 10, 8, 6, 5, 4, 3, 2, 1 MS/s and 500, 400, 200, 100, 64, 50, 40, 20 kS/s samplerates.
   - Due to the USB bandwidth constraints the max usable samplerate is limited: Max 30 MS/s for CH1 only and max 15 MS/s for CH1+CH2 and also CH2 only - otherwise data overrun occurs.
   - Under Linux (even better if using a realtime kernel) the priority of the program can be raised when the user is member of a *realtime* group, e.g. audio:


    1. set limits in /etc/security/limits.d:
       @audio - rtprio 99
    2. add user to the group, e.g. audio:
       usermod -a -G audio <your_user_name>
    or set the limits only for your user in /etc/security/limits.d:
       <your_user_name> - rtprio 99

   - The scope program works with blocks of 20000 samples - either 8 bit samples for CH1 only or 16 bit samples otherwise.
   - Two analog input channels with 8-bit sample-width, max input voltage range is -5 V..+5 V, values outside this range are clipped (shown as minimum or maximum value).
   - For low effective sampling rates < 1 MS/s a 10X..200X oversampling is used to increase the signal-to-noise ratio by 10..20 dB and to get a better voltage resolution (up to > 11 effective bits).
   The oversampling allows also low effective samplerates down to 100 S/s for a slow display range of 10s/div.
   - The first samples are unstable (due to the use of the ADC far outside the common mode voltage specifications) -> take about 2000 additional values to settle the ADC and drop the first samples.
   - No AC coupling available, all signals are measured as DC. (Use either a BNC DC-block or the math mode "CHx AC").
   - A [little HW modification](HANTEK6022_AC_Modification.pdf) adds AC coupling. OpenHantek6022 supports this feature since v2.17-rc5 / FW0204.
   - No HW trigger available, SW trigger searches for trigger condition and positions the trace window accordingly.
   - Can detect rising or falling edge (also alternating from trace to trace) of the signal, displayed trigger position can be at 0..100% of screen width.
