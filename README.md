# robaki

## Overview

TODO demo

[Achtung, die Kurve!](https://en.wikipedia.org/wiki/Achtung,_die_Kurve!) like
game with server client architecture. "Robaki" means "worms" in Polish. This was
an assignment from classes on networking at the University of Warsaw, written in
C++17 with raw sockets and standard library. Architecture consists of three
parts:

1) game server
2) player's client
3) GUI client

A player's client exchanges data packets with the game server that oversees the
game on one hand, on the other hand the client communicates with the GUI client
to draw the game on the player's side. The task was to implement the game server
and the player's client. The GUI was supplied as part of the assignment. GUI
needs cairo and gtk-2 to compile.

### Restrictions

Project was to be written in C11 or C++17 using raw socket programming without
the help of `libevent` or `boost::asio`, intended for Linux.

### How to compile

TODO retest the solution

A [makefile](makefile) is supplied with a default target. It also has a `clean`
and `test` targets for removing output files and for testing respectively.

### Testing

Tests this was graded on were never released, only their description. The
majority of attached tests was written by fellow students, some of them by me.

## Game description

### Game rules

The game is played on a rectangular screen. At least two players take part.
Every player controls a worm's movement. A worm eats the pixel on which it is
located. The game is played in rounds. In each round a worm can move to a
different pixel, leaving the pixel it was on as eaten. A worm moves in the
direction set by the worm's player. If a worm goes to a pixel being eaten, an
eaten pixel or outside the screen, then the worm leaves the screen and the
player leaves the game. The player whose worm stays the longest on the screen
wins. A worm's algorithm is described in detail below.

### Architecture

The game server communicates with clients, manages the game's state, receives
from clients movement information and sends to clients the game state's changes.
The server remembers every event for the current game and sends them to clients
if needed.

The client communicates with the server and the user's interface. The client
ensures that the GUI server receives commands in order consistent with the game
trajectory and without duplicates.

Specification of communication protocols, event types and formats of packets and
commands are described below.

### How to run

Server parameters description:

```bash
./screen-worms-server [-p n] [-s n] [-t n] [-v n] [-w n] [-h n]
```

* `-p n` – port's number (`2021` by default)
* `-s n` – RNG seed (described below, by default uses value given by
`time(nullptr)`
* `-t n` – an integer determining the turning speed (`TURNING_SPEED` parameter,
`6` by default)
* `-v n` – an integer determining the game's speed (`ROUNDS_PER_SEC` parameter
in the protocol's description, `50` by default)
* `-w n` – board's width in pixels (`640` by default)
* `-h n` – board's height in pixels (`480` by default)

Client:

```bash
./screen-worms-client game_server [-n player_name] [-p n] [-i gui_server] [-r n]
```

* `game_server` – address (IPv4 or IPv6) or the game server's name
* `-n player_name` – player's name, per requirements described below
* `-p n` – game server's port (`2021` by default)
* `-i gui_server` – address (IPv4 or IPv6) or GUI server's name (`localhost`
by default)
* `-r n` – GUI server's port (`20210` by default)

