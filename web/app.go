package main

// .prefix hides this file from appengine

import (
	"log"
	"net/http"
	_ "github.com/kayac/irkit/web/app"
)

func main() {
	log.Println("start listening on :8080")
	log.Fatal(http.ListenAndServe(":8080",nil))
}
