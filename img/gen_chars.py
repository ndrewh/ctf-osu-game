from PIL import Image, ImageDraw, ImageFont

PER_ROW = 15
ROWS = 5
CHAR_WIDTH = 60
CHAR_HEIGHT = 110
def make_tilemap(x):
    image = Image.new("RGBA", (CHAR_WIDTH*PER_ROW,CHAR_HEIGHT * ROWS), (0, 0, 0,0))
    draw = ImageDraw.Draw(image)
    font = ImageFont.truetype("/Library/Fonts/RobotoMono-Regular.ttf", 96)
    idx = 0
    for i in range(ord('A'), ord('Z')+1):
        draw.text((CHAR_WIDTH*(idx % PER_ROW), (idx // PER_ROW) * CHAR_HEIGHT), chr(i), (255,255,255), font=font)
        idx += 1

    for i in range(ord('0'), ord('9')+1):
        draw.text((CHAR_WIDTH*(idx % PER_ROW), (idx // PER_ROW) * CHAR_HEIGHT), chr(i), (255,255,255), font=font)
        idx += 1

    for i in ['{', '}', '_', ':', ' ', '-']:
        draw.text((CHAR_WIDTH*(idx % PER_ROW), (idx // PER_ROW) * CHAR_HEIGHT), i, (255,255,255), font=font)
        idx += 1

    font2 = ImageFont.truetype("Arial Unicode.ttf", 72)
    for i in {"✓"}:
        draw.text((CHAR_WIDTH*(idx % PER_ROW), (idx // PER_ROW) * CHAR_HEIGHT), i, (255,255,255), font=font2)
        idx += 1

    for i in range(ord('a'), ord('z')+1):
        draw.text((CHAR_WIDTH*(idx % PER_ROW), (idx // PER_ROW) * CHAR_HEIGHT), chr(i), (255,255,255), font=font)
        idx += 1

    # img_resized = image.resize((188,45), Image.ANTIALIAS)

    image.save(f"tilemap.png", "PNG", optimize=True, quality=80);

make_tilemap(0)

