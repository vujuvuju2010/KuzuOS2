/* orconfig.h - Configuration header for Tor on KuzuOS
 * This replaces the autoconf-generated config for KuzuOS
 * Must be included BEFORE any other Tor headers
 */
#ifndef TOR_ORCONFIG_H
#define TOR_ORCONFIG_H

#include <stdint.h>
#include <stddef.h>

/* Time types - must be defined before Tor headers use them */
typedef long time_t;
typedef long suseconds_t;
typedef long clock_t;

/* Timeval structure - required for struct timeval in Tor code */
struct timeval {
    time_t      tv_sec;   /* seconds */
    suseconds_t tv_usec;  /* microseconds */
};

/* Timespec structure */
struct timespec {
    time_t tv_sec;   /* seconds */
    long   tv_nsec;  /* nanoseconds */
};

/* Broken-down time structure */
struct tm {
    int tm_sec;    /* seconds after the minute [0-60] */
    int tm_min;    /* minutes after the hour [0-59] */
    int tm_hour;   /* hours since midnight [0-23] */
    int tm_mday;   /* day of the month [1-31] */
    int tm_mon;    /* months since January [0-11] */
    int tm_year;   /* years since 1900 */
    int tm_wday;   /* days since Sunday [0-6] */
    int tm_yday;   /* days since January 1 [0-365] */
    int tm_isdst;  /* Daylight Saving Time flag */
};

/* Version info */
#define PACKAGE_NAME "tor"
#define PACKAGE_TORNAME "Tor"
#define PACKAGE_VERSION "0.4.8.9"
#define PACKAGE_STRING "Tor 0.4.8.9"
#define VERSION "0.4.8.9"
#define TOR_VERSION "0.4.8.9"

/* Platform */
#define KUZUOS 1

/* Type sizes for x86_64 */
#define SIZEOF_CHAR 1
#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 8
#define SIZEOF_LONG_LONG 8
#define SIZEOF_SIZE_T 8
#define SIZEOF_VOID_P 8
#define SIZEOF_INT16_T 2
#define SIZEOF_INT32_T 4
#define SIZEOF_INT64_T 8
#define SIZEOF_IN_ADDR_T 4
#define SIZEOF_TIME_T 8
#define SIZEOF_SOCKLEN_T 4

/* Have standard types - let Tor's torint.h handle the rest */
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDBOOL_H 1
#define HAVE_SSIZE_T 1
#define USING_TWOS_COMPLEMENT 1

/* Platform checks - x86_64 is standard */
#define NULL_REP_IS_ZERO_BYTES 1
#define DOUBLE_0_REP_IS_ZERO_BYTES 1

/* We have struct timeval defined in time.h */
#define HAVE_STRUCT_TIMEVAL 1
#define HAVE_STRUCT_TIMEVAL_TV_SEC 1
#define HAVE_STRUCT_TIMEVAL_TV_USEC 1
#define HAVE_STRUCT_TIMESPEC 1
#define HAVE_STRUCT_TM 1
#define HAVE_STRUCT_TM_TM_GMTOFF 1

/* Define time_t for Tor headers that don't include time.h */
typedef long time_t;
typedef unsigned int socklen_t;
typedef uint16_t sa_family_t;

/* Endianness - x86_64 is little endian */
#define WORDS_SIZEOF_VOID_P 8
/* Don't define WORDS_BIGENDIAN - we're little endian */

/* Filesystem paths for KuzuOS */
#define DEFAULT_DATA_DIR "/tor/data"
#define DEFAULT_KEY_DIR "/tor/keys"
#define DEFAULT_CACHE_DIR "/tor/cache"
#define DEFAULT_LOG_DIR "/tor/log"
#define DEFAULT_CONF_FILE "/tor/torrc"

/* Path constants for Tor config */
#define SHARE_DATADIR "/tor/share"
#define CONFDIR "/tor/etc"
#define LOCALSTATEDIR "/tor/var"
#define PATH_SEPARATOR "/"

