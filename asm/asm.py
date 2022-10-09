
from enum import Enum
from collections import defaultdict
import random, struct
import z3
from pysat.solvers import Solver
from pysat.card import CardEnc
from pysat.formula import IDPool

"""
This script is responsible for generating the programs that are executing in the shader.

Each program also has four associated notes. In each execution, registers r0 through r3
contain the *FRAME NUMBER* that that note was successfully played, or -1 if that note has
not been played yet or was missed.

The interpreter for this is in src/shaders/checker.vert

"""


class Opcode(Enum):
    R_ADD = 0
    R_SUB = 1
    R_AND = 2
    R_OR = 3
    R_JZ = 4
    R_JNZ = 5
    R_LOL = 6
    R_CMP = 7
"""
                if (o1 == 0u) {
                set1(o2, get2(o3));
            } else if (o1 == 1u) {
                int z = get1(o2);
                set1(o2, get2(o3));
                set2(o3, z);
            } else if (o1 == 2u) {
                set1(o2, int(o3));
            } else if (o1 == 3u) {
                set1(o2, 0xffffffff);
            }
"""

NOP = b"\x80"
class SpecialReg(Enum):
    STATE = 0
    TEMP = 1

def ass(opc, r1, r2, o):
    inst = opc.value & 0x7
    inst |= ((r1 & 0x3) << 3)
    inst |= ((r2 & 0x3) << 5)
    inst |= ((o & 0x1) << 7)
    return struct.pack("<B", inst)

class Program:
    def __init__(self):
        self.b = b""
        self.relocations = {}
        self.labels = {}

    def xchg_special(self, reg, special):
        self.b += ass(Opcode.R_LOL, 1, reg, special.value)

    def fetch_special(self, reg, src_special):
        self.b += ass(Opcode.R_LOL, 0, reg, src_special.value)

    def mov_const(self, reg, const):
        if const in {0, 1}:
            self.b += ass(Opcode.R_LOL, 2, reg, const)
        elif const == -1:
            self.b += ass(Opcode.R_LOL, 3, reg, 0)
        else:
            assert False, "const must be 0, 1, or -1"

    def fix_relocations(self):
        for jmp_idx, label in self.relocations.items():
            dst_idx = self.labels[label]
            rel_off = dst_idx - (jmp_idx + 1)
            biased_off = rel_off + 7
            assert 0 <= biased_off <= 0xf
            fix = self.b[jmp_idx]
            fix |= ((biased_off) << 3)
            self.b = self.b[:jmp_idx] + bytes([fix]) + self.b[jmp_idx+1:]
            # if rel_off > 1:
            #     assert rel_off <= 5
            #     self.b[jmp_idx] |= ((rel_off-2) << 5)
            # elif rel_off <= -1:
            #     assert rel_off >= -4
            #     self.b[jmp_idx] |= (1 << 7)
            #     off = abs(rel_off)
            #     self.b[jmp_idx] |= ((off-1) << 5)

    def jz(self, special_reg, label):
        self.relocations[len(self.b)] = label
        self.b += ass(Opcode.R_JZ, 0, 0, special_reg.value)

    def jnz(self, special_reg, label):
        self.relocations[len(self.b)] = label
        self.b += ass(Opcode.R_JNZ, 0, 0, special_reg.value)


    def jmp(self, label):
        self.alu(Opcode.R_SUB, 0, 0, SpecialReg.TEMP)
        self.jz(SpecialReg.TEMP, label)

    def label(self, label):
        self.labels[label] = len(self.b)

    def alu(self, opc, r1, r2, dest):
        self.b += ass(opc, r1, r2, dest.value)

    def lt_zero(self, reg, special_dest):
        self.alu(Opcode.R_CMP, reg, reg, special_dest)

    def lt(self, reg1, reg2, special_dest):
        self.alu(Opcode.R_CMP, reg1, reg2, special_dest)

    def gen(self):
        self.fix_relocations()
        assert len(self.b) <= 32
        return self.b.ljust(32, NOP)

class Constraint:
    def __init__(self, code, regs, sat_constraints):
        self.code = code
        self.regs = regs
        self.sat = sat_constraints

    def encode(self):
        return struct.pack("<4Q32s", *self.regs, self.code)

