import re
from PIL import Image

xres = 1920
yres = 1080

with open("recording.dat", "rb") as recording:
    bytes = []
    for i in range(4):
        bytes.append(recording.read(1))

    image = Image.new('RGB', (xres, yres), color='black')
    data = image.load()

    image_counter = 0
    pixel_counter = 0
    while bytes[0]:
        r = int.from_bytes(bytes[0], byteorder='big')  # Convert byte to integer
        g = int.from_bytes(bytes[1], byteorder='big')  # Convert byte to integer
        b = int.from_bytes(bytes[2], byteorder='big')  # Convert byte to integer
        data[pixel_counter%xres, pixel_counter//xres] = (r, g ,b)
        bytes = []
        for i in range(4):
            bytes.append(recording.read(1))
        if pixel_counter == xres*yres-1:
            image.save("images/image"+str(image_counter)+".png")
            pixel_counter = 0
            image_counter += 1
            image = Image.new('RGB', (xres, yres), color='black')
            data = image.load()
        else:
            pixel_counter += 1

        if image_counter == 200:
            break
