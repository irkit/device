package main

import (
	"github.com/hoisie/web"
	"html/template"
	"bytes"
	"fmt"
	"net/http/httputil"
)
const host = "0.0.0.0:9999"

func hello(val string) string {
	return "hello " + val
}

func one(c *web.Context, val string) string {
	dump, _ := httputil.DumpRequest(c.Request,false)
	fmt.Println(string(dump))

	t, err := template.ParseFiles("templates/one.template")
	if err != nil {
		fmt.Println("err: " + err.Error());
		c.Abort( 500, "Internal Server Error" );
		return "";
	}

	type data struct {
		Title string
		Host string
	}
	d := data{
		"<script>alert('you have been pwned')</script>",
		host,
	}

	buf := new(bytes.Buffer)
	t.ExecuteTemplate(buf, "T", d)

	return buf.String()
}

func main() {
	web.Get("/apps/one/(.*)", one)
	web.Get("/(.*)",          hello)
	web.Run(host)
}
