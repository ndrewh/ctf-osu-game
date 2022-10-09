R"(
#version 150
in ivec4 r;
in uvec4 code[2];
in int s;
ivec4 r_c;
int s_c;
int t;
out int out_attr;
out vec4 color;

int get1(uint i) {
    switch (i) {
        case 0u:
            return r_c.x;
        case 1u:
            return r_c.y;
        case 2u:
            return r_c.z;
        case 3u:
            return r_c.w;
        default:
            return 0;
    }
}
void set1(uint i, int val) {
    switch (i) {
        case 0u:
            r_c = ivec4(val, r_c.yzw);
            break;
        case 1u:
            r_c = ivec4(r_c.x, val, r_c.zw);
            break;
        case 2u:
            r_c = ivec4(r_c.xy, val, r_c.w);
            break;
        case 3u:
            r_c = ivec4(r_c.xyz, val);
            break;
        default:
            return;
    }
}

int get2(uint i) {
    return (i == 0u) ? s_c : t;
}

void set2(uint i, int val) {
    if (i == 0u) {
        s_c = val;
    } else {
        t = val;
    }
}

uint handle_insn(uint insn, uint pc) {
    int r1, r2, r3;
    uint op = insn & 0x7u;
    uint o1 = (insn >> 3) & 0x3u;
    uint o2 = (insn >> 5) & 0x3u;
    uint o3 = (insn >> 7) & 0x1u;
    uint next = pc + 1u;
    switch (op) {
        case 0u:
        case 1u:
        case 2u:
        case 3u:
            r1 = get1(o1);
            r2 = get1(o2);
            if (op == 0u) {
                set2(o3, r1 + r2);
            } else if (op == 1u) {
                set2(o3, r1 - r2);
            } else if (op == 2u) {
                set2(o3, r1 & r2);
            } else if (op == 3u) {
                set2(o3, r1 | r2);
            }
            break;
        case 4u:
            r3 = get2(o3);
            if (r3 == 0) {
                uint off = (o2 << 2) | o1;
                next += uint(int(off) - 7);
            }
            break;
        case 5u:
            r3 = get2(o3);
            if (r3 != 0) {
                uint off = (o2 << 2) | o1;
                next += uint(int(off) - 7);
            }
            break;
        case 6u:
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
            break;
        case 7u:
            r1 = get1(o1);
            r2 = get1(o2);
            if (o1 == o2) {
                set2(o3, (r1 < 0) ? 1 : 0);
            } else {
                set2(o3, (r1 < r2) ? 1 : 0);
            }
            break;
    }
    return next;
}

uint fetch_insn_4(uint v, uint idx) {
    switch (idx) {
        case 0u:
          return v & 0xffu;
        case 1u:
          return (v >> 8) & 0xffu;
        case 2u:
          return (v >> 16) & 0xffu;
        case 3u:
          return (v >> 24) & 0xffu;
    }
}

uint fetch_insn_16(uvec4 v, uint idx) {
    switch (idx >> 2) {
        case 0u:
          return fetch_insn_4(v.x, idx & 0x3u);
        case 1u:
          return fetch_insn_4(v.y, idx & 0x3u);
        case 2u:
          return fetch_insn_4(v.z, idx & 0x3u);
        case 3u:
          return fetch_insn_4(v.w, idx & 0x3u);
    }
}

uint fetch_insn(uint pc) {
    if ((pc >> 4) == 0u) {
        return fetch_insn_16(code[0], pc & 0xfu);
    } else {
        return fetch_insn_16(code[1], pc & 0xfu);
    }
}
void main()
{
    r_c = r;
    s_c = s;

    uint count = 0u;
    uint pc = 0u;
    while (count < 10000u && pc < 32u) {
        uint insn = fetch_insn(pc);
        pc = handle_insn(insn, pc);
        count += 1u;
    }
    out_attr = s_c;
    color = vec4(0.0, 0.0, 1.0, 1.0);
    gl_Position = vec4(0, 0, 0, 0);
}
)"
