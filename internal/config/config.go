package config

import (
	"os"
	"strconv"
	"time"
)

type Config struct {
	AppName           string
	HTTPAddr          string
	WorkerPollInterval time.Duration
	JWTSecret         string
	SeedAdminEmail    string
	SeedAdminPassword string
	StorageDriver     string
}

func Load() Config {
	return Config{
		AppName:            env("APP_NAME", "go-agent-platform"),
		HTTPAddr:           env("HTTP_ADDR", ":8081"),
		WorkerPollInterval: envDuration("WORKER_POLL_INTERVAL", 5*time.Second),
		JWTSecret:          env("JWT_SECRET", "dev-secret"),
		SeedAdminEmail:     env("SEED_ADMIN_EMAIL", "admin@example.com"),
		SeedAdminPassword:  env("SEED_ADMIN_PASSWORD", "ChangeMe123!"),
		StorageDriver:      env("STORAGE_DRIVER", "memory"),
	}
}

func env(key, fallback string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return fallback
}

func envDuration(key string, fallback time.Duration) time.Duration {
	if v := os.Getenv(key); v != "" {
		if d, err := time.ParseDuration(v); err == nil {
			return d
		}
	}
	return fallback
}

func envInt(key string, fallback int) int {
	if v := os.Getenv(key); v != "" {
		if n, err := strconv.Atoi(v); err == nil {
			return n
		}
	}
	return fallback
}
