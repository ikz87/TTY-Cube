int kernel[3][3] = { { 4, 8, 4},
                     { 8, 16, 8},
                     { 4, 8, 4} };

int radius = 1;

void blur_pixels(char pixels[], int x, int y)
{
    for (int j = 0; j < y; j++)
    {
        for (int i = 0; i < x; i++)
        {
            unsigned char total[] = {0,
                    0,
                    0};
            for (int ky = -radius; ky <=radius; ky++)
            {
                for (int kx = -radius; kx <=radius; kx++)
                {
                    if (i+kx >=0 && i+kx < x && j+ky >=0 && j+ky < y)
                    {
                        int value = ((j+ky)*x + i+kx)*4;
                        unsigned char rgb[] = {pixels[value],
                            pixels[value+1],
                            pixels[value+2]};
                        for (int c = 0; c < 3; c++)
                        {
                           total[c] += rgb[c]*kernel[kx+radius][ky+radius]/64;
                        }
                    }
                }
            }
            
            for (int c = 0; c < 3; c++)
            {
                if ((unsigned)(unsigned char)pixels[(j*x + i)*4 + 3] == 87)
                {
                    pixels[(j*x + i)*4 + c] = (unsigned)total[c]; 
                }

            }
        }    
    }    
}
