import select
import unittest
from game_launcher import launch_server, ServerLaunchOptions
from game_logic import GameLogic, GameRandom
from protocol import *
import time
import socket
import itertools
import traceback
import random


# solution quirks; having a solution complying to the defaults is probably best
RUN_ROUND_ON_GAME_START = False

# debugging options, if your solution requires these it's probably incorrect
PRINT_RECEIVED_PACKETS = False
PRINT_SENT_PACKETS = False
FORCE_IPV6 = False
SERVER_PORT = random.randint(10000, 20000)


class ServerClient:
    def __init__(self, index, server_port):
        self.index = index
        self.session_id = int(time.time())
        self.player_name = f"player{index}"
        if FORCE_IPV6 or index != 1:
            self.socket = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
            self.socket.bind(("::1", 0))
            self.use_ipv4 = False
        else:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.socket.bind(("127.0.0.1", 0))
            self.use_ipv4 = True
        self.socket.setsockopt(socket.SOL_SOCKET, 35, 1)  # SO_TIMESTAMPNS
        self.socket.setblocking(False)
        self.server_port = server_port
        self.next_expected_event_no = 0
        self.direction = TurnDirection.STRAIGHT
        self.sent_direction = None
        self.game_id = None
        self.pending_events = []
        self.send_status_hook = None

    def send(self):
        msg = C2SMsg(self.session_id, self.direction, self.next_expected_event_no, self.player_name)
        data = msg.encode()
        if self.send_status_hook is not None:
            data = self.send_status_hook(data)
        if len(data) == 0:
            return
        if PRINT_SENT_PACKETS:
            if data == msg.encode():
                print("Send", self.index, msg)
            else:
                print("Send", self.index, data)
        self.socket.sendto(data, ("127.0.0.1" if self.use_ipv4 else "::1", self.server_port))
        self.sent_direction = self.direction

    def handle(self, data):
        pk = S2CMsg()
        log = PacketStructureLog()
        try:
            pk.decode(data, log)
        except Exception as e:
            print("WARN: Got invalid packet: " + data.hex())
            traceback.print_exc()
            print()
            print("Structure:")
            print(log.buf)
            return False, []
        if PRINT_RECEIVED_PACKETS:
            print("Receive", self.index, pk)
        if len(pk.events) == 0:
            print("WARN: got packet with zero events!")
            return False, []
        new_game = False
        if pk.game_id != self.game_id:
            if pk.events[0].event_no != 0:
                print("WARN: Got unexpected packet from another game")
                return False, []
            new_game = True
            self.game_id = pk.game_id
            self.next_expected_event_no = 0
        ret = []
        for e in pk.events:
            if not e.crc_valid:
                print(f"WARN: Got a packet with invalid CRC, your solution probably uses incorrect CRC computation code (server CRC: {hex(e.crc)}, client CRC: {hex(e.expected_crc)})")
                break
            if e.event_no != self.next_expected_event_no:
                print("NOTE: Got packet with out of order event no, might be caused by resending, this might happen normally during testing in some cases")
                continue
            ret.append(e)
            self.next_expected_event_no += 1
        return new_game, ret


class ServerSimulation:
    def __init__(self, random: GameRandom, opts: ServerLaunchOptions, clients: List[ServerClient]):
        self.logic = GameLogic(random, opts.maxx, opts.maxy, opts.turning_speed)
        self.round_time = 1/opts.rounds_per_sec
        self.last_round_time = None
        self.start_time = None
        self.total_rounds = 0
        self.clients = clients

    def run_rounds(self):
        extra_rounds = 0
        if self.last_round_time is None:
            self.start_time = time.time() - 0.001
            self.last_round_time = time.time() - 0.001
            self.logic.begin([c.player_name for c in self.clients])
            extra_rounds = 1 if RUN_ROUND_ON_GAME_START else 0
        rounds = int((time.time() - self.last_round_time) / self.round_time)
        for r in range(rounds + extra_rounds):
            self.logic.round({c.player_name: c.sent_direction if c.sent_direction is not None else c.direction for c in self.clients})
            self.total_rounds += 1
        self.last_round_time += self.round_time * rounds

    def get_events(self):
        self.run_rounds()
        return self.logic.events

    def is_dangerously_close_to_round_exec(self, offset=0):
        if self.last_round_time is None:
            return False
        td = (time.time() + offset - self.last_round_time) % self.round_time
        return td < 0.002 or td > self.round_time - 0.002


