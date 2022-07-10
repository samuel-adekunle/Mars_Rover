#Endpoints
##/ball - POST
To place a ball on the map make a post request to /ball with the a JSON body as such 
>{"pos":{"x":10,"y":12},"colour":16711680}

This places a ball at position (10,12) with RGB value 0xFF0000.
* NB -All balls must have unique colours. If a second ball is added with the same colour the first ball is deleted.

##/reset - POST
Resets the maps. Removes all balls and paths. No JSON body required.

##/roverPosition - POST

Sets the start position of all find path requests. Requires JSON body as such:
>{"pos":{"x":0,"y":0},"theta":0}

This places the rover at (0,0) with heading 0

##/roverPosition - GET
Gets the position of the rover. Response JSON as follows
>{"pos":{"x":0,"y":0},"theta":0}

##/findPath  - GET
Returns a series of (x,y) points representing the fastest path. JSON body required to set destination

>{"x":20,"y":30}

Response JSON as follows

>[{"x":20,"y":30},{"x":6,"y":14},{"x":5,"y":13},{"x":0,"y":0}]

##/deletePath - POST
Removes the most recent path from being drawn on any new SVG

##/svg - POST
Returns the SVG of the map with the most recent path drawn on. Requires a configuration JSON body to detail how to display the map
>{
"displayRover":true,
"displayBallPadding":true,
"displayVisitedTiles":true,
"displayQueuedTiles":true,
"darkMode":true
}
