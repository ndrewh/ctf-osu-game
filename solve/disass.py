
from enum import Enum
import struct
class Opcode(Enum):
    R_ADD = 0
    R_SUB = 1
    R_AND = 2
    R_OR = 3
    R_JZ = 4
    R_JNZ = 5
    R_LOL = 6
    R_CMP = 7

class SpecialReg(Enum):
    STATE = 0
    TEMP = 1

def disass(inst, cur_pc):
    opc = Opcode(inst & 0x7)
    o1 = ((inst >> 3) & 0x3)
    o2 = ((inst >> 5) & 0x3)
    o3 = ((inst >> 7) & 0x1)

    s = ""
    o1_name = f"r{o1}"
    o2_name = f"r{o2}"
    o3_name = "state" if o3 == 0 else "temp"
    match opc:
        case Opcode.R_ADD:
            s += f"{o3_name} = {o1_name} + {o2_name}"
        case Opcode.R_SUB:
            s += f"{o3_name} = {o1_name} - {o2_name}"
        case Opcode.R_AND:
            s += f"{o3_name} = {o1_name} & {o2_name}"
        case Opcode.R_OR:
            s += f"{o3_name} = {o1_name} | {o2_name}"
        case Opcode.R_JZ:
            off = (o2 << 2) | o1
            s += f"jz {o3_name}, {hex(cur_pc+off-6)}"
        case Opcode.R_JNZ:
            off = (o2 << 2) | o1
            s += f"jnz {o3_name}, {hex(cur_pc+off-6)}"
        case Opcode.R_LOL:
            if o1 == 0:
                s += f"{o2_name} = {o3_name}"
            elif o1 == 1:
                s += f"xchg {o2_name}, {o3_name}"
            elif o1 == 2:
                s += f"{o2_name} = {hex(o3)}"
            elif o1 == 3:
                s += f"{o2_name} = {hex(0xffffffff)}"
        case Opcode.R_CMP:
            if o1 == o2:
                s += f"{o3_name} = {o1_name} < 0"
            else:
                s += f"{o3_name} = {o1_name} < {o2_name}"
    return s

def disass_program(code):
    s = ""
    for off, i in enumerate(code):
        ss = disass(i, off)
        if len(ss) > 0:
            s += f"[{hex(off)}] " + ss + "\n"
    print(s)

with open("../dist-osu/game.beatmap", "rb") as f:
    data = f.read()

note_count, constraint_count = struct.unpack("<QQ", data[:16])
constraints = data[16+24*note_count:]
# Read the constraints in

code_examples = set()
for x in range(0, len(constraints), 64):
    constraint = constraints[x:x+64]
    decoded = list(struct.unpack("<4Q32s", constraint))
    circle_list, code = decoded[:4], decoded[4]
    code_examples.add(code)

print(len(code_examples))
for c in code_examples:
    print(c)
    disass_program(c)