def get_sat_constraints(it, notes):
    options_for_note = []
    constraints = []
    max_tick = max([n.time_due for n in notes])
    for n in notes:
        options = {i: pool.id() for i in range(n.time_due-SCORE_THRESHOLD, n.time_due-1)}
        constraints += CardEnc.atmost(list(options.values()), 1, vpool=pool)
        options_for_note.append(options)

    # Now, for every position, check at most one
    is_tick_free = []
    for i in range(max_tick):
        vs = []
        this_tick_free = pool.id()
        is_tick_free.append(this_tick_free)
        for o in options_for_note:
            if i in o:
                vs.append(o[i])
        constraints += CardEnc.atmost(vs + [this_tick_free], 1, vpool=pool)
        # this_tick_free will be forced false if any are used on this tick

    # Now we enforce that we don't select something two frames in a row
    for i in range(1, max_tick):
        prev = is_tick_free[i-1]
        cur = is_tick_free[i]
        constraints += [[prev, cur]] # at least one of them must be free

    return constraints, options_for_note

class CardEncType(Enum):
    ATLEAST = 0
    ATMOST = 1

pool = IDPool()
class Generator:
    def __init__(self, index_to_tick, notes):
        self.it = index_to_tick
        self.included = [i for (i, x) in enumerate(index_to_tick) if x is not None]
        self.excluded = [i for (i, x) in enumerate(index_to_tick) if x is None]

        # add SAT constraints
        self.sat_constraints, self.sat_vars = get_sat_constraints(self.it, notes)

        self.max_tick = max([n.time_due for n in notes])
        # window cache
        self.tick_index = defaultdict(set)
        for i, n in enumerate(notes):
            self.tick_index[n.time_due].add(i)
        self.windows = []
        for t in range(self.max_tick):
            window = []
            for w in range(t+1, t+1+SCORE_THRESHOLD):
                window += self.tick_index[w]
            self.windows.append(window)
        self.windows_length_3 = [[x for x in w if self.it[x] is not None] for w in self.windows]
        self.windows_length_3 = [z for z in self.windows_length_3 if len(z) > 2]
        self.windows_length_3_weights = [len(z) for z in self.windows_length_3]

        self.windows_for_indirect = [z for z in self.windows if len(z) > 2]
        self.dedup = set()

        # print(self.windows)

    def add_sat_lt_included(self, a, b, cond=None):
        a_options = self.sat_vars[a]
        b_options = self.sat_vars[b]

        cons = []
        ext = [] if cond is None else [cond]
        for o1, v in a_options.items():
            # Suppose a is at o1
            # then b must be > o1
            gt_options = [v2 for x, v2 in b_options.items() if x > o1]
            cons.append([-v] + gt_options + ext)

        for o1, v in b_options.items():
            # Suppose b is at o1
            # then a must be < o1
            gt_options = [v2 for x, v2 in a_options.items() if x < o1]
            cons.append([-v] + gt_options + ext)
        return cons

    def add_sat_lt_half_implication(self, a, b):
        # includes b => a but not a => b
        a_options = self.sat_vars[a]
        b_options = self.sat_vars[b]

        for o1, v in a_options.items():
            # Suppose a is at o1
            # then b cannot be < o1
            lte_options = [v2 for x, v2 in b_options.items() if x <= o1]
            self.sat_constraints += [[-v, -x] for x in lte_options]

        for o1, v in b_options.items():
            # Suppose b is at o1
            # then a must be < o1
            gt_options = [v2 for x, v2 in a_options.items() if x < o1]
            self.sat_constraints.append([-v] + gt_options)


        # smt
        # self.var_incl = [z3.Bool(f'incl_{i}', 32) for i in range(len(index_to_tick))]
        # self.vars = [z3.BitVec(f'var_{i}', 32) for i in range(len(index_to_tick))]
        # self.s = z3.Solver()
        # self.s.add(z3.Distinct(*self.vars))

    ## Constraint generation

    # Direct a < b < c, with a, b, c all included
    def lt_a_b_c(self, combo=None):
        if combo is None:
            window = random.choices(self.windows_length_3, weights=self.windows_length_3_weights)[0]
            combo = random.sample(window, 3)
            combo = sorted(combo, key=lambda x: self.it[x])

        if tuple(combo) in self.dedup:
            return None
        self.dedup.add(tuple(combo))
        # print([self.it[x] for x in combo])
        # print(combo)
        # combo[0] < combo[1] < combo[2]
        p = Program()
        p.mov_const(3, 1) # initialize to OK
        p.lt_zero(0, SpecialReg.TEMP)
        p.jnz(SpecialReg.TEMP, 'fail1')
        p.lt_zero(1, SpecialReg.TEMP)
        p.jnz(SpecialReg.TEMP, 'fail1')
        p.lt_zero(2, SpecialReg.TEMP)
        p.jnz(SpecialReg.TEMP, 'fail1')
        p.jmp('pt2')

        p.label('fail1')
        p.mov_const(3, -1)
        p.label('pt2')
        p.lt(0, 1, SpecialReg.TEMP)
        p.jz(SpecialReg.TEMP, 'fail2')
        p.lt(1, 2, SpecialReg.TEMP)
        p.jz(SpecialReg.TEMP, 'fail2')
        p.jmp('end')

        p.label('fail2')
        p.mov_const(3, -1)
        p.label('end')
        p.xchg_special(3, SpecialReg.STATE)

        # ast = self.vars[combo[0]] < self.vars[combo[1]] and self.vars[combo[1]] < self.vars[combo[2]]
        # ast = ast and (self.var_incl[combo[0]] and self.var_incl[combo[1]] and self.var_incl[combo[2]])

        cs = []
        for c in combo:
            cs += [list(self.sat_vars[c].values())] # at least one of them!
        self.sat_constraints += cs
        self.sat_constraints += self.add_sat_lt_included(combo[0], combo[1])
        self.sat_constraints += self.add_sat_lt_included(combo[1], combo[2])
        return Constraint(p.gen(), combo + [0], None)

    def presence_strong(self):
        combo = random.sample(self.included + self.excluded, 3)
        if tuple(combo) in self.dedup:
            return None
        self.dedup.add(tuple(combo))

        # presence(A)
        p = Program()
        p.mov_const(3, 1) # initialize to OK

        for i, c in enumerate(combo):
            p.lt_zero(i, SpecialReg.TEMP)
            if self.it[c] is None:
                p.jz(SpecialReg.TEMP, 'fail1')
            else:
                p.jnz(SpecialReg.TEMP, 'fail1')

        p.jmp('finish')

        p.label('fail1')
        p.mov_const(3, -1)
        p.label('finish')
        p.xchg_special(3, SpecialReg.STATE)

        cs = []
        for c in combo:
            if self.it[c] is not None:
                cs += [list(self.sat_vars[c].values())] # at least one of them!
            else:
                cs += [[-x] for x in self.sat_vars[c].values()] # none of them!
        self.sat_constraints += cs

        return Constraint(p.gen(), combo + [0], None)

    def lt_indirect(self):
        # TODO: We can also choose windows where none or just the first one is included
        if random.randrange(5) > 0:
            window = random.choices(self.windows_length_3, weights=self.windows_length_3_weights)[0]
            combo = random.sample(window, 3)
        else:
            window = random.choice(self.windows_for_indirect)
            combo = random.sample(window, 3)
            # pos = random.choice([x for x in window if self.it[x] is not None])
            # combo = random.sample(set(window) - {pos}, 2)

            # Choose two others which are not included
        combo = sorted(combo, key=lambda x: self.it[x] if self.it[x] is not None else 100000) # put the nones-ies at the end
        if tuple(combo) in self.dedup:
            return None
        self.dedup.add(tuple(combo))
        p = Program()
        p.xchg_special(3, SpecialReg.STATE)
        p.lt_zero(3, SpecialReg.TEMP)
        p.jnz(SpecialReg.TEMP, 'fail1')
        p.mov_const(3, 1)

        p.lt_zero(1, SpecialReg.TEMP)
        p.jnz(SpecialReg.TEMP, 'pt2')
        p.lt_zero(0, SpecialReg.TEMP)
        p.jz(SpecialReg.TEMP, 'pt2')
        p.label('fail1')
        p.mov_const(3, -1)

        p.label('pt2')
        p.lt_zero(2, SpecialReg.TEMP)
        p.jnz(SpecialReg.TEMP, 'pt3')
        p.lt_zero(1, SpecialReg.TEMP)
        p.jz(SpecialReg.TEMP, 'pt3')
        p.mov_const(3, -1)
        p.label('pt3')

        p.xchg_special(3, SpecialReg.STATE)

        # This has the weaker constraint that it can be SATISIFED even if one of them are not included
        # but C => (A and B), and B => A
        self.add_sat_lt_half_implication(combo[0], combo[1])
        self.add_sat_lt_half_implication(combo[1], combo[2])

        return Constraint(p.gen(), combo + [0], None)

    def cardinality(self):
        # 'atleast 1' constraints are cheap, 'atleast 2' are somewhat less cheap
        combo = random.sample(self.included + self.excluded, 3)
        num_excluded = sum([1 if self.it[x] is None else 0 for x in combo])
        if tuple(combo) in self.dedup:
            return None
        self.dedup.add(tuple(combo))
        if num_excluded == 0:
            # at most 1
            constraint = CardEncType.ATMOST
            amount = random.randrange(1, 3) # 1 or 2
        elif num_excluded == 1:
            # at least 1
            constraint = CardEncType.ATLEAST
            amount = 1
        elif num_excluded == 2:
            if random.randrange(3) > 0:
                constraint = CardEncType.ATLEAST
                amount = random.randrange(1, 3) #  1 or 2
            else:
                constraint = CardEncType.ATMOST
                amount = 2
        else:
            assert num_excluded == 3
            constraint = CardEncType.ATLEAST
            amount = random.randrange(1, 3) # 1 or 2

        p = Program()
        p.lt_zero(0, SpecialReg.TEMP)
        p.fetch_special(0, SpecialReg.TEMP)
        p.lt_zero(1, SpecialReg.TEMP)
        p.fetch_special(1, SpecialReg.TEMP)
        p.lt_zero(2, SpecialReg.TEMP)
        p.fetch_special(2, SpecialReg.TEMP)

        p.alu(Opcode.R_ADD, 0, 1, SpecialReg.TEMP)
        p.fetch_special(3, SpecialReg.TEMP)
        p.alu(Opcode.R_ADD, 3, 2, SpecialReg.TEMP)
        p.fetch_special(0, SpecialReg.TEMP)

        p.mov_const(3, 1)
        if amount == 2:
            p.alu(Opcode.R_ADD, 3, 3, SpecialReg.TEMP)
            p.fetch_special(3, SpecialReg.TEMP)

        if constraint == CardEncType.ATLEAST:
            # if R0 < R3, we fail
            p.lt(0, 3, SpecialReg.TEMP)
        else:
            assert constraint == CardEncType.ATMOST
            # if R0 > R3, we fail
            p.lt(3, 0, SpecialReg.TEMP)

        p.mov_const(3, 1) # default to success
        p.jz(SpecialReg.TEMP, 'succ')
        p.mov_const(3, -1) # fail

        p.label('succ')
        p.xchg_special(3, SpecialReg.STATE)

        combined_vars = []
        for c in combo:
            combined_vars += list(self.sat_vars[c].values())
        if constraint == CardEncType.ATLEAST:
            # at least X excluded means at most 3-X included
            self.sat_constraints += CardEnc.atmost(combined_vars, 3-amount, vpool=pool)
        else:
            # at most X excluded means at least 3-X included
            self.sat_constraints += CardEnc.atleast(combined_vars, 3-amount, vpool=pool)

        return Constraint(p.gen(), combo + random.sample(self.included + self.excluded, 1), None)

    def always_ok(self):
        combo = random.sample(self.included, 3)
        combo = sorted(combo, key=lambda x: self.it[x])
        p = Program()
        p.mov_const(3, 1) # initialize to ok
        p.xchg_special(3, SpecialReg.STATE)
        return Constraint(p.gen(), combo + [0])

    def never_jump(self, program, reg_assigns):
        p = program
        p

    def always_jump(self, program, reg_assigns):
        p = program
        p


