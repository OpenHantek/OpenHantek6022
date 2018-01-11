# Content
This directory contains post processing algorithms, namely

* SoftwareTrigger: Determines a steady point, is used by GraphGenerator,
* GraphGenerator: Applies all user settings (gain, offset, trigger point) and produces vertices,
* MathChannelGenerator: Creates a math channel on top of the pysical channels

# Dependency
* Files in this directory depend on structs in the `hantekprotocol` folder.
* Classes in here probably depend on the user settings (../viewsetting.h, ../scopesetting.h)
