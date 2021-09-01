import random
import unittest
from game_launcher import launch_client, ClientLaunchOptions
from protocol import *
import socket
import time


class ClientTest:
    def __init__(self, testcase, player_name="test", use_ipv4=False):
        self.udp_socket = socket.socket((socket.AF_INET if use_ipv4 else socket.AF_INET6), socket.SOCK_DGRAM)
        self.udp_socket.bind(('127.0.0.1', 0) if use_ipv4 else ('::1', 0))
        self.udp_socket.settimeout(1)
        self.tcp_server = socket.socket(socket.AF_INET if use_ipv4 else socket.AF_INET6, socket.SOCK_STREAM)
        self.tcp_server.bind(('127.0.0.1', 0) if use_ipv4 else ('::1', 0))
        self.tcp_server.listen(1)
        opts = ClientLaunchOptions()
        opts.player_name = player_name
        opts.game_server = '127.0.0.1' if use_ipv4 else '::1'
        opts.game_server_port = self.udp_socket.getsockname()[1]
        opts.ui_server = '127.0.0.1' if use_ipv4 else '::1'
        opts.ui_server_port = self.tcp_server.getsockname()[1]
        self.instance = launch_client(opts)
        self.tcp_server.settimeout(1)
        try:
            self.tcp_socket, _ = self.tcp_server.accept()
        except socket.timeout:
            self.udp_socket.close()
            self.tcp_server.close()
            self.instance.kill()
            raise Exception('client did not connect')
        self.tcp_socket.settimeout(1)
        self.game_id = 1337
        self.old_game_id = None
        self.old_game_event_no = None
        self.player_name = player_name.encode()
        self.game_player_names = []
        self.instance_addr = None
        self.send_event_no = 0
        self.session_id = None
        self.expected_tcp = b''
        testcase.addCleanup(self.finish)
        self.read_udp()

    def reset_game(self):
        self.old_game_id = self.game_id
        self.old_game_event_no = self.send_event_no
        self.game_id += 1
        self.send_event_no = 0

    def read_udp(self):
        (data, addr) = self.udp_socket.recvfrom(1024)
        if self.instance_addr is None:
            self.instance_addr = addr
        if addr != self.instance_addr:
            raise Exception('unexpected udp packet (wrong source)')
        pk = C2SMsg(None, None, None, None)
        pk.decode(data)
        if pk.player_name != self.player_name:
            raise Exception('unexpected player name: ' + str(pk) + ' (expected: ' + str(self.player_name) + ')')
        if pk.next_expected_event_no != self.send_event_no:
            raise Exception('unexpected next_expected_event_no: ' + str(pk) + ' (expected: ' + str(self.send_event_no) + ')')
        if self.session_id is None:
            self.session_id = pk.session_id
        if self.session_id != pk.session_id:
            raise Exception('unexpected session_id: ' + str(pk) + ' (expected: ' + str(self.session_id) + ')')
        return pk

    def send_udp(self, events, old_game=False):
        for e in events:
            if e.event_no is not None:
                continue
            if old_game:
                e.event_no = self.old_game_event_no
                self.old_game_event_no += 1
                continue
            e.event_no = self.send_event_no
            self.send_event_no += 1
            if isinstance(e, NewGameEvent) and e.maxx >= 0 and e.maxy >= 0:
                if e.crc is None:
                    self.game_player_names = e.player_names
                self.expected_tcp += f"NEW_GAME {e.maxx} {e.maxy} {' '.join(e.player_names)}\n".encode()
            if isinstance(e, PixelEvent) and e.player_number < len(self.game_player_names):
                self.expected_tcp += f"PIXEL {e.x} {e.y} {self.game_player_names[e.player_number]}\n".encode()
            if isinstance(e, PlayerEliminatedEvent) and e.player_number < len(self.game_player_names):
                self.expected_tcp += f"PLAYER_ELIMINATED {self.game_player_names[e.player_number]}\n".encode()
        data = S2CMsg(self.old_game_id if old_game else self.game_id, events).encode()
        self.udp_socket.sendto(data, self.instance_addr)

    def read_and_check_tcp(self):
        self.tcp_socket.settimeout(1 if len(self.expected_tcp) > 0 else 0.1)
        try:
            rx = self.tcp_socket.recv(1024*16)
        except socket.timeout:
            rx = b''
        if rx != self.expected_tcp:
            print("== Unexpected TCP data ==")
            print("Expected:", self.expected_tcp)
            print("Received:", rx)
            print("")
            raise Exception('unexpected tcp data')
        self.expected_tcp = b''

    def send_tcp(self, cmd):
        self.tcp_socket.send(cmd.encode() + b'\n')

    def finish(self):
        self.udp_socket.close()
        self.tcp_socket.close()
        self.tcp_server.close()
        self.instance.kill()

    def expect_error_exit(self):
        self.instance.expect_error_exit()