/* Compiler info */
#define COMPILER "gcc"
#define COMPILER_VENDOR "gnu"
#define COMPILER_VERSION __VERSION__

/* Default ports */
#define DEFAULT_OR_PORT 9001
#define DEFAULT_DIR_PORT 9030
#define DEFAULT_SOCKS_PORT 9050
#define DEFAULT_CONTROL_PORT 9051
#define DEFAULT_DNS_PORT 5400

/* Cell sizes - let Tor define these */
#define CELL_SIZE 512

/* Protocol versions */
#define MIN_LINK_VERSION 3
#define MAX_LINK_VERSION 4

/* Onion service version */
#define HS_VERSION_V3 3

/* Have SHA functions */
#define HAVE_SHA256 1
#define HAVE_SHA512 1

/* Disable features not available on KuzuOS */
#undef HAVE_SYS_SOCKET_H
#undef HAVE_NETINET_IN_H
#undef HAVE_ARPA_INET_H
#define HAVE_UNISTD_H 1
#undef HAVE_FCNTL_H
#undef HAVE_SYS_STAT_H
#define HAVE_SYS_TIME_H 1
#undef HAVE_POLL_H
#undef HAVE_SYS_EPOLL_H
#undef HAVE_SYS_EVENT_H
#undef HAVE_NETDB_H
#define HAVE_PTHREAD_H 1
#undef HAVE_FORK
#undef HAVE_EXECVE
#define HAVE_CHMOD 1
#undef HAVE_OPENSSL
#undef HAVE_LIBEVENT
#undef HAVE_ZLIB
#undef HAVE_LZMA
#undef HAVE_SYSTEMD
#undef HAVE_SECCOMP
/* No clock_gettime - use gettimeofday instead */
#undef HAVE_CLOCK_GETTIME

/* Disable threading - use single-threaded mode */
#define DISABLE_THREADS 1
#define TOR_DISABLE_THREADING 1

/* We use our own implementations */
#define CRYPTO_BACKEND_KUZU 1
#define USE_SIMPLE_EVENT_LOOP 1

/* Inline and attributes - Tor needs these specific definitions */
#define inline __inline__
/* Don't override __func__ - Tor defines it in compat_compiler.h */
/* Don't override __attribute__ - Tor uses it for type checking */
#define restrict

/* Tor attribute macros */
#define TOR_UNUSED __attribute__((unused))
#define TOR_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define TOR_PRINTF(fmt,arg) __attribute__((format(printf,fmt,arg)))
#define TOR_NORETURN __attribute__((noreturn))
#define STATIC_ASSERT(cond) _Static_assert(cond, #cond)
/* Don't define ARRAY_LENGTH - Tor defines it in compat_compiler.h */

/* Flexible array member */
#define FLEXIBLE_ARRAY_MEMBER

/* Error codes - standard POSIX values */
#define EAGAIN 11
#define EWOULDBLOCK EAGAIN
#define EINPROGRESS 115
#define EINTR 4
#define ECONNREFUSED 111
#define ETIMEDOUT 110
#define ECONNRESET 104
#define EPIPE 32
#define ENOTCONN 107
#define EISCONN 106
#define ENETUNREACH 101
#define EHOSTUNREACH 113

/* Socket constants */
#define AF_INET 2
#define AF_INET6 10
#define AF_UNIX 1
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17

/* File access */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 64
#define O_TRUNC 512
#define O_APPEND 1024

/* Seek modes */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* Signals */
#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGPIPE 13
#define SIGTERM 15
#define SIGUSR1 10
#define SIGUSR2 12

/* Time */
#define CLOCKS_PER_SEC 1000000

/* Path limits */
#define PATH_MAX 4096
#define NAME_MAX 255
#define HOST_NAME_MAX 255

/* Boolean - let stdbool.h handle this */

/* NULL - let stddef.h handle this */

/* Format macros */
#define PRId64 "lld"
#define PRIu64 "llu"
#define PRIx64 "llx"
#define PRIu32 "u"
#define PRId32 "d"
#define PRIx32 "x"

