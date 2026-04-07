package app

import (
	"errors"
	"strings"
	"time"

	"go-agent-platform/internal/domain/shared"
	"go-agent-platform/internal/domain/skill"
)

type CreateSkillRequest struct {
	WorkspaceID string         `json:"workspace_id"`
	Name        string         `json:"name"`
	Slug        string         `json:"slug"`
	Scope       string         `json:"scope"`
	Description string         `json:"description"`
	Version     string         `json:"version"`
	Entry       string         `json:"entry"`
	Schema      map[string]any `json:"schema"`
	Config      map[string]any `json:"config"`
	Enabled     bool           `json:"enabled"`
}

type UpdateSkillRequest struct {
	Name        string         `json:"name"`
	Slug        string         `json:"slug"`
	Description string         `json:"description"`
	Version     string         `json:"version"`
	Entry       string         `json:"entry"`
	Schema      map[string]any `json:"schema"`
	Config      map[string]any `json:"config"`
	Enabled     *bool          `json:"enabled"`
}

func (a *Application) CreateSkill(userID string, req CreateSkillRequest) (skill.Skill, error) {
	workspaceID, err := a.resolveWorkspaceID(userID, req.WorkspaceID)
	if err != nil {
		return skill.Skill{}, err
	}
	if ok, err := a.userInWorkspace(userID, workspaceID); err != nil || !ok {
		return skill.Skill{}, errors.New("workspace access denied")
	}

	name := strings.TrimSpace(req.Name)
	if name == "" {
		return skill.Skill{}, errors.New("skill name is required")
	}
	scope := normalizeSkillScope(req.Scope)
	if scope == skill.ScopePlatform && !a.isPlatformAdmin(userID) {
		return skill.Skill{}, errors.New("only platform admin can create platform skill")
	}

	now := time.Now().UTC()
	item := skill.Skill{
		ID:          shared.NewID("skill"),
		WorkspaceID: workspaceID,
		Name:        name,
		Slug:        firstNonEmpty(strings.TrimSpace(req.Slug), slugify(name)),
		Scope:       scope,
		Description: strings.TrimSpace(req.Description),
		Version:     firstNonEmpty(strings.TrimSpace(req.Version), "0.1.0"),
		Entry:       strings.TrimSpace(req.Entry),
		Schema:      defaultMap(req.Schema),
		Config:      defaultMap(req.Config),
		Enabled:     true,
		CreatedBy:   userID,
		CreatedAt:   now,
		UpdatedAt:   now,
	}
	if err := a.Store.SaveSkill(item); err != nil {
		return skill.Skill{}, err
	}
	a.appendAudit(workspaceID, userID, "skill.create", "skill", item.ID, map[string]any{"name": item.Name, "scope": item.Scope})
	return item, nil
}

func (a *Application) DeleteSkill(userID, workspaceID, skillID string) error {
	if strings.TrimSpace(workspaceID) == "" {
		item, err := a.Store.FindSkillByID(skillID)
		if err != nil {
			return errors.New("skill not found")
		}
		workspaceID = item.WorkspaceID
	}
	if ok, err := a.userInWorkspace(userID, workspaceID); err != nil || !ok {
		return errors.New("workspace access denied")
	}
	if err := a.Store.DeleteSkill(workspaceID, skillID); err != nil {
		return err
	}
	a.appendAudit(workspaceID, userID, "skill.delete", "skill", skillID, nil)
	return nil
}

func (a *Application) UpdateSkill(userID, skillID string, req UpdateSkillRequest) (skill.Skill, error) {
	item, err := a.Store.FindSkillByID(skillID)
	if err != nil {
		return skill.Skill{}, errors.New("skill not found")
	}
	if ok, err := a.userInWorkspace(userID, item.WorkspaceID); err != nil || !ok {
		return skill.Skill{}, errors.New("workspace access denied")
	}

	if value := strings.TrimSpace(req.Name); value != "" {
		item.Name = value
	}
	if value := strings.TrimSpace(req.Slug); value != "" {
		item.Slug = value
	}
	if item.Scope == "" {
		item.Scope = skill.ScopePersonal
	}
	if req.Description != "" {
		item.Description = strings.TrimSpace(req.Description)
	}
	if req.Version != "" {
		item.Version = strings.TrimSpace(req.Version)
	}
	if req.Entry != "" {
		item.Entry = strings.TrimSpace(req.Entry)
	}
	if req.Schema != nil {
		item.Schema = req.Schema
	}
	if req.Config != nil {
		item.Config = req.Config
	}
	if req.Enabled != nil {
		item.Enabled = *req.Enabled
	}
	item.UpdatedAt = time.Now().UTC()

	if err := a.Store.UpdateSkill(item); err != nil {
		return skill.Skill{}, err
	}
	a.appendAudit(item.WorkspaceID, userID, "skill.update", "skill", item.ID, map[string]any{"enabled": item.Enabled})
	return item, nil
}

func (a *Application) ListSkills(workspaceID string) []skill.Skill {
	items, err := a.Store.ListSkills(workspaceID)
	if err != nil {
		return []skill.Skill{}
	}
	return items
}