To parse command line parameters [`getopt`](https://linux.die.net/man/3/getopt)
from standard library was used.

## Communication protocol between client and server

The protocol uses UDP. Datagrams contain binary data in the format described
below. If a number is sent, its bytes are sent in the network order.

### From client to server

A message from a client to the server has the following fields:

* `session_id`: 8 bytes, unsigned integer
* `turn_direction`: 1 byte, unsigned integer:
  * `0` → straight,
  * `1` → right,
  * `2` → left
* `next_expected_event_no`: 4 bytes, unsigned integer
* `player_name`: From `0` to `20` ASCII characters with values from `33` to
`126`, in particular, spaces are not allowed

Such datagram is sent every 30 ms. Communication is maintained always, even if
a game has not started yet or has already finished.

`turn_direction` field shows whether the player wants to turn (has either left
or right arrow buttons pressed on their keyboard).

Empty `player_name` field means that the client does not want to take part in
the game but would like to observe what's happening on the board.

`session_id` field is the same for all datagrams sent by a given client. Client
sets `session_id` at the launch to the current time in microseconds from
1970-01-01 00:00:00 +0000 (UTC).

### From server to client

A message from the server to a client has the following fields:

* `game_id`: 4 bytes, unsigned integer
* `events`: variable number of records, consistent with the specification below

The server sends a message like this immediately after receiving a message from
a client. Information about all events starting from the one numbered as
`next_expected_event_no` from the received packet, up until the last available
is sent. If there are no such events, the server does not respond. The server
sends such a message to all clients after a new event happens.

Maximum size of the data field of a UDP datagram sent by the server is 548
bytes. If the server needs to send more events than can be fit in one UDP
datagram, it sends them in multiple messages. In such a case, all datagrams
beside the last one must contain the maximum number of events that can be fit in
a single datagram.

`game_id` field is used to identify the game in a situation when datagrams from
a previous game are late to reach a client.

### Records describing game events

Records have the following format:

* `len`: 4 bytes, unsigned integer, the sum of lengths of `event_*` fields
* `event_no`: 4 bytes, unsigned integer, for each game consecutive values
starting from zero
* `event_type`: 1 byte
* `event_data`: depends on the type, described below
* `crc32`: 4 bytes, unsigned integer, control sum of fields from `len` to
`event_data` inclusive, CRC-32-IEEE is used

Possible event types:

* `NEW_GAME`
  * `event_type`: `0`
  * `event_data`:
    * `maxx`: 4 bytes, unsigned integer, board's width in pixels
    * `maxy`: 4 bytes, unsigned integer, board's height in pixels
    * a list where each element is a `player_name` as described in this
[section](#from-client-to-server) plus `\0` character
* `PIXEL`
  * `event_type`: `1`
  * `event_data`:
    * `player_number`: 1 byte
    * `x`: 4 bytes, unsigned integer, abscissa
    * `y`: 4 bytes, unsigned integer, ordinate
* `PLAYER_ELIMINATED`
  * `event_type`: `2`
  * `event_data`:
    * `player_number`: 1 byte
* `GAME_OVER`
  * `event_type`: `3`
  * `event_data`: no data

List of player names in the `NEW_GAME` event does not include empty observer
names. The order of players in a `NEW_GAME` event and their enumeration in
`PIXEL` and `NEW_GAME` events is determined by sorting the names alphabetically.
The players are enumerated starting from zero.

### RNG

The following deterministic generator of pseudo random 32-bit integers is used:

```math
\displaylines{
r_0 = seed \\
r_i = (r_{i-1} \cdot 279410273) \mod 4294967291
}
```

where `seed` is a 32-bit value passed to the server as the `-s` parameter. By
default this is `time(nullptr)` truncated to 32-bits. The first call to the RNG
must return $r_0 = seed$.

### Game state

During a game, the server maintains the game's state, which includes at least:

* `game_id`, sent in every outbound datagram
* current coordinates of every worm (as floating point numbers of at least
double precision) and their directions
* all game events so far generated from the start (as described in this
[section](#records-describing-game-events))
* eaten pixels on the board

Left upper corner of the board has coordinates of `(0, 0)`, abscissa grows to
the right, ordinate grows downwards. Movement's direction is expressed in
degrees in clockwise direction with `0` being the right direction.

Note: a worm's position is calculated and stored using floating point numbers.
When converting them into pixel coordinates they are rounded down. We assume the
position is inside the board when abscissa is inside `[0, maxy - 1]` and
ordinate is inside `[0, maxx - 1]`.

### Connecting and disconnecting players

A new player can join in any moment. To join, it's sufficient that the server
receives a valid message from a new client. If the new player joins while a game
is in progress, they become an observer, receiving information on all events so
far in this game from the very start. The new player takes part in the next
game. If a player with an empty `player_name` field joins, they don't take part
in the games but they may observe them.

If a player does not communicate for at least 2 seconds, they are disconnected.
If there is a game in progress, their worm, if existing and alive, continues
moving as described [below](#game-loop).

Clients are identified by `(socket, session_id)` pairs, and receiving a message
from an existing client's socket but with a higher `session_id` is equivalent
with disconnecting that client and a new one joining. Messages with lower
`session_id`'s are ignored.

Packets received from an unknown socket but with an existing client's name are
ignored.

### Game start and management of connected clients

To start a game, all (more than one) connected players (with nonempty names)
need to press an arrow button (send a `turn_direction` value different from
zero).

The game state is initialized in the following way. Players are initialized in
the alphabetical order of their names.

```code
game_id = rand()
generate NEW_GAME event
for each player initialize the position and direction of their worm
  players_worms_x = (rand() mod maxx) + 0.5
  players_worms_y = (rand() mod maxy) + 0.5
  players_worms_direction = rand() mod 360
  if the spawn pixel is being eaten, then
    generate PLAYER_ELIMINATED event
  else
    generate PIXEL event
```

Note: this means a player can lose at the start.

### Game loop

A game consists of rounds. A round has a duration of `1/ROUNDS_PER_SEC` of a
second. Players' moves are determined in the alphabetical order of their names.

```code
for each player
  if the most recent turn_direction == 1, then
    players_worms_direction += TURNING_SPEED
  else if the most recent turn_direction == 2, then
    players_worms_direction -= TURNING_SPEED
  move the worm in the current direction by 1
  if that doesn't change the worm's pixel coordinates, then
    continue
  if the new pixel is eaten, being eaten or is outside the board, then
    generate PLAYER_ELIMINATED event
  else
    generate PIXEL event
```

### Game's end

If there's only one worm left on the board, the game is over. A `GAME_OVER`
event is generated. After the game is over, the server continues to maintain
communication with clients. When the server receives from every connected client
with a nonempty name at least one message with `turn_direction` different from
zero, the server starts the next game. Clients may receive late datagrams from
a previous game after a new one has already started, and it is their
responsibility to deal with that correctly.

## Communication protocol between client and GUI

The protocol uses TCP. Messages are sent as text, each in a new line. Numbers
use decimal format. Lines are ended with the 10-th ASCII character. If a line
contains a few values, the sender separates them with single spaces, without
attaching any white space at the start or the end of the line.

A recipient of an incorrect message ignorees it. Recipient does not confirm the
reception of any message with a reply message.

GUI server accepts the following messages:

* `NEW_GAME maxx maxy player_name1 player_name2 …`
* `PIXEL x y player_name`
* `PLAYER_ELIMINATED player_name`

A player's name is a sequence of 1-20 ASCII characters with values in the range
of 33-126. `x` and `y` coordinates are integers from `0` to `maxx - 1` and from
`0` to `maxy - 1` respectively. Left upper corner of the board has coordinates
`(0, 0)`, abscissa grows to the right, ordinate grows downwards.

GUI server sends the following messages:

* `LEFT_KEY_DOWN`
* `LEFT_KEY_UP`
* `RIGHT_KEY_DOWN`
* `RIGHT_KEY_UP`

## Additional specifications

Communication is possible over IPv4 as well as over IPv6. Large collections of
messages, events etc. are allocated dynamically.

While processing sequences of events in datagrams by a client:

* the first event with an incorrect control sum stops the processing of the next
events in this datagram, but does not invalidate the prevously processed ones
* a record with a correct control sum, of known type, but with nonsensical
values causes the client to exit with an appropriate message and an exit code of
`1`
* events with correct control sums but unknown types are skipped.

When a client encounters an error while trying to connect to a game server or to
GUI, it exits with an error code of `1` and an appropriate message to `stderr`.

A game server should be robust and resilient to errors which still allow it to
function. It should be possible to launch it without the need to restart in case
of communication errors, temporary network unavailability, network
reconfiguration etc.

A game server cannot handle more than 25 players simultaneously. Additional
players above the limit cannot disturb players who already joined.

In a game server, latency issues with some clients cannot decrease the quality
of communcation with other clients. Similarly for a client, latency issues in
communication with a GUI server cannot influence communication with a game
server. A
[note](https://stackoverflow.com/questions/4165174/when-does-a-udp-sendto-block)
on this.

Regular scheduled actions (such as rounds or messages between a game server and
a client) are done independently of time needed for data processing or
computer's or network's load.

Client-GUI connection has Nagle's algorithm turned off to reduce latency.

In case of incorrect command line arguments, programs print an appropriate
message to `stderr` and exit with an exit code of `1`.

All parameters are checked for nonsensical values (appropriate limits on board's
size, `ROUNDS_PER_SEC`, `TURNING_SPEED`, zero or negative values etc.).

`player_name` must not end with zero character.

Each new game starts event enumeration from zero.

Only the most recent message from GUI server is taken into account to determine
the turning direction.

A client is responsible only for a single player and always sends the same
`player_name`.

While processing networking binary data,
[types with fixed sizes](http://en.cppreference.com/w/c/types/integer) are used.

Other references used in the implementation:

* <http://man7.org/linux/man-pages/man2/gettimeofday.2.html>
* <https://stackoverflow.com/questions/809902/64-bit-ntohl-in-c>.