class ClientTests(unittest.TestCase):
    def test_invalid_server_port(self):
        opts = ClientLaunchOptions()
        opts.player_name = 'player'
        opts.game_server = '127.0.0.1'
        opts.game_server_port = random.randint(10000, 20000)
        opts.ui_server = '127.0.0.1'
        opts.ui_server_port = random.randint(10000, 20000)
        instance = launch_client(opts)
        instance.expect_error_exit()

    def test_invalid_server_addr(self):
        tcp_server = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
        tcp_server.bind(('::1', 0))
        tcp_server.listen(1)
        try:
            opts = ClientLaunchOptions()
            opts.player_name = 'player'
            opts.game_server = 'omae'
            opts.game_server_port = random.randint(10000, 20000)
            opts.ui_server = '127.0.0.1'
            opts.ui_server_port = random.randint(10000, 20000)
            instance = launch_client(opts)
            instance.expect_error_exit()
        finally:
            tcp_server.close()

    def test_basic_decode(self):
        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a", "b", "c"])])
        test.read_udp()
        test.read_and_check_tcp()
        test.send_udp([PixelEvent(0, 10, 10), PixelEvent(1, 20, 20), PixelEvent(2, 30, 30)])
        test.read_udp()
        test.read_and_check_tcp()
        test.send_udp([PlayerEliminatedEvent(0)])
        test.read_udp()
        test.read_and_check_tcp()

    def test_basic_decode_ipv4(self):
        test = ClientTest(self, use_ipv4=True)
        test.send_udp([NewGameEvent(48, 48, ["a", "b", "c"])])
        test.read_udp()
        test.read_and_check_tcp()
        test.send_udp([PixelEvent(0, 10, 10), PixelEvent(1, 20, 20), PixelEvent(2, 30, 30)])
        test.read_udp()
        test.read_and_check_tcp()
        test.send_udp([PlayerEliminatedEvent(0)])
        test.read_udp()
        test.read_and_check_tcp()

    def test_simple_input(self):
        test = ClientTest(self)
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.STRAIGHT)
        test.send_tcp("LEFT_KEY_DOWN")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.LEFT)
        test.send_tcp("LEFT_KEY_UP")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.STRAIGHT)
        test.send_tcp("RIGHT_KEY_DOWN")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.RIGHT)
        test.send_tcp("RIGHT_KEY_UP")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.STRAIGHT)

    def test_input_two_keys_simple(self):
        test = ClientTest(self)
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.STRAIGHT)
        test.send_tcp("LEFT_KEY_DOWN")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.LEFT)
        test.send_tcp("RIGHT_KEY_DOWN")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.RIGHT)
        test.send_tcp("LEFT_KEY_UP")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.RIGHT)

    def test_input_two_keys_moodle(self):  # https://moodle.mimuw.edu.pl/mod/forum/discuss.php?d=5701
        test = ClientTest(self)
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.STRAIGHT)
        test.send_tcp("LEFT_KEY_DOWN")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.LEFT)
        test.send_tcp("RIGHT_KEY_DOWN")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.RIGHT)
        test.send_tcp("RIGHT_KEY_UP")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.STRAIGHT)

    def test_invalid_player_id(self):
        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a", "b", "c"])])
        test.read_udp()
        test.read_and_check_tcp()
        test.send_udp([PixelEvent(3, 10, 10)])
        test.expect_error_exit()

        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a", "b", "c"])])
        test.read_udp()
        test.read_and_check_tcp()
        test.send_udp([PlayerEliminatedEvent(3)])
        test.expect_error_exit()

    def test_sudden_new_game(self):
        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a", "b"])])
        test.read_udp()
        test.read_and_check_tcp()
        test.reset_game()
        test.send_udp([NewGameEvent(48, 48, ["x", "y", "z"])])
        test.read_udp()
        test.read_and_check_tcp()
        test.send_udp([PlayerEliminatedEvent(1)])
        test.read_udp()
        test.read_and_check_tcp()

    def test_player_out_of_bounds(self):
        for (x, y) in [(0, 48), (48, 0), (48, 48), (0xffffffff, 0xffffffff)]:
            test = ClientTest(self)
            test.send_udp([NewGameEvent(48, 48, ["a", "b", "c"])])
            test.read_udp()
            test.read_and_check_tcp()
            test.send_udp([PixelEvent(1, x, y)])
            test.expect_error_exit()

    def test_dead_ui(self):
        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a", "b", "c"])])
        test.read_udp()
        test.read_and_check_tcp()
        test.tcp_socket.shutdown(socket.SHUT_RDWR)
        test.tcp_socket.close()
        test.expect_error_exit()

    def test_simple_invalid_input(self):
        test = ClientTest(self)
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.STRAIGHT)
        test.send_tcp("LEFT_KEY_DOWN")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.LEFT)
        test.send_tcp("LEFT_KEY_UPx")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.LEFT)
        test.send_tcp("xRIGHT_KEY_DOWN")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.LEFT)
        test.send_tcp("RIGHT_KEY_DOWN")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.RIGHT)

    def test_invalid_input_long_prefix(self):
        test = ClientTest(self)
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.STRAIGHT)
        test.send_tcp("LEFT_KEY_DOWN")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.LEFT)
        s = time.time()
        for i in range(1, 64*1024):
            test.send_tcp('a' * i + "LEFT_KEY_UP")
        e = time.time()
        for i in range(int((e-s)/0.03*1.5)):
            self.assertEqual(test.read_udp().turn_direction, TurnDirection.LEFT)
        test.send_tcp("RIGHT_KEY_DOWN")
        self.assertEqual(test.read_udp().turn_direction, TurnDirection.RIGHT)

    def test_non_ordered_player_names(self):
        for t in [["c", "a", "b"], ["a", "c", "b"], ["a", "a"]]:
            test = ClientTest(self)
            test.send_udp([NewGameEvent(48, 48, t)])
            test.expect_error_exit()

    def test_empty_player_name(self):
        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a", ""])])
        test.expect_error_exit()

    def test_non_null_terminated_new_game(self):
        event = NewGameEvent(48, 48, ["a", "b"])
        test = ClientTest(self)
        test.send_udp([UnknownEvent(event.event_type, event.encode()[:-1])])
        test.expect_error_exit()

    def test_less_than_two_players(self):
        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, [""])])
        test.expect_error_exit()
        test.finish()

        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a"])])
        test.expect_error_exit()
        test.finish()

    def test_ignore_unknown_events(self):
        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a", "b", "c"]), UnknownEvent(4, b'test'), PixelEvent(0, 10, 10)])
        test.read_udp()
        test.read_and_check_tcp()

    def test_cut_packet(self):  # test packets that have the game id + at least a single byte of event
        event = NewGameEvent(48, 48, ["a", "b", "c"])
        event.event_no = 0
        data = S2CMsg(1337, [event]).encode()

        for i in range(5, len(data)):
            test = ClientTest(self)
            test.game_id = 1337
            test.udp_socket.sendto(data[:i], test.instance_addr)
            test.expect_error_exit()

    def test_cut_event_fields(self):  # test packets that have the game id + at least a single byte of event
        for i in range(0, 6):
            data = struct.pack(">IB", 0, 0)[:i]
            crc = zlib.crc32(struct.pack(">I", len(data)) + data) % (1 << 32)
            data += struct.pack(">I", crc)

            ev = UnknownEvent(0, b"")
            ev.encode_full = lambda: data

            test = ClientTest(self)
            test.send_udp([ev])
            test.expect_error_exit()

    def test_cut_new_game_event(self):  # test packets that have the game id + at least a single byte of event
        event = NewGameEvent(48, 48, ["a"])
        event.event_no = 0
        event_data = event.encode()

        for i in range(0, 8):
            test = ClientTest(self)
            test.send_udp([UnknownEvent(event.event_type, event_data[:i])])
            test.expect_error_exit()

    def _test_cut_ingame_event(self, event):  # test packets that have the game id + at least a single byte of event
        event.event_no = 1
        event_data = event.encode()

        for i in range(0, len(event_data)):
            test = ClientTest(self)
            test.send_udp([NewGameEvent(48, 48, ["a", "b"])])
            test.send_udp([UnknownEvent(event.event_type, event_data[:i])])
            test.expect_error_exit()

    def test_cut_pixel_event(self):
        self._test_cut_ingame_event(PixelEvent(0, 10, 10))

    def test_cut_player_eliminated_event(self):
        self._test_cut_ingame_event(PlayerEliminatedEvent(0))

    def test_cut_game_over_event(self):
        self._test_cut_ingame_event(GameOverEvent())

    def _test_cut_ingame_event_other_game(self, event):  # client should still validate events from other games
        event.event_no = 0
        event_data = event.encode()

        for i in range(0, len(event_data)):
            test = ClientTest(self)
            test.old_game_id = 1
            test.old_game_event_no = 1
            test.send_udp([NewGameEvent(48, 48, ["a", "b"])])
            test.send_udp([UnknownEvent(event.event_type, event_data[:i])], old_game=True)
            test.expect_error_exit()

    def test_cut_event_other_game(self):
        self._test_cut_ingame_event_other_game(PixelEvent(0, 10, 10))
        self._test_cut_ingame_event_other_game(PlayerEliminatedEvent(0))
        self._test_cut_ingame_event_other_game(GameOverEvent())

    def _test_extra_byte_ingame_event(self, event):
        event.event_no = 1
        event_data = event.encode() + b'\x13'

        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a", "b"])])
        test.send_udp([UnknownEvent(event.event_type, event_data)])
        test.expect_error_exit()

    def test_extra_byte_pixel_event(self):
        self._test_extra_byte_ingame_event(PixelEvent(0, 10, 10))

    def test_extra_byte_player_eliminated_event(self):
        self._test_extra_byte_ingame_event(PlayerEliminatedEvent(0))

    def test_extra_byte_game_over_event(self):
        self._test_extra_byte_ingame_event(GameOverEvent())

    def _test_extra_byte_ingame_event_other_game(self, event):
        event.event_no = 1
        event_data = event.encode() + b'\x13'

        test = ClientTest(self)
        test.old_game_id = 1
        test.old_game_event_no = 1
        test.send_udp([NewGameEvent(48, 48, ["a", "b"])])
        test.send_udp([UnknownEvent(event.event_type, event_data)], old_game=True)
        test.expect_error_exit()

    def test_extra_byte_event_other_game(self):
        self._test_extra_byte_ingame_event_other_game(PixelEvent(0, 10, 10))
        self._test_extra_byte_ingame_event_other_game(PlayerEliminatedEvent(0))
        self._test_extra_byte_ingame_event_other_game(GameOverEvent())

    def test_packets_from_old_game(self):
        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a", "b"])])
        test.read_udp()
        test.read_and_check_tcp()
        test.reset_game()
        test.send_udp([NewGameEvent(48, 48, ["x", "y"])])
        test.send_udp([PixelEvent(0, 10, 10)], old_game=True)
        test.read_udp()
        test.read_and_check_tcp()
        test.old_game_event_no = 1
        test.send_udp([PlayerEliminatedEvent(0)], old_game=True)
        test.read_udp()
        test.read_and_check_tcp()
        test.old_game_event_no = 1
        test.send_udp([GameOverEvent()], old_game=True)
        test.read_udp()
        test.read_and_check_tcp()

    def test_new_game_must_have_event_no_zero(self):
        test = ClientTest(self)
        event = NewGameEvent(48, 48, ["a", "b"])
        event.event_no = 1
        data = S2CMsg(test.game_id, [event]).encode()
        test.udp_socket.sendto(data, test.instance_addr)
        test.expect_error_exit()

    def test_client_ignores_resends(self):
        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a", "b", "c"])])
        test.read_udp()
        test.read_and_check_tcp()
        events = [PixelEvent(0, 10, 10), PixelEvent(1, 20, 20), PixelEvent(2, 30, 30)]
        test.send_udp(events)
        test.read_udp()
        test.read_and_check_tcp()

        # resend the events
        test.send_udp(events)
        test.read_udp()
        test.read_and_check_tcp()

    def test_client_handles_new_events_in_resends(self):
        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a", "b", "c"])])
        test.read_udp()
        test.read_and_check_tcp()
        events = [PixelEvent(0, 10, 10), PixelEvent(1, 20, 20), PixelEvent(2, 30, 30)]
        test.send_udp(events)
        test.read_udp()
        test.read_and_check_tcp()

        # resend the events and add the new event
        events += [PixelEvent(0, 11, 11)]  # <- only difference from test_client_ignores_resends is this line
        test.send_udp(events)
        test.read_udp()
        test.read_and_check_tcp()

    def test_bad_crc(self):
        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a", "b", "c"])])
        test.read_udp()
        test.read_and_check_tcp()
        events = [PixelEvent(0, 10, 10), PixelEvent(1, 20, 20), PixelEvent(2, 30, 30)]
        events[1].crc = 1000
        events[1].event_no = 2
        events[2].event_no = 3
        test.send_udp(events)
        test.read_udp()
        test.read_and_check_tcp()

    def test_bad_len_field(self):
        for l in [1, 10, 100, 1000, 10000, 0x80000000, 0xffffffff]:
            data = struct.pack(">IB", l, 0)
            ev = UnknownEvent(0, b"")
            ev.encode_full = lambda: data

            test = ClientTest(self)
            test.send_udp([ev])
            test.expect_error_exit()

    def test_client_enforces_event_order(self):
        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a", "b", "c"])])
        test.read_udp()
        test.read_and_check_tcp()
        events = [PixelEvent(0, 10, 10), PixelEvent(1, 20, 20)]
        events[0].event_no = 2
        events[1].event_no = 1
        test.send_udp(events)
        test.expect_error_exit()

    def test_client_enforces_event_order_with_overflow(self):  # overflow check
        test = ClientTest(self)
        test.send_udp([NewGameEvent(48, 48, ["a", "b", "c"])])
        test.read_udp()
        test.read_and_check_tcp()
        events = [PixelEvent(0, 10, 10), PixelEvent(1, 20, 20)]
        events[0].event_no = 0xffffffff
        events[1].event_no = 0
        test.send_udp(events)
        test.expect_error_exit()


if __name__ == '__main__':
    unittest.main()
