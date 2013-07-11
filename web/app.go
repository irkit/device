package main

import (
	"github.com/hoisie/web"
	"github.com/kayac/irkit/web/app"
	"fmt"
	"net/http/httputil"
)
const host = "0.0.0.0:9999"

func hello(val string) string {
	return "hello " + val
}

func one(c *web.Context, val string) {
	dump, _ := httputil.DumpRequest(c.Request,false)
	fmt.Println(string(dump))

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
	web.Get("/apps/one/(.*)", one)
	web.Get("/(.*)",          hello)
	web.Run(host)
}
