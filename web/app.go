package main

import (
	"github.com/hoisie/web"
	"github.com/kayac/irkit/web/app"
	"io"
	"log"
	"net/http/httputil"
	"os"
)
const host = "0.0.0.0:9999"

func index(c *web.Context, val string) {
	type data struct {
		Name string
	}
	d := data{ val }

	context := app.Context{*c}
	context.WriteTemplate("templates/index.template", d)
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
	buf := context.RenderTemplate("templates/one.template", d)
	io.Copy(c,buf)
}

func main() {
	server := web.NewServer()
	server.Get("/apps/one/(.*)", one)
	server.Get("/(.*)",          index)
	server.SetLogger( log.New(os.Stdout, "app ", log.Ldate|log.Ltime|log.Lshortfile ) )
	server.Run(host)
}