def gen_constraints(notes, index_to_tick, orig_replay):
    gen = Generator(index_to_tick, notes)
    constraints = []
    progress = 0
    for _ in range(100000):
        r = random.randrange(9)
        if r in {0, 1, 2, 3}:
            cons = gen.lt_indirect()
        elif r in {4}:
            cons = gen.lt_a_b_c()
        else:
            cons = gen.cardinality()
            # cons = gen.presence_strong()
        if cons is not None:
            constraints.append(cons)
        progress += 1
        if progress % 10000 == 0:
            print(progress)
        # gen.s.add(cons)
    # soln = gen.s.solution()
    freplay_orig = [(i, r) for (i, r) in enumerate(orig_replay) if r != -1]
    while True:
        print(f"checkers: {len(constraints)}")
        print(len(gen.sat_constraints))
        solver = Solver('cadical', bootstrap_with=gen.sat_constraints)
        is_sat = solver.solve()
        answer = solver.get_model()

        print(f"is_sat: {is_sat}")
        replay = [-1 for _ in range(max([n.time_due for n in notes]))]
        for i, n in enumerate(notes):
            options = gen.sat_vars[i]
            for o, var in options.items():
                if answer[var-1] > 0:
                    assert replay[o] == -1
                    replay[o] = i
        # print(replay)
        # try to get flag

        freplay = [(i, r) for (i, r) in enumerate(replay) if r != -1]
        problem_areas = []
        for (i1, r1), (i2, r2) in zip(freplay, freplay_orig):
            if r1 != r2:
                problem_areas += [i1, i2]
                print(f"diff @ {i2} {i2} -- {r1} vs. {r2}")

        # Add some constraints in the problem areas
        for p in problem_areas:
            # gen.windows_length_3 = [[x for x in w if gen.it[x] is not None] for w in gen.windows[p-SCORE_THRESHOLD:p+SCORE_THRESHOLD]]
            # gen.windows_length_3 = [z for z in gen.windows_length_3 if len(z) > 2]
            gen.windows_length_3 = [z for z in gen.windows[p-SCORE_THRESHOLD:p+SCORE_THRESHOLD] if len(z) > 2]
            # gen.windows_length_3 = []
            gen.windows_length_3 += [z+[freplay_orig[-1][1]] for z in gen.windows[p-SCORE_THRESHOLD:p+SCORE_THRESHOLD] if len(z) == 2]
            assert len(gen.windows_length_3) > 0
            gen.windows_length_3_weights = [1 for _ in gen.windows_length_3]
            for _ in range(50):
                cons = gen.lt_indirect()
                if cons is not None:
                    constraints.append(cons)


        if len(problem_areas) == 0:
            break
        solver.delete()

    bits = []
    for r in replay:
        if r != -1:
            # print(r & 1)
            bits.append(r & 1)
    answer = []
    for i in range(0, len(bits), 8):
        num = 0
        for z in range(8):
            if i + z >= len(bits):
                break
            num |= (bits[i+z] << z)
        answer.append(num)
    flag = bytes(answer)
    print(flag)

    ## CHECKS FOR UNIQUENESS

    # 1: Check that no element that is not included can be included
    extra_constraint = []
    for i in gen.excluded:
        extra_constraint += list(gen.sat_vars[i].values())

    aux1 = pool.id()
    extra_constraint.append(aux1)
    solver.add_clause(extra_constraint)
    # solver = Solver('cadical', bootstrap_with=gen.sat_constraints + [extra_constraint])
    is_sat = solver.solve()
    assert is_sat
    is_sat = solver.solve(assumptions=[-aux1])
    assert not is_sat

    # 2: Check that you can't swap any two elements
    for i in range(1, len(freplay)):
        prev = freplay[i-1]
        cur = freplay[i]

        aux = pool.id()
        extra_constraints = gen.add_sat_lt_included(cur[1], prev[1], cond=aux)
        solver.append_formula(extra_constraints)
        while True:
            is_sat = solver.solve()
            assert is_sat
            is_sat = solver.solve(assumptions=[-aux])
            if is_sat:
                print(f"Fixing {cur[1]} and {prev[1]}, which could have been flipped")
                cons = gen.lt_a_b_c([prev[1], cur[1], freplay[i+3][1]])
                assert cons is not None
                constraints.append(cons)
                solver.append_formula(gen.add_sat_lt_included(prev[1], cur[1]))
                continue
            assert not is_sat
            break
        if (i % 25) == 0:
            print(i)

    return constraints



