package main

import (
	"github.com/hoisie/web"
	"html/template"
	"bytes"
	"fmt"
	"os"
	"net/http/httputil"
)

func hello(val string) string {
	return "hello " + val
}

func one(c *web.Context, val string) string {
	dump, _ := httputil.DumpRequest(c.Request,false)
	fmt.Println(string(dump))
	fmt.Println("val: " + val)

	t, err := template.ParseFiles("templates/one.template")
	if err != nil {
		fmt.Println("err: " + err.Error());
		os.Exit(1);
	}

	type Data struct {
		Title string
	}
	data := Data{"<script>alert('you have been pwned')</script>"}

	buf := new(bytes.Buffer)
	t.ExecuteTemplate(buf, "T", data)

	return buf.String()
}

func main() {
	web.Get("/apps/one/(.*)", one)
	web.Get("/(.*)",          hello)
	web.Run("0.0.0.0:9999")
}