/* Tor-specific constants */
#define MAX_CIRCUITS_PER_CONN 1024
#define DEFAULT_MAX_CIRCUITS 8000
#define MAX_CONNECTIONS 1024
#define DEFAULT_CONN_LIMIT 500

/* Stream end reasons - let Tor define these */
#define END_STREAM_REASON_NONE 0
#define END_STREAM_REASON_MISC 1
#define END_STREAM_REASON_REFUSED 2

/* Circuit purposes - let Tor define these */

/* Connection types - let Tor define these */

/* Directory purposes - let Tor define these */

/* Link protocol version */
#define MIN_PROTOCOL_VERSION 3
#define MAX_PROTOCOL_VERSION 4

/* Handshake types - let Tor define these */

/* Consensus flavors - let Tor define these */

/* Directory authority flags */
#define V3_DIRSERVER 0x40

/* Router status flags */
#define ROUTER_IS_AUTHORITY 0x01
#define ROUTER_IS_RUNNING 0x02
#define ROUTER_IS_VALID 0x04
#define ROUTER_IS_FAST 0x08
#define ROUTER_IS_STABLE 0x10
#define ROUTER_IS_GUARD 0x20
#define ROUTER_IS_EXIT 0x40
#define ROUTER_IS_BAD_EXIT 0x80

/* Address types */
#define AF_TYPE_INET 2
#define AF_TYPE_INET6 10

/* Maximum sizes - let Tor define these properly */
#define MAX_NICKNAME_LEN 19
#define HEX_DIGEST_LEN 40
#define DIGEST_LEN 20
#define DIGEST256_LEN 32

/* Ed25519 key lengths */
#define ED25519_PUBKEY_LEN 32
#define ED25519_SECKEY_LEN 64
#define ED25519_SIGNATURE_LEN 64

/* Curve25519 key lengths */
#define CURVE25519_PUBKEY_LEN 32
#define CURVE25519_SECKEY_LEN 32

/* NTOR handshake */
#define NTOR_KEY_LEN 32
#define NTOR_REPLY_LEN 136
#define NTOR_INITIATOR_KEY_LEN 32
#define NTOR_SERVER_KEY_LEN 32

/* Timestamp format */
#define ISO_TIME_LEN 19
#define ISO_TIME_LEN_SECS 19

/* Log domains - let Tor define these with proper types */

/* Log severities - let Tor define these */

/* Bootstrap status - let Tor define these */

/* Control event types */
#define EVENT_CIRC 0
#define EVENT_STREAM 1
#define EVENT_ORCONN 2
#define EVENT_BW 3
#define EVENT_LOG 4
#define EVENT_STATUS 5

/* Controller commands */
#define CONTROL_COMMAND_AUTHENTICATE 1
#define CONTROL_COMMAND_TAKEOWNERSHIP 2
#define CONTROL_COMMAND_SIGNAL 3
#define CONTROL_COMMAND_NEWNYM 4
#define CONTROL_COMMAND_RESOLVED 5

/* SOCKS5 constants */
#define SOCKS5_VERSION 5
#define SOCKS5_AUTH_NONE 0
#define SOCKS5_AUTH_GSSAPI 1
#define SOCKS5_AUTH_PASSWORD 2
#define SOCKS5_AUTH_NO_ACCEPT 255
#define SOCKS5_CMD_CONNECT 1
#define SOCKS5_CMD_BIND 2
#define SOCKS5_CMD_UDP_ASSOC 3
#define SOCKS5_ATYPE_IPV4 1
#define SOCKS5_ATYPE_DOMAIN 3
#define SOCKS5_ATYPE_IPV6 4

/* Magic numbers for type checking - let Tor define these */

/* Circuit build state */
#define CIRCUIT_BUILD_STATE_NEW 0
#define CIRCUIT_BUILD_STATE_EXTENDING 1
#define CIRCUIT_BUILD_STATE_OPEN 2
#define CIRCUIT_BUILD_STATE_CLOSING 3

