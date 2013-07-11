package app

import (
	"fmt"
	"log"
	"net/http/httputil"
	"net/http"
	"regexp"
	"strings"
)

const host = "0.0.0.0:9999"

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
		host,
	}

	t := Template{"templates/one.template"}
	buf, err := t.Render(d)
	if err != nil {
		w.WriteHeader(500)
		return
	}
	redirectTo := buf.String()
	redirectTo  = strings.Replace(redirectTo, "\n", "", -1)
	// appengine removes location header if we include multibyte
	// so we replace them with unicode numeric character reference
	re, err    := regexp.Compile(`[^\x00-\xFF]`)
	redirectTo  = re.ReplaceAllStringFunc(redirectTo, unicodeReference)
	redirectTo  = "data:text/html;charset=UTF-8," + redirectTo
	http.Redirect(w, r, redirectTo, 302)
}

func unicodeReference(s string) string {
	runes := []rune(s)
	return fmt.Sprintf("&#x%X;", runes[0])
}

func init() {
	http.HandleFunc("/apps/one/", one)
	http.HandleFunc("/", index)
}
