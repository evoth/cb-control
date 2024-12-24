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

## More Ideas
- Both the command and event streams consist of the same "events"
- Events are simply state mutations
  - Commands are requested mutations
  - Events are mutations that have already happened
- The "cameras" within the control module don't have to hold any state; they're simply channels for events (back and forth)
- The actual state is reconstructed on the other end of the streams (or somewhere along the way)
  - The state reconstruction can be done in C++ and/or JavaScript... hmm...
- It's possible that the sequence module can operate entirely on events without having to worry about state
- Everything is events

## Even More Ideas
- The main data structure is sorta like a nested map
- Each state change can be distilled to a camera ID, prop ID, and value (the type of the prop is already tied to the prop ID)
  - On either side, if the camera/property in question doesn't yet exist, it is created
  - A command will only induce a state change if it is different to the current state
  - Examples:
    - Command (cameraId=1, connected=true)
      - Assuming camera with id=1 doesn't exist yet, it will be created within the local state and connected
      - But of course, information is needed to connect to the camera
      - Which brings me to my next point
- Certain state properties have to be changed together (okay it's kind of like function/event parameters but bear with me), which is serialized in the different event packet types
- But maybe these "properties that have to be changed together" are better defined as a single property (with multiple fields) that can only be changed atomically
  - For example, the connection information for a camera can be a single property, which will take different forms for PTP/IP, PTP/USB, Bluetooth, etc. So changing the property would destroy the underlying camera implementation and create a new one with the appropriate type
- Forget commands/events; there are two types of **events**: **requests** and **results**. They are essentially the same (state change) except for a boolean indicator
- Requests trigger necessary actions to change the state from its current value to the value in the request (doesn't change the state directly)
- Results overwrite the current state
- One state object can **control** another by only outputting requests and only receiving results, while the other does the opposite
- The state object being controlled also listens to its own result events
- Example: The backend is a state object in C++ which controls cameras in response to requests. Its results are piped to the frontend, which updates the UI. When a user interacts with the UI, it produces requests which are sent to the backend.