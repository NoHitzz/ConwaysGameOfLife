SDL3 Conway's Game of Life
======

A C++ implementation of [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) using [SDL3](https://wiki.libsdl.org/SDL3/FrontPage).  

Supports starting from a random configuration, drawing cells manually, or pasting a whole pattern (e.g. from [Life Lexicon](http://www.radicaleye.com/lifepage/lexicon.html)). 
Pasted patterns must follow the Life Lexicon format:
- Live cells are represented by ```O```
- Each row is on a new line
  
Cells can be inspected, highlighting the live neighbouring cells. In games smaller than 200x200, each cell also displays its live neighbour count.

![SDL3Conway Screenshot](/resources/conway_screenshot.png?raw=true "SDL3 Conway's Game of Life Screenshot")

Installation
--------------------
- Clone the repository
- Build the project: ```make``` 
- Run the application: ```make run``` or  ```./build/gameOfLife <size>```

Controls
--------------------

|Button/Combination | Function                                        |
|-------------------|-------------------------------------------------|
| r                 | reset (random)                                  |
| c                 | clear                                           |
| d                 | enter/exit draw mode                            |
| left mouse button | inspect/draw                                    |
| space             | pause/continue                                  |
| right arrow       | advance one step                                |
| esc               | leave mode                                      |
| ctrl + v          | paste pattern                                   |

Dependencies
--------------------
- SDL3
- SDL3_ttf
