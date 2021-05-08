# Vulkan Pong

Vulkan Pong is a simple Pong Clone using the Vulkan 1.0 API. I created it to test the Vulkan 1.0 API to develop an understanding
of it. This is a very simple game that doesn't have an ending but I learned quite a bit of how Vulkan is used while creating this 
little game project. Now here is how to play...

If you simply click on the run.exe program it will make the compute play on easy mode. To increase the difficulty of the game you'll
need to open windows CMD in the folder location and type this in...

run [difficulty]

difficulty = {
    easy,
    normal,
    hard,
    expert,
    impossible
}

The set above is all the difficulty settings the game has... Sorry I didn't have time to create a menu system. It just due to me wanting
to get started with another project that uses textures... Oh and one more thing the game will to control the paddle just using the arrow
keys. 

Up Arrow = moving the paddle up
Down Arrow = moving the paddle down

The last thing is that the game doesn't keep track of any score so there is no ending to it. It really just a demonstration to show that
I got Vulkan working. I also love creating demos that actually has functionality because it makes it more interesting... Well enjoy this 
infinite pong game and have a nice day...

Dependencies
SDL2
SDL2_image
SDL2_ttf
SDL2_mixer
vulkan-1

The MIT License (MIT)
Copyright © 2021 <Frederic R. Cook>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.