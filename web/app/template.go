package app

import (
	"bytes"
	"html/template"
	"log"
	"net/http"
)

type Template struct {
	path string
}

func (t Template) WriteResponse(w http.ResponseWriter, data interface{}) {
	buf, err := t.Render(data)
	if err != nil {
		w.WriteHeader(500)
		return
	}

	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	w.Write(buf.Bytes())
}

func (t Template) Render(data interface{}) (*bytes.Buffer, error) {
	buf := new(bytes.Buffer)

	tmpl, err := template.ParseFiles(t.path)
	if err != nil {
		log.Println("err: " + err.Error())
		return buf, err
	}

	err = tmpl.ExecuteTemplate(buf, "T", data)
	if err != nil {
		log.Println("err: " + err.Error())
		return buf, err
	}

	return buf, nil
}
