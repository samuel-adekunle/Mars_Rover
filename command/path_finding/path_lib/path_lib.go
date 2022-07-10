package path_lib

import (
	"bytes"
	"container/heap"
	"fmt"
	"math"
	"sort"

	svg "github.com/ajstarks/svgo"
)

type Colour uint32
func (c Colour) toString() string {
	return "fill:#" + fmt.Sprintf("%06x", uint64(c))
}

const BALL_PADDING = 8
type Ball struct {
	Pos    Pos    `json:"pos"`
	Colour Colour `json:"colour"`
}

type Pos struct {
	X int 	`json:"x"`
	Y int	`json:"y"`
}

type HeapEntry struct {
	Pos                Pos
	DistStart, DistEnd float64
	parent             *HeapEntry
}
type PriorityQueue []HeapEntry
func (h PriorityQueue) Len() int { return len(h) }
func (h PriorityQueue) Less(i, j int) bool {
	return (h[i].DistStart + h[i].DistEnd) < (h[j].DistStart + h[j].DistEnd)
}
func (h PriorityQueue) Swap(i, j int) { h[i], h[j] = h[j], h[i] }
func (h *PriorityQueue) Push(x interface{}) {
	*h = append(*h, x.(HeapEntry))
}
func (h *PriorityQueue) Pop() interface{} {
	old := *h
	n := len(old)
	x := old[n-1]
	*h = old[0 : n-1]
	return x
}

type Status byte
const (
	FREE Status = iota
	BLOCKED
	QUEUED
	VISITED
)

type Rover struct {
	Pos   Pos     `json:"pos"`
	Theta float64 `json:"theta"`
}
type Map struct {
	balls  []Ball
	Blocks [][]Status
	Path   []Pos
	Rover  Rover
	MAP_WIDTH int
	BALL_WIDTH int
	TILE_WIDTH int
	TILE_DIAGONAL_WIDTH float64
	MAP_WIDTH_IN_TILES int
}

