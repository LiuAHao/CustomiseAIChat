package main

import (
	"context"
	"log"
	"os"
	"os/signal"
	"syscall"
	"time"

	"go-agent-platform/internal/app"
	"go-agent-platform/internal/config"
	httptransport "go-agent-platform/internal/transport/http"
)

func main() {
	cfg := config.Load()
	application := app.New(cfg)
	server := httptransport.New(cfg.HTTPAddr, application)
	ticker := time.NewTicker(cfg.WorkerPollInterval)
	defer ticker.Stop()

	go func() {
		log.Printf("api listening on %s", cfg.HTTPAddr)
		if err := server.Start(); err != nil {
			log.Printf("api stopped: %v", err)
		}
	}()
	go func() {
		for range ticker.C {
			application.RunDueSchedules()
		}
	}()

	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)
	<-sigCh

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	if err := server.Shutdown(ctx); err != nil {
		log.Printf("shutdown error: %v", err)
	}
}
