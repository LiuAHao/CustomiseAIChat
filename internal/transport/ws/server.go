package ws

import (
	"bufio"
	"crypto/sha1"
	"encoding/base64"
	"fmt"
	"net"
	"net/http"
	"strings"

	"go-agent-platform/internal/platform/events"
)

type Server struct {
	hub *events.Hub
}

func New(hub *events.Hub) *Server {
	return &Server{hub: hub}
}

func (s *Server) Handle(w http.ResponseWriter, r *http.Request) {
	if !headerContains(r.Header, "Connection", "Upgrade") || !headerContains(r.Header, "Upgrade", "websocket") {
		http.Error(w, "websocket upgrade required", http.StatusBadRequest)
		return
	}
	key := r.Header.Get("Sec-WebSocket-Key")
	if key == "" {
		http.Error(w, "missing websocket key", http.StatusBadRequest)
		return
	}
	hj, ok := w.(http.Hijacker)
	if !ok {
		http.Error(w, "hijacking not supported", http.StatusInternalServerError)
		return
	}
	conn, rw, err := hj.Hijack()
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	defer conn.Close()

	accept := websocketAccept(key)
	_, _ = rw.WriteString("HTTP/1.1 101 Switching Protocols\r\n")
	_, _ = rw.WriteString("Upgrade: websocket\r\n")
	_, _ = rw.WriteString("Connection: Upgrade\r\n")
	_, _ = rw.WriteString("Sec-WebSocket-Accept: " + accept + "\r\n\r\n")
	_ = rw.Flush()

	subID := r.RemoteAddr + "|" + r.URL.RawQuery
	ch := s.hub.Subscribe(subID)
	defer s.hub.Unsubscribe(subID)

	for payload := range ch {
		if err := writeTextFrame(conn, payload); err != nil {
			return
		}
	}
}

func websocketAccept(key string) string {
	h := sha1.New()
	_, _ = h.Write([]byte(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"))
	return base64.StdEncoding.EncodeToString(h.Sum(nil))
}

func writeTextFrame(conn net.Conn, payload []byte) error {
	header := []byte{0x81}
	length := len(payload)
	switch {
	case length < 126:
		header = append(header, byte(length))
	case length < 65536:
		header = append(header, 126, byte(length>>8), byte(length))
	default:
		return fmt.Errorf("payload too large")
	}
	if _, err := conn.Write(header); err != nil {
		return err
	}
	_, err := conn.Write(payload)
	return err
}

func headerContains(headers http.Header, key, want string) bool {
	reader := bufio.NewReader(strings.NewReader(strings.ToLower(headers.Get(key))))
	line, _ := reader.ReadString('\n')
	return strings.Contains(line, strings.ToLower(want))
}
