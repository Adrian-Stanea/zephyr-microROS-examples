# MicroROS Docker Compose Setup

This Docker Compose setup provides an easy way to run MicroROS services with configurable device settings.

## Quick Start

1. Edit `.env` to match your device configuration:
   ```
   DEVICE_PATH=/dev/ttyUSB0
   BAUD_RATE=115200
   ```

2. Run the desired service using profiles:

   ### MicroROS Agent
   ```bash
   # Run the MicroROS agent
   docker compose --profile agent up

   # Run in detached mode
   docker compose --profile agent up -d
   ```

   ### MicroROS Base (Development Environment)
   ```bash
   # Run the base environment for development
   docker compose --profile base up

   # Or use the development profile (same as base)
   docker compose --profile development up
   ```

## Available Services

### micro-ros-agent
- **Image**: `microros/micro-ros-agent:humble`
- **Purpose**: Runs the MicroROS agent for serial communication
- **Profile**: `agent`
- **Auto-restart**: Yes (unless stopped)

### micro-ros-base
- **Image**: `microros/base:humble`
- **Purpose**: Development environment with bash shell
- **Profile**: `base`, `development`
- **Auto-restart**: No
- **Workspace**: Current directory mounted at `/workspace`

## Configuration

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `DEVICE_PATH` | `/dev/ttyUSB0` | Serial device path |
| `BAUD_RATE` | `115200` | Serial communication baud rate |

### Common Device Paths
- `/dev/ttyUSB0` - USB to Serial adapters
- `/dev/ttyACM0` - Arduino-compatible boards
- `/dev/ttyUSB1` - Alternative USB serial port

### Common Baud Rates
- `9600`, `115200`, `230400`, `460800`, `921600`

## Usage Examples


### Basic Agent Run
```bash
docker compose run --rm micro-ros-agent
```

### Custom Device and Baud Rate
Edit `.env`:
```
DEVICE_PATH=/dev/ttyACM0
BAUD_RATE=921600
```
Then run:
```bash
docker compose run --rm micro-ros-agent
```

### Run Both Services
```bash
# Run agent in background, then development environment
docker compose run --rm micro-ros-agent
docker compose run --rm micro-ros-base
```

## Troubleshooting

### Permission Issues
If you get permission denied errors, ensure your user is in the `dialout` group:
```bash
sudo usermod -a -G dialout $USER
```
Then log out and log back in.

### Device Not Found
1. Check if the device exists: `ls -la /dev/tty*`
2. Update the `DEVICE_PATH` in your `.env` file
3. For USB devices, try unplugging and reconnecting

### Container Issues
```bash
# Stop all services
docker compose down

# Remove containers
docker compose down --remove-orphans
```
