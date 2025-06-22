SDL3 Conway's Game of Life
======

An SDL3/C++ implementation of [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life).  

![SDL3Conway Screenshot](/resources/conway_screenshot.png?raw=true "SDL3 Conway's Game of Life Screenshot")

Installation
--------------------
- Clone this repo
- Compile with ```make``` 
- Run with ```make run``` or  ```./build/conway <size>```


Dependencies
--------------------
- SDL3
- SDL3_ttf

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
| ctrl-v            | paste pattern from [life lexicon]{http://www.radicaleye.com/lifepage/lexicon.html} |