func MakeMap(width int) (m Map){
	m.MAP_WIDTH = width
	m.BALL_WIDTH = 50
	m.TILE_WIDTH= m.BALL_WIDTH / 2
	m.TILE_DIAGONAL_WIDTH = float64(m.TILE_WIDTH) * math.Sqrt2
	m.MAP_WIDTH_IN_TILES = m.MAP_WIDTH / m.TILE_WIDTH
	m.Blocks = make([][]Status, width)
	for i,_ := range m.Blocks {
		m.Blocks[i] = make([]Status, width)
	}
	m.Rover = Rover{
		Pos{50,50},
		0,
	}
	return
}
func (m * Map) insideMap(i int) bool {
	return i >= 0 && i < (m.MAP_WIDTH_IN_TILES-1)
}
func abs(x int) int {
	if x < 0 {
		return -x
	}
	return x
}
func (m *Map) smoothPath() {
	var end Pos
	raytrace := func(i int) bool {
		start := m.Path[i]

		dx := abs(end.X - start.X)
		dy := abs(end.Y - start.Y)
		pos := start
		n := 1 + dx + dy
		error := dx - dy
		var x_inc int
		var y_inc int
		if end.X > start.X {
			x_inc = 1
		} else {
			x_inc = -1
		}
		if end.Y > start.Y {
			y_inc = 1
		} else {
			y_inc = -1
		}
		dx *= 2
		dy *= 2

		for ; n > 0; n-- {
			if !m.insideMap(pos.X) || !m.insideMap(pos.Y) {
				return false
			}
			if m.Blocks[pos.Y][pos.X] == BLOCKED {
				return false
			}
			if error > 0 {
				pos.X += x_inc
				error -= dy
			} else {
				pos.Y += y_inc
				error += dx
			}
		}
		return true
	}

	for i := 1; i < len(m.Path); i++ {
		end = m.Path[len(m.Path) - i]
		for {
			cut := sort.Search(len(m.Path) - i - 1, raytrace)
			if (cut == len(m.Path) - i - 1) || (cut == 0 && len(m.Path) - i == 1){
				break
			}
			m.Path = append(m.Path[0:cut+1], m.Path[len(m.Path) - i:]...)
		}
	}
}
func (m *Map) PlaceBall(ball Ball) {
	m.DeleteBallByColour(ball.Colour)
	m.balls = append(m.balls, ball)
}
func (m *Map) FindPath(startPos, endPos Pos) bool {
	m.Path = []Pos{}
	for y := 0; y < m.MAP_WIDTH_IN_TILES; y++ {
		for x := 0; x < m.MAP_WIDTH_IN_TILES; x++ {
			m.Blocks[y][x] = FREE
		}
	}

	for _, ball := range m.balls {
		topX := ball.Pos.X - BALL_PADDING
		topY := ball.Pos.Y - BALL_PADDING

		for y := topY; y < topY+2*(BALL_PADDING); y++ {
			for x := topX; x < topX+2*(BALL_PADDING); x++ {
				if m.insideMap(x) && m.insideMap(y) {
					m.Blocks[y][x] = BLOCKED
				}
			}
		}
	}

	findNearestUnblocked := func(pos Pos) (Pos) {
		if m.Blocks[pos.Y][pos.X] == FREE { return pos }
		for r:= 0; ;r++{
			x, y := r, 0
			p := 1 - r
			for x > y {
				y++
				if p <= 0 {
					p +=  2 * y + 1
				} else{
					x--
					p += 2*y - 2*x + 1
				}
				if x < y {
					continue
				}
				firstCheck := []Pos{
					Pos{x+pos.X, y+pos.Y},
					Pos{-x+pos.X, y+pos.Y},
					Pos{x+pos.X, -y+pos.Y},
					Pos{-x+pos.X, -y+pos.Y},
				}
				for i:=0; i < len(firstCheck); i++ {
					if !m.insideMap(firstCheck[i].Y) || !m.insideMap(firstCheck[i].X){
						continue
					}
					if m.Blocks[firstCheck[i].Y][firstCheck[i].X] == FREE {
						return firstCheck[i]
					}
				}
				if x != y {
					additionalCheck := []Pos{
						Pos{y + pos.X, x + pos.Y},
						Pos{-y + pos.X, x + pos.Y},
						Pos{y + pos.X, -y + pos.Y},
						Pos{-y + pos.X, -y + pos.Y},
					}
					for i := 0; i < len(additionalCheck); i++ {
						if !m.insideMap(additionalCheck[i].Y) || !m.insideMap(firstCheck[i].X){
							continue
						}
						if m.Blocks[additionalCheck[i].Y][additionalCheck[i].X] == FREE {
							return additionalCheck[i]
						}
					}
				}
			}

		}
	}

	startPos = findNearestUnblocked(startPos)
	endPos = findNearestUnblocked(endPos)

	distance := func(startPos, endPos Pos) float64 {
		dx := float64(endPos.X - startPos.X)
		dy := float64(endPos.Y - startPos.Y)
		val := float64(m.TILE_WIDTH) * math.Sqrt(float64(math.Pow(dx, 2)+math.Pow(dy, 2)))
		return val
	}

	if m.Blocks[startPos.Y][startPos.X] == BLOCKED {
		panic("Path cannot start in blocked area")
	}

	m.Blocks[startPos.Y][startPos.X] = VISITED
	openSet := new(PriorityQueue)
	heap.Init(openSet)
	heap.Push(openSet, HeapEntry{startPos, 0, distance(startPos, endPos), nil})

	for openSet.Len() != 0 {
		p := heap.Pop(openSet).(HeapEntry)
		if p.Pos == endPos {
			tile := &p
			for tile != nil {
				m.Path = append(m.Path, tile.Pos)
				tile = tile.parent
			}
			m.smoothPath()
			return true
		}
		m.Blocks[p.Pos.Y][p.Pos.X] = VISITED

		expand := func(newPos Pos, dist float64) {
			status := m.Blocks[newPos.Y][newPos.X]

			distEnd := distance(endPos, newPos)
			distStart := p.DistStart + dist
			switch status {
			case FREE:
				m.Blocks[newPos.Y][newPos.X] = QUEUED
				heap.Push(openSet, HeapEntry{
					Pos:       newPos,
					DistStart: distStart,
					DistEnd:   distEnd,
					parent:    &p,
				})
			case QUEUED:
				for i, elem := range *openSet {
					if elem.Pos == newPos {
						if elem.DistStart > distStart {
							(*openSet)[i].DistStart = distStart
							(*openSet)[i].parent = &p
							heap.Fix(openSet, i)
						}
						break
					}
				}
			default:
				{
				}
			}
		}

		for y := -1; y <= 1; y++ {
			for x := -1; x <= 1; x++ {
				if x == 0 && y == 0 { continue }
				positionToCheck := Pos{p.Pos.X + x, p.Pos.Y + y}
				var dist float64
				if(abs(x * y) == 1){
					dist = m.TILE_DIAGONAL_WIDTH
				} else {
					dist = float64(m.TILE_WIDTH)
				}
				if m.insideMap(positionToCheck.X) && m.insideMap(positionToCheck.Y) {
					expand(positionToCheck, dist)
				}
			}
		}

	}

	return false
}
func (m *Map) GenerateSVG(displayRover, displayBallPadding, displayVisitedTiles, displayQueuedTiles, darkMode bool) string {

	var buff bytes.Buffer

	canvas := svg.New(&buff)

	canvas.Start(int(m.MAP_WIDTH), int(m.MAP_WIDTH))

	if(darkMode){
		canvas.Rect(0,0, m.MAP_WIDTH, m.MAP_WIDTH, "fill:#000000")
	} else {
		canvas.Rect(0,0, m.MAP_WIDTH, m.MAP_WIDTH, "fill:#FFFFFF")
	}
	for _, b := range m.balls {
		if(darkMode){
			canvas.Circle(int(m.TILE_WIDTH*b.Pos.X), int(m.TILE_WIDTH*b.Pos.Y), int(m.BALL_WIDTH/2), "stroke:white;stroke-width:5;" + b.Colour.toString())
		} else {
			canvas.Circle(int(m.TILE_WIDTH*b.Pos.X), int(m.TILE_WIDTH*b.Pos.Y), int(m.BALL_WIDTH/2), "stroke:black;stroke-width:5;" + b.Colour.toString())

		}
	}
	for y := 0; y < len(m.Blocks); y++ {
		for x := 0; x < len(m.Blocks); x++ {
			if m.Blocks[y][x] == QUEUED && displayQueuedTiles{
				canvas.CenterRect(int(m.TILE_WIDTH/2+m.TILE_WIDTH*x), int(m.TILE_WIDTH/2+m.TILE_WIDTH*y), int(m.TILE_WIDTH/2), int(m.TILE_WIDTH/2), `fill:chartreuse;stroke-width:5`)
			} else if m.Blocks[y][x] == VISITED && displayVisitedTiles{
				canvas.CenterRect(int(m.TILE_WIDTH/2+m.TILE_WIDTH*x), int(m.TILE_WIDTH/2+m.TILE_WIDTH*y), int(m.TILE_WIDTH/2), int(m.TILE_WIDTH/2), `fill:#228B22;stroke-width:5`)
			} else if m.Blocks[y][x] == BLOCKED && displayBallPadding{
				canvas.CenterRect(int(m.TILE_WIDTH/2+m.TILE_WIDTH*x), int(m.TILE_WIDTH/2+m.TILE_WIDTH*y), int(m.TILE_WIDTH/2), int(m.TILE_WIDTH/2), `fill:crimson;stroke-width:5`)
			}
		}
	}
	for i := 1; i < len(m.Path); i++ {
		canvas.Line(int(m.TILE_WIDTH/2+m.Path[i-1].X*m.TILE_WIDTH), int(m.TILE_WIDTH/2+m.Path[i-1].Y*m.TILE_WIDTH), int(m.TILE_WIDTH/2+m.Path[i].X*m.TILE_WIDTH), int(m.TILE_WIDTH/2+m.Path[i].Y*m.TILE_WIDTH), `stroke:red;stroke-width:10`)
	}

	if(displayRover){
		canvas.Circle(m.TILE_WIDTH/2 + m.Rover.Pos.X*m.TILE_WIDTH, m.TILE_WIDTH/2 + m.Rover.Pos.Y*m.TILE_WIDTH, 75,`fill:red`)
	}
	canvas.End()
	return buff.String()
}
func (m *Map) DeleteBallByColour(colour Colour) bool{
	for i :=0; i < len(m.balls); i ++ {
		if m.balls[i].Colour == colour {
			m.balls = append(m.balls[:i], m.balls[i+1:]...)
			return true
		}
	}
	return false
}
func (m *Map) SetRoverPosition(x, y int, theta float64){
	m.Rover.Pos.X = x;
	m.Rover.Pos.Y= y;
	m.Rover.Theta = theta;
}
