package main

import (
	"log"
	"net/rpc"
	"github.com/sideshow/apns2"
	"github.com/sideshow/apns2/certificate"
	"github.com/gasbank/laidoff/shared-server"
	"net"
	"errors"
	"encoding/gob"
	"os"
	"io/ioutil"
	"github.com/NaySoftware/go-fcm"
	"net/http"
	"html/template"
	"regexp"
	"fmt"
)

type UserId [16]byte

func registerArith(server *rpc.Server, arith shared_server.Arith, pushService shared_server.PushService) {
	// registers Arith interface by name of `Arithmetic`.
	// If you want this name to be same as the type name, you
	// can use server.Register instead.
	server.RegisterName("Arithmetic", arith)
	server.RegisterName("PushService", pushService)
}

type Arith int

func (t *Arith) Multiply(args *shared_server.Args, reply *int) error {
	*reply = args.A * args.B
	return nil
}

func (t *Arith) Divide(args *shared_server.Args, quo *shared_server.Quotient) error {
	if args.B == 0 {
		return errors.New("divide by zero")
	}
	quo.Quo = args.A / args.B
	quo.Rem = args.A % args.B
	return nil
}

type PushUserData struct {
	UserId [16]byte
	Domain int
	Memo string
	PushToken string
}

type PushServiceData struct {
	PushTokenMap map[string]PushUserData // Push Token --> Push User Data
	UserIdMap    map[UserId]string // User ID --> Push Token
}

func (t *PushServiceData) Add(id UserId, pushToken string, domain int) {
	var idFixed [16]byte
	copy(idFixed[:], id[:])
	t.PushTokenMap[pushToken] = PushUserData{idFixed, domain, "", pushToken}
	t.UserIdMap[idFixed] = pushToken
}

func (t *PushServiceData) DeletePushToken(pushToken string) {
	pushUserData := t.PushTokenMap[pushToken]
	delete(t.PushTokenMap, pushToken)
	delete(t.UserIdMap, pushUserData.UserId)
}

type PushService struct {
	data         PushServiceData
	fcmServerKey string
}

func (t *PushService) RegisterPushToken(args *shared_server.PushToken, reply *int) error {
	log.Printf("Register push token: %v, %v", args.Domain, args.PushToken)
	t.data.Add(args.UserId, args.PushToken, args.Domain)
	log.Printf("pushTokenMap len: %v", len(t.data.PushTokenMap))
	writePushServiceData(&t.data)
	//if args.Domain == 2 { // Android (FCM)
	//	PostAndroidMessage(t.fcmServerKey, args.PushToken)
	//} else if args.Domain == 1 {
	//	PostIosMessage(args.PushToken)
	//}
	*reply = 1
	return nil
}

func (t *PushService) Broadcast(args *shared_server.BroadcastPush, reply *int) error {
	log.Printf("Broadcast: %v", args)
	log.Printf("Broadcasting message to the entire push pool (len=%v)", len(t.data.PushTokenMap))
	for pushToken, pushUserData := range t.data.PushTokenMap {
		domain := pushUserData.Domain
		if domain == 2 {
			go PostAndroidMessage(t.fcmServerKey, pushToken, args.Title, args.Body)
		} else if domain == 1 {
			go PostIosMessage(pushToken, args.Body)
		} else {
			log.Printf("Unknown domain: %v", domain)
			*reply = 0
			return nil
		}
	}
	*reply = 1
	return nil
}

func writePushServiceData(pushServiceData *PushServiceData) {
	pushKeyDbFile, err := os.Create("db/pushkeydb")
	if err != nil {
		log.Fatalf("pushkeydb create failed: %v", err.Error())
	}
	encoder := gob.NewEncoder(pushKeyDbFile)
	encoder.Encode(pushServiceData)
	pushKeyDbFile.Close()
}

func PostAndroidMessage(fcmServerKey string, pushToken string, title string, body string) {
	data := map[string]string{
		"msg": "Hello world!",
		"sum": "HW",
	}
	c := fcm.NewFcmClient(fcmServerKey)
	ids := []string{
		pushToken,
	}
	//xds := []string{
	//	pushToken,
	//}
	c.NewFcmRegIdsMsg(ids, data)
	noti := &fcm.NotificationPayload{
		Title: title,
		Body: body,
		Sound: "default",
		Icon: "ic_stat_name",
	}
	c.SetNotificationPayload(noti)
	//c.AppendDevices(xds)
	status, err := c.Send()
	if err != nil {
		log.Printf("FCM send error: %v", err.Error())
	} else {
		status.PrintResults()
	}
}

type Page struct {
	Title string
	Body  []byte
}

var templates = template.Must(template.ParseFiles("editPushToken.html", "edit.html", "view.html", "push.html", "writePush.html"))
var validPath = regexp.MustCompile("^/(edit|save|view)/([a-zA-Z0-9]+)$")
var validPushTokenPath = regexp.MustCompile("^/(savePushToken|editPushToken|deletePushToken|sendTestPush|writePush|sendPush)/([a-zA-Z0-9-:_]+)$")

