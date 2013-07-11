package main

import (
	"github.com/kayac/irkit/web/app"
)

func main() {
	server := app.NewServer()
	server.Run(":9999")
}
