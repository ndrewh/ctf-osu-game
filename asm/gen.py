
from mido import MidiFile, tick2second
from collections import defaultdict, namedtuple
import random
import itertools
import struct
from asm import generate_programs


class Color:
    RED = b"\xff\x00\x00"
    GREEN = b"\x00\xff\x00"
    BLUE = b"\x00\x00\xff"
    def random():
        return random.choice([Color.RED, Color.GREEN, Color.BLUE])
class Note:
    DELAY_TIME = 140
    STARTUP_DELAY = 160
    ENCODE_SIZE = 24

    """
    CircleElement(Position _p, Color _c, long _entry_time, char num): Element(_p, _c, _entry_time) {
    """
    def __init__(self, i, time, loc):
        self.i = i
        self.loc = loc
        self.color = Color.random()
        self.time_due = time

    def encode(self):
        return struct.pack("<ffQ3s", self.loc.x, self.loc.y, self.time_due, self.color).ljust(Note.ENCODE_SIZE, b"\x00")

class GameSave:
    def __init__(self, notes, constraints):
        self.notes = notes
        self.constraints = constraints
    def encode(self):
        header = struct.pack("<QQ", len(self.notes), len(self.constraints))
        data = b"".join((n.encode() for n in self.notes))
        data += b"".join((n.encode() for n in self.constraints))
        return header + data


Point = namedtuple('Point', ['x', 'y'])
class Layout:
    GRID_SIZE = 10
    def __init__(self):
        self.grid = [[-(Note.DELAY_TIME+2) for _ in range(self.GRID_SIZE)] for _ in range(self.GRID_SIZE)]
        self.last_loc = Point(0, 0)
        self.last_time = 0

    def num_free(self):
        free_spots = sum(1 if time - self.grid[x][y] > Note.DELAY_TIME+1 else 0 for (x, y) in itertools.product(range(self.GRID_SIZE), range(self.GRID_SIZE)))
        return free_spots

    def should_skip(self, time):
        free_spots = sum(1 if time - self.grid[x][y] > Note.DELAY_TIME+1 else 0 for (x, y) in itertools.product(range(self.GRID_SIZE), range(self.GRID_SIZE)))
        if free_spots < 20:
            if free_spots < random.randrange(1, 21):
                return True
        return False

    def next_loc(self, time):
        jumps = 1 + (time - self.last_time) // 10
        while True:
            valid_directions = []
            if self.last_loc.x > 0:
                valid_directions.append(Point(-1, 0))
            if self.last_loc.x < self.GRID_SIZE-1:
                valid_directions.append(Point(1, 0))
            if self.last_loc.y > 0:
                valid_directions.append(Point(0, -1))
            if self.last_loc.y < self.GRID_SIZE-1:
                valid_directions.append(Point(0, 1))

            try_direction = random.choice(valid_directions)
            self.last_loc = Point(self.last_loc.x + try_direction.x, self.last_loc.y + try_direction.y)
            jumps = max(0, jumps-1)
            if jumps == 0 and time - self.grid[self.last_loc.x][self.last_loc.y] > Note.DELAY_TIME+1:
                break
        self.grid[self.last_loc.x][self.last_loc.y] = time
        self.last_time = time
        return self.last_loc

    def grid_to_screen(point):
        # -0.9, -0.7, -0.5, -0.3, -0.1, 0.1, 0.3, 0.5, 0.7, 0.9
        return Point(point.x * 0.2 - 0.9, point.y * 0.2 - 0.9)





mid = MidiFile('../midi/new_song.mid', clip=True)
print(mid)
for track in mid.tracks:
    print(track)
piano = mid.tracks[1]

t = 0
notes = []
counts = defaultdict(int)
for tr in mid.tracks[0:1]:
    for msg in tr:
        t += msg.time
        if msg.type == 'note_on':
            notes.append(t)
            counts[t] += 1
# for n in sorted(counts.keys()):
#     print(f"{n}: {counts[n]}")
print(len(counts.keys()))


note_objs = []
laid = Layout()
skipped = 0
note_count = 0
for n in sorted(counts.keys()):
    c = counts[n]
    num_notes = min(c, 4)
    n_frame = int(tick2second(n, mid.ticks_per_beat, 500000) * 60.0) + Note.STARTUP_DELAY
    for _ in range(num_notes):
        if not laid.should_skip(n_frame):
            note_objs.append(Note(note_count, n_frame, Layout.grid_to_screen(laid.next_loc(n_frame))))
            note_count += 1
        else:
            skipped += 1

print(f"Skipped {skipped}")

replay, index_to_tick, constraints = generate_programs(note_objs, "buckeye{d0nt_t41k_t0_m3_0r_my_50000_thr34d_vm_3v3r_4g41n_btw_d1d_y0u_us3_SAT_0r_SMT}")

gs = GameSave(note_objs, constraints)
game_save = gs.encode()
print(len(game_save))
print("done")
with open("save", "wb") as f:
    f.write(game_save)

# Encode the replay
replay_bytes = struct.pack("<Q", len(replay))
for r in replay:
    replay_bytes += struct.pack("<q", r)

with open("solve.replay", "wb") as f:
    f.write(replay_bytes)


