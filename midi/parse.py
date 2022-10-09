from mido import MidiFile

mid = MidiFile('/Users/andrew/Downloads/2179360_1.mid', clip=True)
print(mid)
for track in mid.tracks:
    print(track)
piano = mid.tracks[1]

t = 0
notes = []
for msg in piano:
    t += msg.time
    if msg.type == 'note_on':
        if len(notes) == 0 or notes[-1] != t:
            notes.append(t)
    # print(msg)
print(f"{len(notes)} notes")


for track in mid.tracks[:]:
    print(track.name)
    if 'Piano' not in track.name:
        mid.tracks.remove(track)

# mid.save('new_song.mid')
