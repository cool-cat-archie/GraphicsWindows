As a command line argument, you can specify the file name of what texture to use for the billboard.

By default, the billboard shown uses Alpha Distribution. 

If you click the 1 key, it will display with normal alpha testing with no distribution.

If you click the 2 key, it will display with Alpha Distribution.

If you click the 3 key, it will display with Stochastic Alpha Testing.

If you click the 4 key, it will display with Hashed Alpha Testing.

If you want to zoom in/out you must hold the control key and drag the mouse up and down. This is useful for showing how all the approaches compare at the different mipmap levels.

I used visual studio 2019 and glut with the cy libraries on windows. I also use the lodepng library similarly to the previous projects. All of this is included in the zip file and is runnable most easily through the visual studio solution.
