package main

import (
	"log"
	"time"

	"go-agent-platform/internal/app"
	"go-agent-platform/internal/config"
)

func main() {
	cfg := config.Load()
	application, err := app.New(cfg)
	if err != nil {
		log.Fatalf("bootstrap app: %v", err)
	}
	defer application.Close()
	ticker := time.NewTicker(cfg.WorkerPollInterval)
	defer ticker.Stop()

	log.Printf("worker polling every %s", cfg.WorkerPollInterval)
	for range ticker.C {
		application.RunDueSchedules()
	}
}