/* OR connection state */
#define OR_CONN_STATE_OR_HANDSHAKING 1
#define OR_CONN_STATE_OR_HANDSHAKED 2
#define OR_CONN_STATE_OR_CIRCUIT_CLEAN 3
#define OR_CONN_STATE_OR_CIRCUIT_BUSY 4
#define OR_CONN_STATE_OR_CLOSE_WAIT 5
#define OR_CONN_STATE_OR_CLOSE_NEEDED 6

/* Stream state */
#define STREAM_STATE_NEW 0
#define STREAM_STATE_SENT_CONNECT 1
#define STREAM_STATE_SENT_RESOLVE 2
#define STREAM_STATE_SUCCEEDED 3
#define STREAM_STATE_FAILED 4
#define STREAM_STATE_CLOSED 5
#define STREAM_STATE_DETACHED 6

/* Directory request status */
#define DIR_CONN_STATE_NEW 0
#define DIR_CONN_STATE_CONNECTING 1
#define DIR_CONN_STATE_SERVER_HANDSHAKING 2
#define DIR_CONN_STATE_CLIENT_HANDSHAKING 3
#define DIR_CONN_STATE_OPEN 4
#define DIR_CONN_STATE_FLUSHING 5
#define DIR_CONN_STATE_CLOSE_WAIT 6

/* Directory response status */
#define DIR_RESPONSE_NONE 0
#define DIR_RESPONSE_OK 1
#define DIR_RESPONSE_NOT_FOUND 2
#define DIR_RESPONSE_BAD_REQUEST 3
#define DIR_RESPONSE_INTERNAL_ERROR 4

/* Router hash algorithm - let Tor define */

/* Key exchange types - let Tor define these */

/* Certificate types - let Tor define these */

/* Protocol list */
#define PROTOINFO_NO_EXTRA_INFO 0
#define PROTOINFO_EXTRA_INFO 1

/* Directory cache constants */
#define MAX_DIR_SIZE (10*1024*1024)
#define MAX_MICRODESC_SIZE 100000
#define MAX_ROUTERS_IN_CONSENSUS 10000
#define MAX_MICRODESCRIPTORS 6000

/* Time constants */
#define CRYPTO_NO_EXPIRATION 0
#define CRYPTO_EXPIRES_ALL 1
#define CRYPTO_EXPIRES_SOON 2

/* Bandwidth rate constants */
#define BW_RATE_MIN 1024
#define BW_RATE_MAX (1024*1024*1024)
#define BW_BURST_MIN 1024
#define BW_BURST_MAX (1024*1024*1024)

/* Circuit padding */
#define CIRCUIT_PADDING_DISABLED 0
#define CIRCUIT_PADDING_ENABLED 1

/* Congestion control */
#define CONGESTION_CONTROL_DISABLED 0
#define CONGESTION_CONTROL_ENABLED 1

/* DOS defense */
#define DOS_OVERLOAD_ENABLED 1
#define DOS_OVERLOAD_DISABLED 0

/* Shared random */
#define SR_COMMITMENT_LEN 32
#define SR_VALUE_LEN 32
#define SR_PERIOD_DURATION (24*60*60)

/* Key pinning */
#define KEYPIN_JOURNAL_ENTRIES 1000

/* Testing mode */
#undef TOR_UNIT_TESTS
#undef TOR_FUZZ_TESTS

/* Debugging */
#undef DEBUG
#undef VERBOSE_LOGGING

/* Sandbox - disabled for now */
#undef ENABLE_SECCOMP
#undef USE_LIBSECCOMP
#undef ENABLE_FRAGILE_HARDENING

/* Rust - not available */
#undef HAVE_RUST

/* LTTNG tracing */
#undef USE_LTTNG

/* Transport plugins */
#undef HAVE_TRANSPORTS
#define DISABLE_TRANSPORTS 1

/* Bridge client */
#define DISABLE_BRIDGE_CLIENT 1

/* Directory server */
#undef DISABLE_DIRSERVER

/* Tor2web */
#undef TOR2WEB

#endif /* TOR_ORCONFIG_H */
