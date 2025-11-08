# Jettison Command TX

A C++ WebSocket client for sending zoom commands to Jettison GUI cameras (heat/day) with **keyboard control**, **client-side validation**, and **named pipe video streaming**.

## Table of Contents

- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [Connection Details](#connection-details)
- [Command Validation](#command-validation)
- [Building](#building)
- [Dependencies](#dependencies)
- [Protocol Definition](#protocol-definition)
- [Troubleshooting](#troubleshooting)
- [License](#license)
- [Related Projects](#related-projects)

## Features

- **Dual Camera Control**: Separate executables for heat (thermal) and day (visible light) cameras
- **WebSocket Command Streaming**: Send zoom commands via WebSocket (wss://) with proper origin validation
- **Client-Side Command Validation**: Uses [buf.validate](https://github.com/bufbuild/protovalidate) with [protovalidate-cc](https://github.com/bufbuild/protovalidate-cc) to validate all commands before sending - invalid commands are rejected locally with detailed error messages
- **JSON Debug Output**: Pretty-printed JSON representation of all commands (both valid and rejected)
- **Video Streaming**: Receive H.264 video frames via separate WebSocket and output to named pipes
- **Keyboard Control**: Interactive zoom control (optical positions + digital zoom levels)
- **Named Pipe Output**: Zero-dependency video output to `/tmp/jettison_heat.h264` or `/tmp/jettison_day.h264`
- **Portable AppImage**: Dual executables (`heat-ctl` and `day-ctl`) with all dependencies bundled

## Installation

### Quick Start: Download AppImage (Recommended)

Download the pre-built AppImages from the [Releases](https://github.com/YOUR_USERNAME/demo_jettison_cmd_tx_cpp/releases) page:

```bash
# Download the AppImages
wget https://github.com/YOUR_USERNAME/demo_jettison_cmd_tx_cpp/releases/latest/download/heat-ctl-1.0.0-linux-x86_64.AppImage
wget https://github.com/YOUR_USERNAME/demo_jettison_cmd_tx_cpp/releases/latest/download/day-ctl-1.0.0-linux-x86_64.AppImage

# Make them executable
chmod +x heat-ctl-*.AppImage day-ctl-*.AppImage

# Run them
./heat-ctl-1.0.0-linux-x86_64.AppImage --help
./day-ctl-1.0.0-linux-x86_64.AppImage --help
```

**Benefits of AppImage:**
- No installation required - single executable files
- No dependencies to install - everything bundled
- Works on any Linux with glibc 2.35+ (Ubuntu 22.04+)
- Completely portable - run from any directory

## Usage

### Heat Camera Control

```bash
./heat-ctl-1.0.0-linux-x86_64.AppImage sych.local
```

**Keyboard Controls:**
- `1-5`: Optical zoom positions (A-E for heat camera)
- `a-k`: Digital zoom levels (1.0x - 6.0x in 0.5x increments)
- `z/Z`: Send invalid zoom command (tests client-side validation)
- `q/Q`: Quit

### Day Camera Control

```bash
./day-ctl-1.0.0-linux-x86_64.AppImage sych.local
```

**Keyboard Controls:**
- `1-5`: Optical zoom positions (I-V for day camera)
- `a-k`: Digital zoom levels (1.0x - 6.0x in 0.5x increments)
- `z/Z`: Send invalid zoom command (tests client-side validation)
- `q/Q`: Quit

### View Video Stream

In a separate terminal, use `ffplay` or `mpv` to view the H.264 stream:

**FFplay (recommended):**
```bash
# For heat camera
ffplay -fflags nobuffer -flags low_delay -framedrop /tmp/jettison_heat.h264

# For day camera
ffplay -fflags nobuffer -flags low_delay -framedrop /tmp/jettison_day.h264
```

**MPV:**
```bash
# For heat camera
mpv --no-cache --untimed /tmp/jettison_heat.h264

# For day camera
mpv --no-cache --untimed /tmp/jettison_day.h264
```

## Connection Details

### WebSocket Endpoints

**Heat Camera:**
- **Command**: `wss://sych.local:443/ws/ws_cmd`
- **Video**: `wss://sych.local:443/ws/ws_rec_video_heat` (H.264 recording stream)

**Day Camera:**
- **Command**: `wss://sych.local:443/ws/ws_cmd`
- **Video**: `wss://sych.local:443/ws/ws_rec_video_day` (H.264 recording stream)

### Origin Requirement

**IMPORTANT**: The nginx server requires the WebSocket `Origin` header to match:
- `https://$host` (e.g., `https://sych.local`), OR
- `https://$host:443` (e.g., `https://sych.local:443`)

Connections without the correct origin will be rejected with HTTP 403 Forbidden.

### Protocol Details

- **Command Protocol**: Binary Protocol Buffers (`ser.JonGUICmd`)
- **Video Protocol**: H.264 video with 24-byte header (PTS, duration, system time)
- **SSL/TLS**: Self-signed certificates accepted (certificate validation disabled)
- **Message Format**: Commands include client timestamp for latency tracking
- **Video Output**: Named pipe (FIFO) with 5MB buffer - frames dropped if no reader attached or pipe full

## Command Validation

This application uses [buf.validate](https://github.com/bufbuild/protovalidate) to validate all commands **before** sending them to the server. Invalid commands are rejected locally with detailed error messages, preventing bad data from reaching the camera system.

### How It Works

All Protocol Buffer messages are validated using constraints defined in the `.proto` files:

```proto
message SetZoomTableValue {
  int32 value = 1 [(buf.validate.field).int32 = {
    gte: 0  // value must be >= 0
  }];
}
```

When you send a command, the application:
1. Builds the protobuf message
2. Validates it against buf.validate constraints
3. **Only sends valid commands** - invalid commands are rejected with detailed errors

### Validation Example

**Testing Invalid Commands:**

Press `z` to send an invalid zoom command (-1 violates the `gte: 0` constraint):

```
→ Queued INVALID optical zoom (index: -1)
  Testing buf.validate - command should be rejected locally
[DEBUG] Building command with optical_zoom_index = -1 (testing validation)

✗✗✗ VALIDATION FAILED - Command rejected ✗✗✗
Validation errors (1 total):
  - Field 'day_camera.zoom.set_zoom_table_value.value': value must be greater than or equal to 0 (rule: int32.gte)

Rejected command (JSON):
{
 "protocol_version": 1,
 "client_type": "JON_GUI_DATA_CLIENT_TYPE_LOCAL_NETWORK",
 "frame_time_day": "9809400000000000",
 "state_time": "277944132524322000",
 "client_time_ms": "1762636706996",
 "day_camera": {
  "zoom": {
   "set_zoom_table_value": {
    "value": -1
   }
  }
 }
}
```

**Valid Command (for comparison):**

Press `1` to send a valid zoom command:

```
→ Queued optical zoom to position 0 ('I')

Command (JSON):
{
 "protocol_version": 1,
 "client_type": "JON_GUI_DATA_CLIENT_TYPE_LOCAL_NETWORK",
 "frame_time_day": "9312700000000000",
 "state_time": "277447419252103000",
 "client_time_ms": "1762636210272",
 "day_camera": {
  "zoom": {
   "set_zoom_table_value": {
    "value": 0
   }
  }
 }
}

→ Sent optical zoom command (index: 0)
```

### Benefits of Client-Side Validation

- **Data Integrity**: Prevents invalid commands from reaching the camera system
- **Fast Feedback**: Immediate validation errors without round-trip to server
- **Detailed Errors**: Clear error messages showing exactly which field violated which constraint
- **JSON Output**: Rejected commands are displayed as JSON for debugging

### Validation in Action

The validation system uses [protovalidate-cc](https://github.com/bufbuild/protovalidate-cc) with full CEL expression support, providing:

- **Range constraints**: `gte`, `lte`, `gt`, `lt`
- **Enum validation**: `defined_only`
- **String validation**: `min_len`, `max_len`, `pattern`
- **Complex expressions**: Custom CEL expressions for advanced validation

For more details on validation constraints, see the [jettison_protogen](https://github.com/lpportorino/jettison_protogen) repository where `.proto` files are defined.

## Building

### Docker Build (Recommended)

Build both heat_ctl and day_ctl AppImages with a single Docker command:

```bash
# Clone with submodules
git clone --recursive git@github.com:YOUR_USERNAME/demo_jettison_cmd_tx_cpp.git
cd demo_jettison_cmd_tx_cpp

# Build both AppImages
./scripts/build.sh
```

The build script will:
1. ✓ Check for required tools (docker, git)
2. ✓ Initialize git submodules (if needed)
3. ✓ Build both heat_ctl and day_ctl in Docker
4. ✓ Extract AppImages to project root
5. ✓ Test AppImages for basic functionality

**Build Script Options:**

```bash
./scripts/build.sh --clean    # Remove old AppImages first
./scripts/build.sh --help     # Show all options
```

### Docker Stages

The Dockerfile uses a multi-stage build:

1. **Dependencies** (layers cached for fast rebuilds):
   - IXWebSocket 11.4.5 (fetched via CMake FetchContent)
   - Abseil 20240722.0
   - Protocol Buffers 29.2
   - RE2 2024-07-02
   - protovalidate-cc v1.0.0-rc.2 (with vendored CEL-C++)

2. **Build heat_ctl** (`/build-heat`):
   - Compiled with `-DCAMERA_TYPE=HEAT`
   - Installed to `/usr/local/bin/heat_ctl`

3. **Build day_ctl** (`/build-day`):
   - Compiled with `-DCAMERA_TYPE=DAY`
   - Installed to `/usr/local/bin/day_ctl`

4. **AppImage Creation**:
   - Both binaries packaged with linuxdeploy
   - All dynamic libraries bundled

5. **Export**:
   - `FROM scratch AS export` - extracts AppImages (for docker buildx)
   - `FROM ubuntu:22.04 AS runtime` - runtime stage (for regular docker)

## Dependencies

The application uses the following libraries (all bundled in AppImage):

- **[Protobuf 29.2](https://github.com/protocolbuffers/protobuf)**: Protocol buffer serialization
- **[protovalidate-cc v1.0.0-rc.2](https://github.com/bufbuild/protovalidate-cc)**: Runtime buf.validate constraint validation
- **[Abseil 20240722.0](https://github.com/abseil/abseil-cpp)**: Google's C++ common libraries (required by Protobuf 29.2)
- **[IXWebSocket 11.4.5](https://github.com/machinezone/IXWebSocket)**: Modern C++11 WebSocket client with built-in multi-threading and SSL/TLS support
- **[RE2 2024-07-02](https://github.com/google/re2)**: Regular expression library (used by CEL-C++)
- **[CEL-C++](https://github.com/google/cel-cpp)**: Common Expression Language (vendored by protovalidate-cc)
- **OpenSSL**: SSL/TLS for secure WebSocket connections

## Protocol Definition

This project uses the **Jettison Protocol** defined in Protocol Buffers with [buf.validate](https://github.com/bufbuild/protovalidate) constraints.

### Source Repositories

**Protocol Source:**
- [jettison_protogen](https://github.com/lpportorino/jettison_protogen.git) - Source repository containing:
  - Protocol Buffer definitions (`.proto` files in `./proto`)
  - buf.validate constraint annotations
  - Code generation scripts
  - GitHub Actions automation for generating C++/Python/etc. code

**Generated Code (Git Submodule):**
- [jettison_proto_cpp](https://github.com/lpportorino/jettison_proto_cpp.git) - **Auto-generated** by jettison_protogen CI/CD
  - Pre-compiled C++ headers (`.pb.h`) and implementations (`.pb.cc`)
  - Generated from Protocol Buffers 29.2
  - Automatically updated when proto files change
  - Used as a git submodule in this project (`jettison_proto_cpp/`)

**Important:** Do not manually edit files in `jettison_proto_cpp/` - they are automatically generated and pushed by the jettison_protogen GitHub Actions workflow.

## Troubleshooting

### Connection Timeout

**Symptom:**
```
Connecting to command endpoint...
✗ WebSocket connection timeout
✗ WebSocket connection error: Closed before conn
✗ Failed to connect to command endpoint
```

**Possible Causes:**

1. **Origin header requirement**
   - The nginx server expects `Origin: https://sych.local` or `Origin: https://sych.local:443`
   - IXWebSocket automatically sets the correct Origin header based on the connection URL
   - Check nginx logs: `docker logs -f jettison_web 2>&1 | grep 403`

2. **WebSocket connection issues**
   - Verify the WebSocket endpoints are accessible
   - Check that SSL/TLS is properly configured for self-signed certificates

3. **SSL certificate configuration**
   - The application automatically accepts self-signed certificates
   - IXWebSocket handles SSL/TLS certificate validation

4. **Server not reachable**
   ```bash
   ping sych.local
   curl -k -I https://sych.local/
   ```

5. **WebSocket endpoint not running**
   - Check if the command service is running on the Jettison system
   - Verify nginx is proxying to the correct backend port (8083)

6. **Firewall blocking connections**
   - Ensure port 443 is accessible from your network

### SSL/TLS Errors

- The application disables certificate validation by design (for self-signed certs)
- SSL errors should not occur unless OpenSSL is misconfigured

### Video Not Displaying

1. **Named pipe not found**
   - Check that `/tmp/jettison_heat.h264` or `/tmp/jettison_day.h264` exists
   - Pipe is created automatically on startup

2. **ffplay showing artifacts**
   - This is normal - first frames may not be keyframes
   - Wait a few seconds for a keyframe to arrive

3. **No video frames**
   - Verify the video WebSocket endpoint is accessible
   - Check that the camera is streaming video on the server

### Build Errors

1. **Submodule not initialized**
   ```bash
   git submodule update --init --recursive
   ```

2. **Docker build fails**
   - Ensure Docker is running and has internet access
   - Check Docker logs for specific errors
   - Try cleaning Docker cache: `docker system prune -a`

## License

This project is licensed under the GNU General Public License v3.0 or later (GPL-3.0-or-later).

See the [LICENSE](LICENSE) file for details.

## Related Projects

- **[jettison_protogen](https://github.com/lpportorino/jettison_protogen)**: Protocol Buffer source definitions
- **[jettison_proto_cpp](https://github.com/lpportorino/jettison_proto_cpp)**: Auto-generated C++ protobuf code
- **[demo_jettison_state_rx_cpp](https://github.com/YOUR_USERNAME/demo_jettison_state_rx_cpp)**: WebSocket client for receiving GUI state messages
