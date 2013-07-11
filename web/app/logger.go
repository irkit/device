package app

import (
	"bytes"
	"fmt"
	"log"
	"net/http"
	"strings"
	"time"
)

// borrowed from github.com/hoisie/web/server.go
func logRequest(r *http.Request, sTime time.Time) {
	//log the request
	var logEntry bytes.Buffer
	requestPath := r.URL.Path

	duration := time.Now().Sub(sTime)
	var client string

	// We suppose RemoteAddr is of the form Ip:Port as specified in the Request
	// documentation at http://golang.org/pkg/net/http/#Request
	pos := strings.LastIndex(r.RemoteAddr, ":")
	if pos > 0 {
		client = r.RemoteAddr[0:pos]
	} else {
		client = r.RemoteAddr
	}

	fmt.Fprintf(&logEntry, "%s - \033[32;1m%s %s\033[0m - %v", client, r.Method, requestPath, duration)

	log.Print(logEntry.String())
}

func Log(handler http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		tm := time.Now().UTC()
		defer logRequest(r, tm)
		handler.ServeHTTP(w, r)
	})
}
