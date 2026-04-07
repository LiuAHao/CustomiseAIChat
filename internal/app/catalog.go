package app

import (
	"errors"
	"slices"
	"strings"

	"go-agent-platform/internal/domain/session"
	"go-agent-platform/internal/domain/skill"
	"go-agent-platform/internal/domain/task"
	"go-agent-platform/internal/domain/tool"
	"go-agent-platform/internal/domain/workspace"
)

type CatalogResult[T any] struct {
	PlatformItems []T `json:"platform_items"`
	MyItems       []T `json:"my_items"`
}

func (a *Application) resolveWorkspaceID(userID, workspaceID string) (string, error) {
	trimmed := strings.TrimSpace(workspaceID)
	if trimmed != "" {
		return trimmed, nil
	}
	items := a.ListWorkspaces(userID)
	if len(items) == 0 {
		return "", errors.New("no workspace available")
	}
	return items[0].ID, nil
}

func (a *Application) ResolveWorkspaceID(userID, workspaceID string) (string, error) {
	return a.resolveWorkspaceID(userID, workspaceID)
}

func (a *Application) resolveAgentWorkspace(userID, workspaceID, agentID string) (string, error) {
	if trimmed := strings.TrimSpace(workspaceID); trimmed != "" {
		return trimmed, nil
	}
	if trimmedAgentID := strings.TrimSpace(agentID); trimmedAgentID != "" {
		item, err := a.Store.FindAgentByID(trimmedAgentID)
		if err == nil && item.ID != "" {
			return item.WorkspaceID, nil
		}
	}
	return a.resolveWorkspaceID(userID, workspaceID)
}

func (a *Application) isPlatformAdmin(userID string) bool {
	user, err := a.Store.FindUserByID(userID)
	if err != nil {
		return false
	}
	return strings.EqualFold(strings.TrimSpace(user.Email), strings.TrimSpace(a.Config.SeedAdminEmail))
}

func normalizeSkillScope(scope string) string {
	if strings.EqualFold(strings.TrimSpace(scope), skill.ScopePlatform) {
		return skill.ScopePlatform
	}
	return skill.ScopePersonal
}

func normalizeToolScope(scope string) string {
	if strings.EqualFold(strings.TrimSpace(scope), tool.ScopePlatform) {
		return tool.ScopePlatform
	}
	return tool.ScopePersonal
}

func (a *Application) ListSkillCatalog(userID string) CatalogResult[skill.Skill] {
	platformItems, _ := a.Store.ListPlatformSkills()
	personalItems, _ := a.Store.ListUserSkills(userID)
	installedIDs, _ := a.Store.ListInstalledSkillIDs(userID)

	myItems := append([]skill.Skill{}, personalItems...)
	for _, item := range platformItems {
		if slices.Contains(installedIDs, item.ID) {
			myItems = append(myItems, item)
		}
	}

	return CatalogResult[skill.Skill]{
		PlatformItems: platformItems,
		MyItems:       myItems,
	}
}

func (a *Application) ListToolCatalog(userID string) CatalogResult[tool.Tool] {
	platformItems, _ := a.Store.ListPlatformTools()
	personalItems, _ := a.Store.ListUserTools(userID)
	installedIDs, _ := a.Store.ListInstalledToolIDs(userID)

	myItems := append([]tool.Tool{}, personalItems...)
	for _, item := range platformItems {
		if slices.Contains(installedIDs, item.ID) {
			myItems = append(myItems, item)
		}
	}

	return CatalogResult[tool.Tool]{
		PlatformItems: platformItems,
		MyItems:       myItems,
	}
}

func (a *Application) InstallSkill(userID, skillID string) error {
	item, err := a.Store.FindSkillByID(skillID)
	if err != nil {
		return errors.New("skill not found")
	}
	if item.Scope != skill.ScopePlatform {
		return errors.New("only platform skill can be installed")
	}
	return a.Store.InstallSkill(userID, skillID)
}

func (a *Application) UninstallSkill(userID, skillID string) error {
	return a.Store.UninstallSkill(userID, skillID)
}

func (a *Application) InstallTool(userID, toolID string) error {
	item, err := a.Store.FindToolByID(toolID)
	if err != nil {
		return errors.New("mcp not found")
	}
	if item.Scope != tool.ScopePlatform {
		return errors.New("only platform mcp can be installed")
	}
	return a.Store.InstallTool(userID, toolID)
}

func (a *Application) UninstallTool(userID, toolID string) error {
	return a.Store.UninstallTool(userID, toolID)
}

func (a *Application) skillAccessible(userID string, item skill.Skill) bool {
	if item.Scope == skill.ScopePersonal {
		return item.CreatedBy == userID
	}
	installedIDs, _ := a.Store.ListInstalledSkillIDs(userID)
	return slices.Contains(installedIDs, item.ID)
}

func (a *Application) toolAccessible(userID string, item tool.Tool) bool {
	if item.Scope == tool.ScopePersonal {
		return item.CreatedBy == userID
	}
	installedIDs, _ := a.Store.ListInstalledToolIDs(userID)
	return slices.Contains(installedIDs, item.ID)
}

func (a *Application) ListSessionsByAgent(userID, agentID string) []session.Session {
	items, err := a.Store.ListSessionsByAgent(userID, agentID)
	if err != nil {
		return []session.Session{}
	}
	return items
}

func (a *Application) ListTasksBySession(sessionID string) []task.Task {
	items, err := a.Store.ListTasksBySession(sessionID)
	if err != nil {
		return []task.Task{}
	}
	return items
}

func (a *Application) DefaultMembership(userID string) (workspace.Membership, bool) {
	workspaces := a.ListWorkspaces(userID)
	if len(workspaces) == 0 {
		return workspace.Membership{}, false
	}
	return workspace.Membership{WorkspaceID: workspaces[0].ID, UserID: userID}, true
}
