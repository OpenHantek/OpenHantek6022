# Content
This directory contains all Enums, Structs, Bit- and Byte values
that together composes the Hantek protocol (in different model variants).

# Namespace
Everything in here needs to be in the `Hantek` namespace.

# Dependency
Files in this directory may depend on structs in the `usb` folder, because
the Hantek protocol is only defined for USB devices and some
data structures may depend on USB specific handling.