class ServerTest:
    def __init__(self, testcase, opts=ServerLaunchOptions(), clients=2, seed=None):
        if seed is not None:
            opts.seed = seed
        if SERVER_PORT is not None:
            opts.port = SERVER_PORT
        self.instance = launch_server(opts)
        opts = self.instance.opts
        self.clients = []
        for ndx in range(clients):
            self.clients.append(ServerClient(ndx, opts.port))
        self.last_send_time = None
        testcase.addCleanup(self.finish)

        if opts.seed is not None:
            self.random = GameRandom(opts.seed)
            self.simulation = ServerSimulation(self.random, opts, self.clients)
        else:
            self.simulation = None

    def finish(self):
        self.instance.kill()
        for c in self.clients:
            c.socket.close()
        self.clients = []

    def set_dir(self, client, direction):
        self.clients[client].direction = direction

    def get_send_status_sleep_time(self):
        wait_time = 0
        if self.last_send_time is not None:
            wait_time = self.last_send_time + 0.03 - time.time()
            wait_time = max(wait_time, 0)
        while self.simulation is not None and self.simulation.is_dangerously_close_to_round_exec(wait_time):
            wait_time += 0.001
        return wait_time

    def wait_for_status_send(self):
        if self.last_send_time is not None:
            wait_time = self.get_send_status_sleep_time()
            if wait_time > 0:
                time.sleep(wait_time)
            self.last_send_time += 0.03
            if self.last_send_time < -0.05:
                self.last_send_time = time.time()
        else:
            self.last_send_time = time.time()

    def send_status(self):
        first_status = (self.last_send_time is None)
        self.wait_for_status_send()
        if self.simulation is not None and not first_status:
            self.simulation.run_rounds()
        for client in self.clients:
            client.send()

    def read_events(self, clients=None, timeout=0.05, return_packets=False):
        clients = [self.clients[c] for c in clients] if clients is not None else self.clients
        timeout_abs = time.time() + timeout

        retval = []
        packets = []
        for client in clients:
            retval += [(client.index, event) for event in client.pending_events]
        if len(retval) > 0:
            timeout_abs = min(time.time() + 0.001, timeout_abs)  # give the server an extra 1ms to send all events
        while True:
            diff = timeout_abs - time.time()
            if diff < 0:
                break
            ready_read, _, _ = select.select([client.socket for client in clients], [], [], min(self.get_send_status_sleep_time(), diff))
            clients_by_socket = {client.socket: client for client in clients}
            for x in ready_read:
                client = clients_by_socket[x]
                data, ancdata, _, _ = client.socket.recvmsg(1024, 1024)
                ts = struct.unpack("LL", ancdata[0][2])
                ts = ts[0] + ts[1] / 1e9
                packets.append((client.index, ts, data))
                new_game, events = client.handle(data)
                if new_game and len(retval) > 0:
                    client.pending_events += events
                    break
                retval += [(client.index, event) for event in events]
                timeout_abs = min(time.time() + 0.001, timeout_abs)  # give the server an extra 1ms to send all events
            if self.get_send_status_sleep_time() == 0:
                self.send_status()
        if return_packets:
            return packets, retval
        return retval


