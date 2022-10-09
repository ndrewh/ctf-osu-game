

with open("tilemap.png", "rb") as f:
    data = f.read()

b = ",".join(map(str, data))

with open("../src/tilemap.h", "w") as f:
    f.write(f"unsigned char tilemap[{len(data)}] = {{{b}}};\nint tilemap_len = {len(data)};\n")
