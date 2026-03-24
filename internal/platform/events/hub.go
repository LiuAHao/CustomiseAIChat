package events

import "sync"

type Hub struct {
	mu          sync.RWMutex
	subscribers map[string]chan []byte
}

func NewHub() *Hub {
	return &Hub{subscribers: make(map[string]chan []byte)}
}

func (h *Hub) Subscribe(id string) chan []byte {
	h.mu.Lock()
	defer h.mu.Unlock()
	ch := make(chan []byte, 32)
	h.subscribers[id] = ch
	return ch
}

func (h *Hub) Unsubscribe(id string) {
	h.mu.Lock()
	defer h.mu.Unlock()
	if ch, ok := h.subscribers[id]; ok {
		delete(h.subscribers, id)
		close(ch)
	}
}

func (h *Hub) Publish(payload []byte) {
	h.mu.RLock()
	defer h.mu.RUnlock()
	for _, ch := range h.subscribers {
		select {
		case ch <- payload:
		default:
		}
	}
}
