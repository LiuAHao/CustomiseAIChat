export type AppIconKey =
  | 'create-agent'
  | 'skills'
  | 'mcp'
  | 'models'
  | 'settings'
  | 'person'
  | 'collapse'
  | 'expand'
  | 'chat'

type AppIconProps = {
  kind: AppIconKey
}

export function AppIcon({ kind }: AppIconProps) {
  switch (kind) {
    case 'create-agent':
      return (
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <path d="M4 16.5V20h3.5L18 9.5 14.5 6 4 16.5Z" />
          <path d="M13.5 7L17 10.5" />
        </svg>
      )
    case 'skills':
      return (
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <path d="M13 3 5 14h5l-1 7 8-11h-5l1-7Z" />
        </svg>
      )
    case 'mcp':
      return (
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <rect x="4" y="6" width="6" height="12" rx="2" />
          <rect x="14" y="6" width="6" height="12" rx="2" />
          <path d="M10 12h4" />
        </svg>
      )
    case 'models':
      return (
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <path d="m12 3 1.3 3.7L17 8l-3.7 1.3L12 13l-1.3-3.7L7 8l3.7-1.3L12 3Z" />
          <path d="m18 14 .8 2.2L21 17l-2.2.8L18 20l-.8-2.2L15 17l2.2-.8L18 14Z" />
          <path d="m6 14 .8 2.2L9 17l-2.2.8L6 20l-.8-2.2L3 17l2.2-.8L6 14Z" />
        </svg>
      )
    case 'settings':
      return (
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <circle cx="12" cy="12" r="3.2" />
          <path d="M12 2.8v2.1" />
          <path d="M12 19.1v2.1" />
          <path d="m4.9 4.9 1.5 1.5" />
          <path d="m17.6 17.6 1.5 1.5" />
          <path d="M2.8 12h2.1" />
          <path d="M19.1 12h2.1" />
          <path d="m4.9 19.1 1.5-1.5" />
          <path d="m17.6 6.4 1.5-1.5" />
        </svg>
      )
    case 'collapse':
      return (
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <path d="M8 6v12" />
          <path d="m16 8-4 4 4 4" />
        </svg>
      )
    case 'expand':
      return (
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <path d="M16 6v12" />
          <path d="m8 8 4 4-4 4" />
        </svg>
      )
    case 'chat':
      return (
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <path d="M7 18.5 3.5 20l1-3.7A7.5 7.5 0 1 1 7 18.5Z" />
          <path d="M8.5 10.5h7M8.5 13.5h4.5" />
        </svg>
      )
    default:
      return (
        <svg viewBox="0 0 24 24" aria-hidden="true">
          <circle cx="12" cy="8" r="3.2" />
          <path d="M6.5 19a5.5 5.5 0 0 1 11 0" />
        </svg>
      )
  }
}
