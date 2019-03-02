---
layout: default
---
## Software triggered devices like the 6022BE/BL

   - Support theoretically 48, 30, 24, 16, 8, 4, 2, 1 MS/s and 500, 200, 100 kS/s samplerates.
   - Due to the USB bandwith constraints the max samplerate is limited:
   - Max 30 MS/s for CH1 only and max 16 MS/s for CH1+CH2 and also CH2 only.
   - For the 6022BE/BL with modded firmware by [jhoenicke](https://github.com/rpcope1/Hantek6022API).
   - Can detect rising or falling edge of the signal.
   - Note that the first few samples are dropped due to unstable/unusual reading.
