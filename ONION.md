# KuzuOS Onion Service (Hidden Service)

## Overview
KuzuOS runs a Tor hidden service that makes the local HTTP server accessible via a .onion address on the Tor network.

## Architecture
```
Internet (Tor Network)
         ↓
    [Tor Daemon]
         ↓
    localhost:80
         ↓
    [httpd server]
         ↓
    /www/* files
```

## How It Works

1. **Tor Daemon** (`/dev/tor`)
   - Runs the full Tor 0.4.8.9 implementation
   - Listens on SOCKS port 9050
   - Control port on 9051
   - Configuration: `/etc/services/torrc`

2. **Hidden Service Configuration**
   - Service directory: `/tor/hidden_service/`
   - Public key and hostname stored in service directory
   - Forwards port 80 to localhost:80 (httpd)

3. **HTTP Server** (`/dev/httpd`)
   - Serves files from `/www/`
   - Handles requests from both clearnet and Tor

## Usage

### Starting the Services

```bash
# Start HTTP server
servicectl start httpd

# Start Tor daemon (this will create the hidden service)
servicectl start tor
```

### Getting Your Onion Address

After Tor starts, your .onion address will be generated:

```bash
# Read the hostname file (once Tor has initialized)
cat /tor/hidden_service/hostname
```

This will show your unique .onion address, something like:
```
abc123def456ghi7.onion
```

### Accessing Your Hidden Service

From any Tor Browser:
```
http://your-address.onion/
```

### Adding Content

Place files in `/www/` directory:
```bash
# Example: create an index page
echo "<h1>Welcome to KuzuOS on Tor!</h1>" > /www/index.html
```

## Service Management

```bash
# Check Tor status
servicectl status tor

# View Tor logs
# (Tor logs to stdout, check kernel/service logs)

# Restart Tor (regenerates keys if needed)
servicectl restart tor

# Stop hidden service
servicectl stop tor
```

## Security Notes

- The hidden service private key is stored in `/tor/hidden_service/`
- Backup this directory to keep your .onion address
- The hidden service is only accessible via Tor network
- No clearnet exposure of your service

## Configuration

Edit `/etc/services/torrc` to customize:
- Change hidden service port mappings
- Add multiple hidden services
- Adjust logging levels
- Configure bandwidth limits

## Troubleshooting

### Hidden service not accessible
1. Ensure httpd is running: `servicectl status httpd`
2. Check Tor is running: `servicectl status tor`
3. Verify httpd is listening on port 80
4. Check Tor logs for errors

### Can't find .onion address
- Wait 30-60 seconds after starting Tor
- Check `/tor/hidden_service/hostname` exists
- Ensure DataDirectory has write permissions

### Connection refused
- Verify httpd is responding: `ip connect 127.0.0.1 80`
- Check firewall/network settings
- Ensure TCP stack is initialized

## Technical Details

- **Tor Version**: 0.4.8.9
- **Protocol**: Tor v3 hidden services (ed25519 keys)
- **Network**: Fully integrated with KuzuOS TCP/IP stack
- **Dependencies**: Custom libc, syscall interface, TCP implementation
