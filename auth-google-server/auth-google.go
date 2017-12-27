package main

import (
	"crypto/rsa"
	"log"
	"encoding/json"
	"encoding/pem"
	"crypto/x509"
	"net/http"
	"github.com/gasbank/laidoff/auth-google-server/jws"
	"os"
	"fmt"
	"io/ioutil"
)

const (
	ServiceName = "auth-google"
	ServiceAddr = ":10622"
	CertFile    = "key/popsongremix_com.cer"
	KeyFile     = "key/popsongremix_com.pem"
)

func main() {
	// Set default log format
	log.SetFlags(log.LstdFlags | log.Lshortfile)
	log.SetOutput(os.Stdout)
	log.Printf("Greetings from %v service", ServiceName)
	http.HandleFunc("/auth-google", handleAuthGoogle)
	err := http.ListenAndServeTLS(ServiceAddr, CertFile, KeyFile, nil)
	if err != nil {
		log.Fatal("ListenAndServe: ", err)
	}
}
func handleAuthGoogle(_ http.ResponseWriter, request *http.Request) {
	defer request.Body.Close()
	body, err := ioutil.ReadAll(request.Body)
	if err != nil {
		panic(err)
	}
	idToken := string(body)
	log.Printf("ID Token posted: %v", idToken)
	validate(idToken)
}

func validate(idToken string) {
	res, err := http.Get("https://www.googleapis.com/oauth2/v1/certs")
	if err != nil {
		log.Fatal(err)
		return
	}
	defer res.Body.Close()
	var certs map[string]string
	dec := json.NewDecoder(res.Body)
	dec.Decode(&certs)
	// add error checking
	for googleCertKey, googleCert := range certs {
		blockPub, _ := pem.Decode([]byte(googleCert))
		certInterface, err := x509.ParseCertificates(blockPub.Bytes)
		pubKey := certInterface[0].PublicKey.(*rsa.PublicKey)
		log.Printf("Verifying token with cert key %v...", googleCertKey)
		err = jws.Verify(idToken, pubKey)
		if err != nil {
			log.Printf("Verify error: %v", err.Error())
		} else {
			log.Printf("Valid token.")
			claimSet, err := jws.Decode(idToken)
			if err != nil {
				log.Printf("Decode error: %v", err.Error())
			} else {
				claimSetStrBytes, err := json.MarshalIndent(claimSet, "", "  ")
				if err != nil {
					fmt.Println("error:", err)
				}
				log.Printf(string(claimSetStrBytes))

			}
			break
		}
	}
}
