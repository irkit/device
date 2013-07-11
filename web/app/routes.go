package app

import (
	"log"
	"net/http/httputil"
	"net/http"
	"strings"
)

func index(w http.ResponseWriter, r *http.Request) {
	type data struct {
		Name string
	}
	d := data{r.URL.Path}

	t := Template{"templates/index.template"}
	t.WriteResponse(w, d)
}

func one(w http.ResponseWriter, r *http.Request) {
	dump, _ := httputil.DumpRequest(r, false)
	log.Println(string(dump))

	type data struct {
		Title string
		Host  string
	}
	d := data{
		"<script>alert('you have been pwned')</script>",
		r.Host,
	}

	t := Template{"templates/one.template"}
	buf, err := t.Render(d)
	if err != nil {
		w.WriteHeader(500)
		return
	}
	redirectTo := buf.String()
	redirectTo  = strings.Replace(redirectTo, "\n", "", -1)
	redirectTo  = "data:text/html;charset=UTF-8," + redirectTo
	http.Redirect(w, r, redirectTo, 302)
}

func write(w http.ResponseWriter, r *http.Request) {
	type data struct {
		Host string
	}
	d := data{r.Host}

	t := Template{"templates/write.template"}
	t.WriteResponse(w, d)
}

func init() {
	http.HandleFunc("/apps/one/write.js", write)
	http.HandleFunc("/apps/one/", one)
	http.HandleFunc("/", index)
}
