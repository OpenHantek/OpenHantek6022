# Content
This directory contains post processing algorithms, namely

* SpectrumGenerator: calculates signal frequency by auto correlation, applies window and calculates DFT spectrum,
* GraphGenerator: Applies all user settings (gain, offset, trigger point) and produces vertices,

# Dependency
* Files in this directory depend on structs in the `hantekprotocol` folder.
* Classes in here probably depend on the user settings (../viewsetting.h, ../scopesetting.h)