func getTitle(w http.ResponseWriter, r *http.Request) (string, error) {
	m := validPath.FindStringSubmatch(r.URL.Path)
	if m == nil {
		http.NotFound(w, r)
		return "", errors.New("invalid page title")
	}
	return  m[2], nil
}

func (p *Page) save() error {
	filename := "pages/" + p.Title + ".txt"
	return ioutil.WriteFile(filename, p.Body, 0600)
}

func loadPage(title string) (*Page, error) {
	filename := "pages/" + title + ".txt"
	body, err := ioutil.ReadFile(filename)
	if err != nil {
		return nil, err
	}
	return &Page{Title: title, Body: body}, nil
}

func renderTemplate(w http.ResponseWriter, tmpl string, p *Page) {
	err := templates.ExecuteTemplate(w, tmpl+".html", p)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func viewHandler(w http.ResponseWriter, r *http.Request, title string) {
	title, err := getTitle(w, r)
	if err != nil {
		return
	}
	p, err := loadPage(title)
	if err != nil {
		http.Redirect(w, r, "/edit/" + title, http.StatusFound)
		return
	}
	renderTemplate(w, "view", p)
}

func editHandler(w http.ResponseWriter, r *http.Request, title string) {
	title, err := getTitle(w, r)
	if err != nil {
		return
	}
	p, err := loadPage(title)
	if err != nil {
		p = &Page{Title: title}
	}
	renderTemplate(w, "edit", p)
}

func saveHandler(w http.ResponseWriter, r *http.Request, title string) {
	title, err := getTitle(w, r)
	if err != nil {
		return
	}
	body := r.FormValue("body")
	p := &Page{Title: title, Body: []byte(body)}
	err = p.save()
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	http.Redirect(w, r, "/view/"+title, http.StatusFound)
}

func makeHandler(fn func (http.ResponseWriter, *http.Request, string)) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		m := validPath.FindStringSubmatch(r.URL.Path)
		if m == nil {
			http.NotFound(w, r)
			return
		}
		fn(w, r, m[2])
	}
}

func pushHandler(w http.ResponseWriter, _ *http.Request, pushService *PushService) {
	err := templates.ExecuteTemplate(w, "push.html", pushService.data)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func deletePushTokenHandler(w http.ResponseWriter, r *http.Request, pushService *PushService, pushToken string) {
	pushService.data.DeletePushToken(pushToken)
	writePushServiceData(&pushService.data)
	http.Redirect(w, r, "/push/", http.StatusFound)
}

func sendTestPushHandler(w http.ResponseWriter, r *http.Request, pushService *PushService, pushToken string) {
	domain := pushService.data.PushTokenMap[pushToken].Domain
	if domain == 2 {
		PostAndroidMessage(pushService.fcmServerKey, pushToken, "용사는 실직중", "어서와.. 테스트 푸시는 처음이지...?")
	} else if domain == 1 {
		PostIosMessage(pushToken, "어서와... 테스트 푸시는 처음이지?")
	} else {
		http.NotFound(w, r)
		return
	}
	http.Redirect(w, r, "/push/", http.StatusFound)
}

func savePushTokenHandler(w http.ResponseWriter, r *http.Request, pushService *PushService, pushToken string) {
	pushUserData := pushService.data.PushTokenMap[pushToken]
	memo := r.FormValue("memo")
	pushUserData.Memo = memo
	pushService.data.PushTokenMap[pushToken] = pushUserData
	writePushServiceData(&pushService.data)
	http.Redirect(w, r, "/push/", http.StatusFound)
}

func sendPushHandler(w http.ResponseWriter, r *http.Request, pushService *PushService, pushToken string) {
	body := r.FormValue("body")
	domain := pushService.data.PushTokenMap[pushToken].Domain
	if domain == 2 {
		PostAndroidMessage(pushService.fcmServerKey, pushToken, "용사는 실직중", body)
	} else if domain == 1 {
		PostIosMessage(pushToken, body)
	} else {
		http.NotFound(w, r)
		return
	}
	http.Redirect(w, r, "/push/", http.StatusFound)
}

func editPushTokenHandler(w http.ResponseWriter, r *http.Request, pushService *PushService, pushToken string) {
	pushUserData := pushService.data.PushTokenMap[pushToken]
	err := templates.ExecuteTemplate(w, "editPushToken.html", pushUserData)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func writePushHandler(w http.ResponseWriter, r *http.Request, pushService *PushService, pushToken string) {
	type WritePushData struct {
		PushToken string
	}
	err := templates.ExecuteTemplate(w, "writePush.html", WritePushData{pushToken})
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
	}
}

func makePushHandler(fn func (http.ResponseWriter, *http.Request, *PushService), pushServiceData *PushService) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		fn(w, r, pushServiceData)
	}
}