SCORE_THRESHOLD = 30
def generate_programs(notes, flag):
    # Pick the notes that are necessary to spell the flag
    encoded_flag = []
    for c in flag:
        c = ord(c)
        encoded_flag.append(c & 1)
        encoded_flag.append((c >> 1) & 1)
        encoded_flag.append((c >> 2) & 1)
        encoded_flag.append((c >> 3) & 1)
        encoded_flag.append((c >> 4) & 1)
        encoded_flag.append((c >> 5) & 1)
        encoded_flag.append((c >> 6) & 1)
        encoded_flag.append((c >> 7) & 1)
    index_to_tick = [None for _ in range(len(notes))]
    last_tick = notes[-1].time_due

    tick_index = defaultdict(set)
    for n in notes:
        tick_index[n.time_due].add(n)

    replay = []

    last = False
    for t in range(last_tick):
        if len(encoded_flag) == 0:
            break
        window = []
        for w in range(t+1, t+1+SCORE_THRESHOLD):
            window += tick_index[w]
        if not last and random.randrange(13) < 2:
            # With 50% probability, we can pick one note from this range
            choices = [x for x in window if x.i & 1 == encoded_flag[0]]
            if len(choices) == 0:
                replay.append(-1)
                continue
            note = random.choice(choices)
            tick_index[note.time_due].remove(note)
            index_to_tick[note.i] = t
            encoded_flag = encoded_flag[1:]
            replay.append(note.i)
            last = True
        else:
            replay.append(-1)
            last = False
            continue
    replay += [-1 for _ in range(last_tick - len(replay))]

    print(f"Finished encoding flag in {t}/{last_tick} ticks, {len(encoded_flag)} bits remain")
    print(f"{len(flag) * 8} bits / {len(notes)}")

    constraints = gen_constraints(notes, index_to_tick, replay)
    random.shuffle(constraints)
    return replay, index_to_tick, constraints



