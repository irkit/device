package app

import (
	"bytes"
	"fmt"
	"github.com/hoisie/web"
	"html/template"
	"io"
)

type Context struct {
	web.Context
}

func(c Context) RenderTemplate(path string, data interface{}) {
	buf := new(bytes.Buffer)

	t, err := template.ParseFiles(path)
	if err != nil {
		fmt.Println("err: " + err.Error())
		c.Abort( 500, "Internal Server Error" )
		return
	}

	err = t.ExecuteTemplate(buf, "T", data)
	if err != nil {
		fmt.Println("err: " + err.Error())
		c.Abort( 500, "Internal Server Error" )
		return
	}
	io.Copy(c, buf)
}
