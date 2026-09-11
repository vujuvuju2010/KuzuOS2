# KuzuOS Hidden Service Setup Guide

## What Changed

1. **Removed** simplified Tor implementation (`src/internet/tor.c` and `tor.h`)
2. **Using** full Tor 0.4.8.9 daemon from `src/tor_port/`
3. **Configured** Tor to run as a hidden service for the httpd server

## Architecture

```
┌─────────────────┐
│   Tor Network   │
│   (.onion)      │
└────────┬────────┘
         │
         ▼
   ┌─────────────┐
   │ Tor Daemon  │  Port 9050 (SOCKS)
   │  /dev/tor   │  Port 9051 (Control)
   └──────┬──────┘
          │ Forward port 80
          ▼
   ┌─────────────┐
   │   httpd     │  Port 80
   │ /dev/httpd  │
   └──────┬──────┘
          │
          ▼
    /www/* files
```

## Build and Run

```bash
# Build the ISO with Tor integrated
make clean
make

# Run in QEMU
qemu-system-x86_64 -cdrom kuzuos.iso -m 512M -netdev user,id=net0 -device e1000,netdev=net0
```

## Using the Hidden Service

### 1. Start the HTTP Server

```bash
servicectl start httpd
```

### 2. Start Tor Daemon

```bash
servicectl start tor
```

Tor will:
- Initialize the Tor network connection
- Generate hidden service keys (first time only)
- Create your .onion address
- Start accepting connections

### 3. Get Your .onion Address

```bash
cat /tor/hidden_service/hostname
```

This will display something like:
```
abcd1234efgh5678.onion
```

### 4. Access Your Hidden Service

From any Tor Browser anywhere in the world:
```
http://your-address.onion/
```

## Managing Content

Add files to `/www/` directory:

```bash
# Create a welcome page
echo "<html><body><h1>Welcome to KuzuOS on Tor!</h1></body></html>" > /www/index.html

# Create other pages
mkdir /www/about
echo "<h1>About Page</h1>" > /www/about/index.html
```

## Service Management

```bash
# Check status
servicectl status tor
servicectl status httpd

# View logs
# (Tor logs to stdout - visible in kernel console)

# Restart services
servicectl restart tor
servicectl restart httpd

# Stop services
servicectl stop tor
servicectl stop httpd
```

## Configuration Files

### `/etc/services/torrc`
Main Tor configuration:
- DataDirectory: `/tor/data`
- SocksPort: `9050`
- ControlPort: `9051`
- Hidden service: `/tor/hidden_service` → `127.0.0.1:80`

### `/etc/services/tor.conf`
Service control configuration:
- Auto-start on boot
- Restart on failure
- High priority

## Technical Details

### Hidden Service Directory Structure
```
/tor/hidden_service/
├── hostname           # Your .onion address
├── hs_ed25519_public_key
└── hs_ed25519_secret_key
```

**Important**: Backup the `hidden_service` directory to keep your .onion address!

### Network Flow
1. Tor client connects to your .onion address
2. Connection routed through Tor network
3. Arrives at your Tor daemon (port 9050)
4. Tor forwards to localhost:80 (httpd)
5. httpd serves the requested file
6. Response goes back through Tor network

## Troubleshooting

### Tor won't start
- Check `/tor/data` directory exists and is writable
- Verify torrc syntax: `/etc/services/torrc`
- Look for error messages in console

### Can't access hidden service
- Wait 30-60 seconds after starting Tor (propagation time)
- Verify httpd is running: `servicectl status httpd`
- Test local access: `ip connect 127.0.0.1 80`
- Check hostname file exists: `ls /tor/hidden_service/`

### httpd not responding
- Ensure httpd started: `servicectl start httpd`
- Check files exist in `/www/`
- Verify TCP stack initialized: `ip addr`

## Security Notes

- Only your hidden service is exposed, not your IP address
- All connections encrypted by Tor
- No exit node traffic (we reject all exit traffic)
- Private keys stored in `/tor/hidden_service/`
- Hidden service only accessible via Tor network

## Advanced Configuration

Edit `/etc/services/torrc` to:

### Multiple hidden services
```
HiddenServiceDir /tor/hidden_service
HiddenServicePort 80 127.0.0.1:80

HiddenServiceDir /tor/hidden_service2
HiddenServicePort 80 127.0.0.1:8080
```

### Custom ports
```
HiddenServicePort 80 127.0.0.1:8080
HiddenServicePort 443 127.0.0.1:8443
```

### Access control
```
HiddenServiceAuthorizeClient stealth client1,client2
```

## Files Included in ISO

- `/dev/tor` - Tor daemon executable
- `/etc/services/tor.conf` - Service configuration
- `/etc/services/torrc` - Tor configuration
- `/etc/services/ONION.md` - This documentation
- `/tor/data/` - Tor data directory (empty)
- `/tor/hidden_service/` - Hidden service keys (generated on first run)

## References

- Tor Project: https://www.torproject.org/
- Tor Hidden Services: https://community.torproject.org/onion-services/
- KuzuOS httpd: Uses custom TCP/IP stack with full Tor integration