func makePushTokenHandler(fn func (http.ResponseWriter, *http.Request, *PushService, string), pushService *PushService) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		m := validPushTokenPath.FindStringSubmatch(r.URL.Path)
		if m == nil {
			http.NotFound(w, r)
			return
		}
		fn(w, r, pushService, m[2])
	}
}

func startAdminService(pushService *PushService) {
	http.HandleFunc("/view/", makeHandler(viewHandler))
	http.HandleFunc("/edit/", makeHandler(editHandler))
	http.HandleFunc("/save/", makeHandler(saveHandler))
	http.HandleFunc("/push/", makePushHandler(pushHandler, pushService))
	http.HandleFunc("/deletePushToken/", makePushTokenHandler(deletePushTokenHandler, pushService))
	http.HandleFunc("/sendTestPush/", makePushTokenHandler(sendTestPushHandler, pushService))
	http.HandleFunc("/writePush/", makePushTokenHandler(writePushHandler, pushService))
	http.HandleFunc("/sendPush/", makePushTokenHandler(sendPushHandler, pushService))
	http.HandleFunc("/editPushToken/", makePushTokenHandler(editPushTokenHandler, pushService))
	http.HandleFunc("/savePushToken/", makePushTokenHandler(savePushTokenHandler, pushService))
	addr := "0.0.0.0:18080"
	log.Printf("Listening %v for admin service...", addr)
	http.ListenAndServe(addr, nil)
}

const (
	ANDROID_KEY_PATH = "cert/dev/fcmserverkey"
	APPLE_KEY_PATH = "cert/dev/cert.p12"
	APPLE_PROD_KEY_PATH = "cert/prod/cert.p12"
)

var prod bool

func main() {
	// Set default log format
	log.SetFlags(log.Lshortfile | log.LstdFlags)
	log.Println("Greetings from push server")
	if len(os.Args) >= 2 && os.Args[1] == "prod" {
		log.Println("PRODUCTION MODE")
		prod = true
	} else {
		prod = false
	}
	// Create db directory to save user database
	os.MkdirAll("db", os.ModePerm)
	os.MkdirAll("pages", os.ModePerm)
	// Load Firebase Cloud Messaging (FCM) server key
	fcmServerKeyBuf, err := ioutil.ReadFile(ANDROID_KEY_PATH)
	if err != nil {
		log.Fatalf("fcm server key load failed: %v", err.Error())
	}
	fcmServerKey := string(fcmServerKeyBuf)
	//Creating an instance of struct which implement Arith interface
	arith := new(Arith)
	pushService := &PushService{
		PushServiceData{
			make(map[string]PushUserData),
			make(map[UserId]string),
		},
		fcmServerKey,
	}
	pushKeyDbFile, err := os.Open("db/pushkeydb")
	if err != nil {
		if os.IsNotExist(err) {
			log.Printf("Push key DB not found.")
		} else {
			log.Fatalf("pushkeydb create failed: %v", err.Error())
		}
	} else {
		defer pushKeyDbFile.Close()
		decoder := gob.NewDecoder(pushKeyDbFile)
		err := decoder.Decode(&pushService.data)
		if err != nil {
			log.Fatalf("pushkeydb decode failed: %v", err.Error())
		}
		log.Printf("pushTokenMap loaded from file. len: %v", len(pushService.data.PushTokenMap))
	}
	go startAdminService(pushService)

	//rpc.Register(arith)

	// Register a new rpc server (In most cases, you will use default server only)
	// And register struct we created above by name "Arith"
	// The wrapper method here ensures that only structs which implement Arith interface
	// are allowed to register themselves.
	server := rpc.NewServer()
	registerArith(server, arith, pushService)

	addr := ":20171"
	log.Printf("Listening %v for push service...", addr)
	// Listen for incoming tcp packets on specified port.
	l, e := net.Listen("tcp", addr)
	if e != nil {
		log.Fatal("listen error:", e)
	}

	// This statement links rpc server to the socket, and allows rpc server to accept
	// rpc request coming from that socket.
	server.Accept(l)
}

func PostIosMessage(pushToken string, body string) {
	var appleKeyPath string
	if prod {
		appleKeyPath = APPLE_PROD_KEY_PATH
	} else {
		appleKeyPath = APPLE_KEY_PATH
	}
	cert, err := certificate.FromP12File(appleKeyPath, "")
	if err != nil {
		log.Fatal("Cert Error:", err)
	}

	notification := &apns2.Notification{}
	notification.DeviceToken = pushToken
	notification.Topic = "com.popsongremix.laidoff"
	notification.Payload = []byte(fmt.Sprintf(`{"aps":{"alert":"%s"}}`, body))

	var client *apns2.Client
	if prod {
		client = apns2.NewClient(cert).Production()
	} else {
		client = apns2.NewClient(cert).Development()
	}
	res, err := client.Push(notification)

	if err != nil {
		log.Fatal("Error:", err)
	}

	log.Printf("Ios push (token %v) result: %v %v %v", pushToken, res.StatusCode, res.ApnsID, res.Reason)
}
