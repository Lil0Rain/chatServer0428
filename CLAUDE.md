# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
cd /home/userme/ChatServer/chatServer0428
mkdir -p build && cd build
cmake ..
make
```

Binaries are output to `bin/` at the project root: `ChatServer` and `ChatClient`.

## Run

```bash
# Server
./bin/ChatServer <ip> <port>    # e.g., ./bin/ChatServer 192.168.112.129 6000

# Client (terminal-based interactive chat)
./bin/ChatClient <ip> <port>
```

## Database config

`mysql.ini` at the project root controls the MySQL connection pool (IP, port, credentials, pool size, idle timeout). The `chat` database is assumed to exist with tables: `user`, `friend`, `OfflineMessage`, `AllGroup`, `GroupUser`.

## Architecture

### Message dispatch flow

```
Client TCP → ChatServer (muduo) → onMessage → JSON parse → ChatService::getHandler(msgid) → handler lambda
```

`ChatService` is a singleton. It maps `EnMsgType` enum values (defined in `include/public.hpp`) to handler methods via `_msgHandlerMap`. The ChatServer wrapper (`chatserver.hpp`/`.cpp`) is thin — connection/message callbacks delegate directly to `ChatService`.

### Cross-server messaging via Redis pub/sub

Each online user subscribes to a Redis channel named by their user ID. When the target user of a chat message is connected to a *different* server instance, the sending server publishes to that channel. A background thread (`observer_channel_message`) in `Redis` reads incoming publishes and calls back to `ChatService::handleRedisSubscribeMessage` to deliver locally. Two separate `redisContext` handles are maintained: one for publish (blocking), one for subscribe (non-blocking).

### MySQL connection pool

`ConnectionPool` is a singleton in `src/ConnectionPool/`. It reads `mysql.ini`, maintains a `queue<Connection*>`, and runs two background threads:
- **Producer** — creates new connections when the queue is empty (up to `_maxSize`)
- **Scanner** — periodically destroys idle connections exceeding `_initialSize` and `_maxIdleTime`

`getConnection()` returns `shared_ptr<Connection>` with a custom deleter that returns the connection to the pool rather than destroying it.

### Data access layer

Each table has a corresponding data-access class in `src/server/model/` and header in `include/server/model/`:

| Class | Table | Operations |
|---|---|---|
| `userMuduo` | `user` | insert, query by id/name, updateState, resetState |
| `friendMuduo` | `friend` | add friendship, query friends (JOIN user) |
| `groupMuduo` | `AllGroup` + `GroupUser` | create group, add member, query groups/members |
| `offlineMsgMuduo` | `OfflineMessage` | queue messages for offline users, retrieve on login, delete after delivery |

ORM data classes (`User`, `Group`, `GroupUser`) are pure data containers in `include/server/model/`.

### Message protocol

All communication uses JSON with an `msgid` field matching the `EnMsgType` enum (`include/public.hpp`). The server dispatches based on `msgid`; the client's receive thread dispatches incoming JSON to handlers based on the same enum values.

### Key headers

- `include/public.hpp` — shared by server and client: `EnMsgType` enum, `LOG` macro
- `include/server/chatservice.hpp` — business logic singleton, message handler map
- `include/server/chatserver.hpp` — muduo TcpServer wrapper
- `include/db/db.h` — MySQL connection wrapper (uses a pooled Connection)
- `include/ConnectionPool/CommonConnectionPool.h` — thread-safe connection pool singleton
- `include/server/redis/redis.hpp` — Redis pub/sub client
