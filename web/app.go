package main

import (
	"github.com/hoisie/web"
	"github.com/kayac/irkit/web/app"
	"log"
	"net/http/httputil"
	"os"
)
const host = "0.0.0.0:9999"

func hello(val string) string {
	return "hello " + val
}

func one(c *web.Context, val string) {
	dump, _ := httputil.DumpRequest(c.Request,false)
	c.Server.Logger.Println(string(dump))

	type data struct {
		Title string
		Host string
	}
	d := data{
		"<script>alert('you have been pwned')</script>",
		host,
	}

	context := app.Context{*c}
	context.RenderTemplate("templates/one.template", d)
}

func main() {
	server := web.NewServer()
	server.Get("/apps/one/(.*)", one)
	server.Get("/(.*)",          hello)
	server.SetLogger( log.New(os.Stdout, "app ", log.Ldate|log.Ltime|log.Lshortfile ) )
	server.Run(host)
}
