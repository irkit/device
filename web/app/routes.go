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

	Template{"templates/index.template"}.WriteResponse(w,d)
}

func one(w http.ResponseWriter, r *http.Request) {
	dump, _ := httputil.DumpRequest(r, false)
	log.Println(string(dump))

	d := struct {
		Title string
		Host  string
	} {
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

func writejs(w http.ResponseWriter, r *http.Request) {
	Template{"templates/writejs.template"}.WriteResponse(w, r)
}

func init() {
	http.HandleFunc("/apps/one/write.js", writejs)
	http.HandleFunc("/apps/one/", one)
	http.HandleFunc("/", index)
}
