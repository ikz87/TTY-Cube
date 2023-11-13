import sys
from PIL import Image

# Import an image from directory: 
input_image = Image.open(sys.argv[1])

# Extracting pixel map: 
pixel_map = input_image.load()

# Extracting the width and height  
# of the image: 
width, height = input_image.size

with open("image.dat", "wb") as file:
    for j in range(height):
        for i in range(width):
            # getting the RGB pixel value. 
            try:
                r, g, b = input_image.getpixel((i, j))
            except:
                r, g, b, a = input_image.getpixel((i, j))
            file.write(r.to_bytes())
            file.write(g.to_bytes())
            file.write(b.to_bytes())
