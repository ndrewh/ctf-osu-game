from pysat.solvers import Solver
from pysat.card import *
from pysat.formula import IDPool

import struct

with open("../dist-osu/game.beatmap", "rb") as f:
    data = f.read()

note_count, constraint_count = struct.unpack("<QQ", data[:16])
notes = data[16:16+24*note_count]
constraints = data[16+24*note_count:]

# Read the constraints in

constraints_new = []
for x in range(0, len(constraints), 64):
    constraint = constraints[x:x+64]
    decoded = list(struct.unpack("<4Q32s", constraint))
    circle_list, code = decoded[:4], decoded[4]
    constraints_new.append((circle_list, code))

notes_new = []
for x in range(0, len(notes), 24):
    node = notes[x:x+24]
    x, y, tick_due, color = struct.unpack("<ffQ3s", node[:19])
    notes_new.append(tick_due)

# There are 30 possible ticks that each note could be played on
# (see Element::kill)
SCORE_THRESHOLD = 30

pool = IDPool()
def create_base_constraints(notes):
    options_for_note = []
    constraints = []
    max_tick = max([n for n in notes])

    # Here we create a variable for each of the 30 ticks a note could be scored on
    for tick_due in notes:
        options = {i: pool.id() for i in range(tick_due-SCORE_THRESHOLD, tick_due-1)}

        # A note can only be clicked once!
        constraints += CardEnc.atmost(list(options.values()), 1, vpool=pool)
        options_for_note.append(options)

    # Now, for every tick, check at most one
    is_tick_free = []
    for i in range(max_tick):
        vs = []
        this_tick_free = pool.id()
        is_tick_free.append(this_tick_free)
        for o in options_for_note:
            if i in o:
                vs.append(o[i])

        # this_tick_free will be forced false if any are used on this tick
        constraints += CardEnc.atmost(vs + [this_tick_free], 1, vpool=pool)

    # Now we enforce that we don't select something two frames in a row
    # (this is a limitation of the handle_click function)
    for i in range(1, max_tick):
        prev = is_tick_free[i-1]
        cur = is_tick_free[i]
        constraints += [[prev, cur]] # at least one of them must be free

    return constraints, options_for_note

sat_constraints, options_for_note = create_base_constraints(notes_new)

def get_combined_var_list(circle_list):
    combined_vars = []
    for c in circle_list[:3]:
        combined_vars.extend(options_for_note[c].values())
    return combined_vars

# Create SAT constraints to represent a < b where both a and b are actually scored
def add_sat_lt_included(a, b):
    a_options = options_for_note[a]
    b_options = options_for_note[b]

    cons = []
    for o1, v in a_options.items():
        # Suppose a is at o1
        # then b must be > o1
        gt_options = [v2 for x, v2 in b_options.items() if x > o1]
        cons.append([-v] + gt_options)

    for o1, v in b_options.items():
        # Suppose b is at o1
        # then a must be < o1
        gt_options = [v2 for x, v2 in a_options.items() if x < o1]
        cons.append([-v] + gt_options)
    return cons

# Create SAT constraints to represent b => (a AND a < b)
def add_sat_lt_half_implication(a, b):
    # includes b => a but not a => b
    a_options = options_for_note[a]
    b_options = options_for_note[b]

    cons = []
    for o1, v in a_options.items():
        # Suppose a is at o1
        # then b cannot be < o1
        lte_options = [v2 for x, v2 in b_options.items() if x <= o1]
        cons += [[-v, -x] for x in lte_options]

    for o1, v in b_options.items():
        # Suppose b is at o1
        # then a must be < o1
        gt_options = [v2 for x, v2 in a_options.items() if x < o1]
        cons.append([-v] + gt_options)
    return cons

count = 0
for circle_list, code in constraints_new:
    if count % 1000 == 0:
        print(count)
    if b'\x87\x86\xaf\xa6\xd7\xc6\xa0\xe6\xd8\x86\xf6\xf8\xe6\x9f\xf6\xc4~n\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80' in code:
        # At most 2 NOT selected
        # At most 2 NOT selected <=> At least 1 selected
        combined_vars = get_combined_var_list(circle_list)
        sat_constraints += CardEnc.atleast(combined_vars, 1, vpool=pool)
    elif b'\x87\x86\xaf\xa6\xd7\xc6\xa0\xe6\xd8\x86\xf6\x9f\xf6\xc4~n\x80' in code:
        # At most 1 NOT selected
        # At most 1 NOT selected <=> At least 2 selected
        combined_vars = get_combined_var_list(circle_list)
        sat_constraints += CardEnc.atleast(combined_vars, 2, vpool=pool)
    elif b'\x87\x86\xaf\xa6\xd7\xc6\xa0\xe6\xd8\x86\xf6\xe7\xf6\xc4~n\x80' in code:
        # At least 1 NOT selected
        # At least 1 NOT selected <=> At most 2 selected
        combined_vars = get_combined_var_list(circle_list)
        sat_constraints += CardEnc.atmost(combined_vars, 2, vpool=pool)
    elif b'\x87\x86\xaf\xa6\xd7\xc6\xa0\xe6\xd8\x86\xf6\xf8\xe6\xe7\xf6\xc4~n\x80' in code:
        # At least 2 NOT selected
        # At least 2 NOT selected <=> At most 1 selected
        combined_vars = get_combined_var_list(circle_list)
        sat_constraints += CardEnc.atmost(combined_vars, 1, vpool=pool)
    elif b'\xf6\x87\xed\xaf\xdd\xd7\xcd\x81\xc4~\xa7\xdc\xcf\xcc\x81\xc4~n\x80' in code:
        # a < b < c, all are selected
        for c in circle_list:
            sat_constraints += [list(options_for_note[c].values())] # at least one of them!
        sat_constraints += add_sat_lt_included(circle_list[0], circle_list[1])
        sat_constraints += add_sat_lt_included(circle_list[1], circle_list[2])
    elif b'n\xff\xe5\xf6\xaf\xd5\x87\xc4~\xd7\xd5\xaf\xc4~n\x80' in code:
        # c => (b ^ (b < c))
        # b => (a ^ (a < b))
        sat_constraints += add_sat_lt_half_implication(circle_list[0], circle_list[1])
        sat_constraints += add_sat_lt_half_implication(circle_list[1], circle_list[2])
    else:
        raise RuntimeError("bad code")
    count += 1

# We have a SAT problem, solve it
solver = Solver('cadical', bootstrap_with=sat_constraints)
assert solver.solve()
soln = solver.get_model()

# A replay describes what note to play on each tick (-1 = play nothing)
replay = [-1 for _ in range(max([n for n in notes_new]))]
for i, n in enumerate(notes_new):
    options = options_for_note[i]
    for o, var in options.items():
        if soln[var-1] > 0: # if true in the SAT assignment
            assert replay[o] == -1
            replay[o] = i

# The flag is decoded from the low bit of the note ids played
bits = []
for r in replay:
    if r != -1:
        bits.append(r & 1)

answer = []
for i in range(0, len(bits), 8):
    num = 0
    for z in range(8):
        num |= (bits[i+z] << z)
    answer.append(num)
flag = bytes(answer)
print(flag)

replay_bytes = struct.pack("<Q", len(replay))
for r in replay:
    replay_bytes += struct.pack("<q", r)

with open("replay", "wb") as f:
    f.write(replay_bytes)
