# CaptureBeam Control
C++ module for camera discovery and control using various protocols.

## Target Camera Brands
- Canon
- Nikon
- Sony
- Fujifilm (stretch)

## Prospective Protocols
- PTP/IP (current focus)
- PTP/USB
- Bluetooth
- HTTP APIs
- Shutter release

## Current features
- Wireless control of Canon camera shutter and exposure settings over PTP/IP
- Automatic PTP/IP vendor detection and API selection
- Decoupled camera control API using events to facilitate control over only a binary stream
- Rich packet API to allow flexible bidirectional packet definition and parsing to/from binary
- Minimal implementations of several protocols:
  - Buffered TCP sockets
  - PTP
  - PTP/IP
  - HTTP
  - XML
  - UDP multicast
- Easy portability to new platforms
  - Currently supports Windows and the ESP32 microcontroller (build with PlatformIO)
  - Will later add Linux/macOS and possibly Android

## Upcoming features
- WiFi camera discovery using UPnP/SSDP
- Camera manager to automatically add/remove camera instances from discovery events
  - When complete, will allow full control/feedback of camera control and discovery over a single binary stream such as TCP or WebSockets
- PTP over USB (and possibly Bluetooth control)
- Support for Sony and Nikon cameras
- Interactive applications for desktop, mobile, and embedded