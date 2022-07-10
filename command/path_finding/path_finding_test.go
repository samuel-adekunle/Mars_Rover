package main

import (
	"fmt"
	"math/rand"
	"os"
	path "path_finding/path_lib"
	"testing"
	"time"
)
func TestSetupMap(t *testing.T) {
	width := 2000
	m, startY, endY, startX, endX := setupMap(width,int(float64(width * width) * 2.3 * 0.00001) )
	m.FindPath(path.Pos{startX, startY}, path.Pos{endX, endY})
	outputFile, _ := os.Create("generate.svg")
	outputFile.Write([]byte(m.GenerateSVG(false, true, true, true, true)))
}
func BenchmarkSetupMap(b *testing.B) {
	for i:= 2000; i < 30000; i+=100 {
		b.Run("Layers" + fmt.Sprint(i), func(b * testing.B) {
			for n := 0; n < b.N; n++ {
				b.StopTimer()
				m, startY, endY, _, _ := setupMap(i,int(float64(i * i) * 2.0 * 0.00001) )
				b.StartTimer()
				val := m.FindPath(path.Pos{m.MAP_WIDTH_IN_TILES/2, startY/2}, path.Pos{m.MAP_WIDTH_IN_TILES/2, endY})
				b.StopTimer()
				if(!val){
					panic("Could not determine path!!");
				}
			}
		})

	}
}
func BenchmarkDensity(b *testing.B) {
	for i:= 1000; i <=4000; i+=10 {
		b.Run("Layers" + fmt.Sprint(i), func(b * testing.B) {
			for n := 0; n < b.N; n++ {
				b.StopTimer()
				m, startY, endY, _, _ := setupMap(20000,i )
				b.StartTimer()
				val := m.FindPath(path.Pos{m.MAP_WIDTH_IN_TILES/2, startY/2}, path.Pos{m.MAP_WIDTH_IN_TILES/2, endY})
				b.StopTimer()
				if(!val){
					panic("Could not determine path!!");
				}
			}
		})
	}
}
func setupMap(mapWidth, balls int) (m path.Map, startY ,endY, startX, endX int){
	m = path.MakeMap(mapWidth)
	rand.Seed(time.Now().UTC().UnixNano())
	colourDiff := path.Colour(0xFFFFFF / (balls))
	colour := path.Colour(0)
	boundaryFractionHoz := 0.05
	boundaryFractionVert := 0.05

	startY = int(float64(m.MAP_WIDTH_IN_TILES) * boundaryFractionVert)
	endY =  int(float64(m.MAP_WIDTH_IN_TILES) * (1- boundaryFractionVert))
	startX = int(float64(m.MAP_WIDTH_IN_TILES) * (1-boundaryFractionHoz))
	endX = int(float64(m.MAP_WIDTH_IN_TILES) * boundaryFractionHoz)
	extraPadding := 0.05
	for i := 0; i < balls; i++ {
		ballX := rand.Intn(int(float64(m.MAP_WIDTH_IN_TILES)*(1-2 * (boundaryFractionHoz + extraPadding))))
		ballY := rand.Intn(int(float64(m.MAP_WIDTH_IN_TILES)*(1-2 * (boundaryFractionHoz + extraPadding))))
		m.PlaceBall(path.Ball{
			Pos: path.Pos{int(float64(m.MAP_WIDTH_IN_TILES)*(boundaryFractionHoz+extraPadding)) + ballX,int(float64(m.MAP_WIDTH_IN_TILES)*(boundaryFractionVert+extraPadding)) + ballY},
			Colour: colour,
		})
		colour += colourDiff
	}
	return

}
func TestFunctionality(t * testing.T) {
	m := path.MakeMap(2000)
	m.PlaceBall(path.Ball{
		Pos:    path.Pos{10, 10},
		Colour: 0xFF0000,
	})
	m.PlaceBall(path.Ball{
		Pos:    path.Pos{10, 18},
		Colour: 0x00FF00,
	})
	m.PlaceBall(path.Ball{
		Pos:    path.Pos{2, 30},
		Colour: 0x0000FF,
	})
	m.PlaceBall(path.Ball{
		Pos:    path.Pos{20, 10},
		Colour: 0xFF00FF,
	})
	m.PlaceBall(path.Ball{
		Pos:    path.Pos{2, 8},
		Colour: 0xFFFF00,
	})
	m.PlaceBall(path.Ball{
		Pos:    path.Pos{15, 26},
		Colour: 0xFFFF,
	})
	m.PlaceBall(path.Ball{
		Pos:    path.Pos{40, 26},
		Colour: 0x6,
	})
	m.PlaceBall(path.Ball{
		Pos:    path.Pos{20, 23},
		Colour: 0x8,
	})
	m.PlaceBall(path.Ball{
		Pos:    path.Pos{23, 23},
		Colour: 0xFF,
	})
	m.PlaceBall(path.Ball{
		Pos:    path.Pos{31, 27},
		Colour: 0x7,
	})
	
	m.FindPath(path.Pos{2, 20}, path.Pos{20, 13})
	file, _ := os.Create("found.svg")
	file.Write([]byte(m.GenerateSVG(false,true, true, true,true)))
}
