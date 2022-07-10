package main

import (
	"encoding/json"
	"fmt"
	"net/http"
	"os"
	path "path_finding/path_lib"
	"strconv"
)

func main() {
	m := path.MakeMap(5000)
	portStr := os.Getenv("PORT")
	port, err := strconv.Atoi(portStr)
	if err != nil {
		port = 6900
	}

	ballHandler := func(w http.ResponseWriter, req *http.Request) {
		fmt.Println("Add ball")
		var ball path.Ball
		decoder := json.NewDecoder(req.Body)
		err := decoder.Decode(&ball)

		if err != nil {
			fmt.Fprintf(os.Stderr, "Could not demarshal JSON")
			return
		}

		m.PlaceBall(ball)
	}
	generateSVG := func(w http.ResponseWriter, req *http.Request) {
		fmt.Println("generateSVG")
		type SVG_config struct {
			DisplayRover        bool `json:"displayRover"`
			DisplayBallPadding  bool `json:"displayBallPadding"`
			DisplayVisitedTiles bool `json:"displayVisitedTiles"`
			DisplayQueuedTiles  bool `json:"displayQueuedTiles"`
			DarkMode            bool `json:"darkMode"`
		}
		var config SVG_config
		decoder := json.NewDecoder(req.Body)
		err := decoder.Decode(&config)

		if err != nil {
			fmt.Fprintf(os.Stderr, "Could not demarshal JSON")
			return
		}

		type Output struct {
			Out string `json:"out"`
		}
		var output Output

		output.Out = m.GenerateSVG(config.DisplayRover, config.DisplayBallPadding, config.DisplayVisitedTiles, config.DisplayQueuedTiles, config.DarkMode)
		json.NewEncoder(w).Encode(output)
		file, _ := os.Create("output_test.svg")
		file.Write([]byte(output.Out))
	}
	setRoverPosition := func(w http.ResponseWriter, req *http.Request) {
		fmt.Println("setRoverPosition")
		switch req.Method {
		case "POST":
			var rover path.Rover
			decoder := json.NewDecoder(req.Body)
			err := decoder.Decode(&rover)

			if err != nil {
				fmt.Fprintf(os.Stderr, "Could not demarshal JSON")
				return
			}

			m.Rover = rover

		case "GET":
			json.NewEncoder(w).Encode(m.Rover)
		}
	}

	resetCommand := func(w http.ResponseWriter, req *http.Request) {
		fmt.Println("reset")
		m = path.MakeMap(m.MAP_WIDTH)
	}

	findPath := func(w http.ResponseWriter, req *http.Request) {
		fmt.Println("Find Path" )
		fmt.Println(m.MAP_WIDTH_IN_TILES)
		m.Path = []path.Pos{}
		var pos path.Pos
		decoder := json.NewDecoder(req.Body)
		err := decoder.Decode(&pos)

		if err != nil {
			fmt.Fprintf(os.Stderr, "Could not demarshal JSON")
			return
		}
		m.FindPath(m.Rover.Pos, pos)
		fmt.Println(m.Rover.Pos)
		fmt.Println(pos)
		fmt.Println()
		json.NewEncoder(w).Encode(m.Path)
		file, _ := os.Create("found.svg")
		file.Write([]byte(m.GenerateSVG(false, true, true, true, false)))
	}
	deletePath := func(w http.ResponseWriter, req *http.Request) {
		fmt.Println("Delete")
		m.Path = []path.Pos{}
		for y := 0; y < m.MAP_WIDTH; y++ {
			for x := 0; x < m.MAP_WIDTH; x++ {
				m.Blocks[y][x] = path.FREE
			}
		}
	}
	http.HandleFunc("/ball", ballHandler)
	http.HandleFunc("/svg", generateSVG)
	http.HandleFunc("/reset", resetCommand)
	http.HandleFunc("/roverPosition", setRoverPosition)
	http.HandleFunc("/findPath", findPath)
	http.HandleFunc("/deletePath", deletePath)

	http.ListenAndServe(":"+strconv.FormatInt(int64(port), 10), nil)
}
