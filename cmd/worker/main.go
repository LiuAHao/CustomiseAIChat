package main

import (
	"log"
	"time"

	"go-agent-platform/internal/app"
	"go-agent-platform/internal/config"
)

func main() {
	cfg := config.Load()
	application := app.New(cfg)
	ticker := time.NewTicker(cfg.WorkerPollInterval)
	defer ticker.Stop()

	log.Printf("worker polling every %s", cfg.WorkerPollInterval)
	for range ticker.C {
		application.RunDueSchedules()
	}
}
