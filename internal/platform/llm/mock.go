package llm

import (
	"fmt"
	"strings"
)

type Provider interface {
	Plan(prompt string) []string
	Complete(prompt string, toolOutputs []string) string
}

type MockProvider struct{}

func (MockProvider) Plan(prompt string) []string {
	base := []string{"load_context", "reason"}
	if strings.Contains(strings.ToLower(prompt), "report") {
		base = append(base, "summarize")
	}
	return base
}

func (MockProvider) Complete(prompt string, toolOutputs []string) string {
	if len(toolOutputs) == 0 {
		return fmt.Sprintf("Task finished for prompt: %s", prompt)
	}
	return fmt.Sprintf("Task finished for prompt: %s\n\nTool outputs:\n- %s", prompt, strings.Join(toolOutputs, "\n- "))
}