class ServerProtocolValidityTests(unittest.TestCase):
    def test_two_same_usernames(self):
        test = ServerTest(self)
        test.clients[0].player_name = "a"
        test.clients[1].player_name = "a"
        test.set_dir(0, TurnDirection.LEFT)
        test.set_dir(1, TurnDirection.LEFT)
        test.send_status()
        self.assertTrue(len(test.read_events(clients=[0])) == 0, "game should have not began")

    def _test_check_username(self, username, should_be_okay, send_status_hook=None):
        test = ServerTest(self)
        test.clients[0].player_name = username
        test.set_dir(0, TurnDirection.LEFT)
        test.set_dir(1, TurnDirection.LEFT)
        if send_status_hook is not None:
            test.clients[0].send_status_hook = send_status_hook
        test.send_status()
        if should_be_okay:
            self.assertTrue(len(test.read_events(clients=[0])) > 0, "game should have began")
        else:
            self.assertTrue(len(test.read_events(clients=[0])) == 0, "game should have not began")
        test.finish()

    def test_valid_usernames(self):
        self._test_check_username("a", True)
        self._test_check_username("a" * 20, True)
        for i in range(33, 126+1):
            self._test_check_username("aaa" + chr(i) + "aaa", True)

    def test_invalid_usernames(self):
        self._test_check_username("", False)
        self._test_check_username("a" * 21, False)
        for i in range(0, 256):
            if i < 33 or i > 126:
                self._test_check_username("aaa" + chr(i) + "aaa", False)

    def test_cut_packet(self):
        for i in range(0, 8+1+4+1):
            self._test_check_username("a", False, lambda data: data[:i])

    def test_one_packet_is_enough_to_start(self):
        test = ServerTest(self)
        test.set_dir(0, TurnDirection.LEFT)
        test.set_dir(1, TurnDirection.STRAIGHT)
        test.send_status()
        test.set_dir(0, TurnDirection.STRAIGHT)
        test.send_status()
        test.set_dir(1, TurnDirection.RIGHT)
        test.send_status()
        self.assertTrue(len(test.read_events(clients=[0])) > 0, "game should have began")

    def test_25_clients(self):
        test = ServerTest(self, clients=25)
        for i in range(25):
            test.clients[i].player_name = test.clients[i].player_name.rjust(20, 'a')
        test.send_status()
        for i in range(25):
            test.set_dir(i, TurnDirection.LEFT)
        test.send_status()
        events = test.read_events(clients=[0])
        self.assertTrue(len(events) > 0, "game should have began")
        expected_event = NewGameEvent(maxx=640, maxy=480, player_names=[c.player_name for c in test.clients])
        expected_event.event_no = 0
        self.assertEqual(str(events[0][1]), str(expected_event))

    def test_26_clients(self):
        # Serwer nie musi obsługiwać więcej niż 25 podłączonych graczy jednocześnie. Dodatkowi gracze ponad limit nie mogą jednak przeszkadzać wcześniej podłączonym.
        # I do not think that another implementation is acceptable here. The 26th player would break the client packet
        test = ServerTest(self, clients=26)
        for i in range(26):
            test.clients[i].player_name = test.clients[i].player_name.rjust(20, 'a')
        test.clients[24].player_name = test.clients[24].player_name[1:]  # make it easier to get easier to get an edge case
        test.clients[25].player_name = 'a'  # this should end up in a start game packet too big by 1 byte in an incorrect impl
        test.send_status()
        for i in range(26):
            test.set_dir(i, TurnDirection.LEFT)
        test.send_status()
        events = test.read_events(clients=[0])
        self.assertTrue(len(events) > 0, "game should have began")
        expected_event = NewGameEvent(maxx=640, maxy=480, player_names=[c.player_name for c in test.clients[:25]])
        expected_event.event_no = 0
        self.assertEqual(str(events[0][1]), str(expected_event))

    def test_timeout_handling_pre_game(self):
        test = ServerTest(self, clients=3)
        test.clients[2].player_name = test.clients[0].player_name
        test.set_dir(0, TurnDirection.LEFT)
        test.clients[2].send_status_hook = lambda x: b''
        test.send_status()
        test.clients[0].send_status_hook = lambda x: b''
        t = time.time() + 2.01
        while time.time() < t:
            test.send_status()

        # test timeouts
        test.set_dir(1, TurnDirection.LEFT)
        test.send_status()
        self.assertTrue(len(test.read_events(clients=[0])) == 0, "game should have not began")
        self.assertTrue(len(test.read_events(clients=[1])) == 0, "game should have not began")
        self.assertTrue(len(test.read_events(clients=[2])) == 0, "game should have not began")
        test.clients[2].send_status_hook = None
        test.set_dir(2, TurnDirection.LEFT)
        test.send_status()
        self.assertTrue(len(test.read_events(clients=[1])) > 0, "game should have began")
        self.assertTrue(len(test.read_events(clients=[2])) > 0, "game should have began")
        self.assertTrue(len(test.read_events(clients=[0])) == 0, "client 0 should have not got the game start info")

        # try to join back with client 0 as a spectator, should fail since our name is same as client[2]'s
        test.clients[0].send_status_hook = None
        test.send_status()
        self.assertTrue(len(test.read_events(clients=[0])) == 0, "client 0 shouldn't be allowed back due to bad name")

        # change client 0's name and try to rejoin
        test.clients[0].player_name = 'whatever'
        self.assertTrue(len(test.read_events(clients=[0])) > 0, "client 0 should be allowed back as a spectator")


