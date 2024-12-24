# CaptureBeam Control
C++ module for camera control using various protocols.

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

## Ideas
- End goal is to have full control and feedback over a pair of simple binary streams (similar to PTP/IP)
- Command stream and event stream
- All feedback is asynchronous over the event stream; no explicit command responses
  - This makes things easier and means that it will also work with multiple devices controlling the same control module
- Each camera object will have an event queue (of event packets), to which events are added as they occur
  - E.g. on connection, disconnection, prop change, etc.
- When an error occurs, necessary actions are executed (therefore triggering respective events) before throwing an exception
  - E.g. socket timeout closes transport, which dispatches camera disconnect event, which eventually updates front end
- Exceptions are caught at the relevant level and may be added to a separate exception queue
- The main program handles all commands from the command stream and extracts events to put in the event stream
- The central data structure of the main program will be a list/map of cameras with unique IDs
  - IDs should be something like a MAC address that can be found during discovery (without explicitly connecting to camera)
  - Cameras can be populated/added through discovery or restored from persistent storage
  - Camera list is updated using events
  - Camera objects contain necessary information to connect to the camera (like CameraWrapper or Factory\<Camera>)
  - Each camera-specific command specifies the target camera via ID
  - Events are consolidated from individual camera queues, using camera IDs to specify origin