class ServerLogicTests(unittest.TestCase):
    def assertEventsSame(self, l, r):
        for a, b in zip(l, r):
            self.assertEqual(repr(a), repr(b))
        self.assertEqual(len(l), len(r))

    def _validate_gameplay_with_simulation(self, test, hook=None):
        history = []
        try:
            simulated_event_i = 0
            while True:
                if hook is not None:
                    hook()
                client_packets, client_events = test.read_events(clients=[0], return_packets=True)
                client_packets = [(ts, packet) for _, ts, packet in client_packets]
                client_events = [event for _, event in client_events]
                t = time.time()
                total_rounds = test.simulation.total_rounds
                simulated_events = test.simulation.get_events()
                history.append((t, total_rounds, client_packets, client_events, simulated_events[simulated_event_i:]))
                self.assertEventsSame(client_events, simulated_events[simulated_event_i:])
                simulated_event_i = len(simulated_events)
                if isinstance(client_events[-1], GameOverEvent):
                    break
        except:
            print("Game log")
            for b_time, total_rounds, packets, client, simulated in history:
                print("[[ Batch " + str(b_time - test.simulation.start_time) + " " + str(total_rounds) + " ]]")
                for ts, data in packets:
                    pk = S2CMsg()
                    try:
                        pk.decode(data)
                        print("Packet:", ts, pk)
                    except:
                        print("Packet:", ts, data)
                for (c, s) in itertools.zip_longest(client, simulated):
                    print(f"C+ {c}")
                    print(f"PY {s}")
            raise

    def test_basic_gameplay(self):
        test = ServerTest(self, seed=1337)
        test.set_dir(0, TurnDirection.LEFT)
        test.set_dir(1, TurnDirection.LEFT)
        test.send_status()
        self._validate_gameplay_with_simulation(test)

    def test_basic_gameplay_more(self):
        for i in range(10):
            test = ServerTest(self, seed=2000+i)
            test.set_dir(0, TurnDirection.LEFT)
            test.set_dir(1, TurnDirection.LEFT)
            test.send_status()
            self._validate_gameplay_with_simulation(test)
            test.finish()

    def test_basic_gameplay_direction_changes(self):
        test = ServerTest(self, seed=1337)
        test.set_dir(0, TurnDirection.LEFT)
        test.set_dir(1, TurnDirection.LEFT)
        test.send_status()

        def hook():
            test.set_dir(0, random.choice(list(TurnDirection)))
            test.set_dir(1, random.choice(list(TurnDirection)))
        self._validate_gameplay_with_simulation(test, hook)

    def test_disconnected_clients_dont_affect_gameplay(self):
        opts = ServerLaunchOptions()
        opts.maxx = 1920
        opts.maxy = 1080
        test = ServerTest(self, opts=opts, seed=1337)
        test.set_dir(0, TurnDirection.LEFT)
        test.set_dir(1, TurnDirection.LEFT)
        test.send_status()
        test.set_dir(0, TurnDirection.STRAIGHT)
        test.set_dir(1, TurnDirection.STRAIGHT)

        s = time.time()
        def hook():
            o = time.time() - s
            if 0.5 <= o < 2.6:
                test.clients[1].send_status_hook = lambda x: b''
            if 2.6 <= o < 3:
                c = test.clients[1]
                c.send_status_hook = lambda x: C2SMsg(c.session_id, random.choice(list(TurnDirection)), c.next_expected_event_no, c.player_name).encode()
                c.send_status_hook = None
            if o >= 4:
                test.set_dir(0, TurnDirection.LEFT)
        self._validate_gameplay_with_simulation(test, hook)



if __name__ == '__main__':
    unittest.main()
