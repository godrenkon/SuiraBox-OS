# Suiram Official Discord Architecture

## Goal

The official Discord should be easy for users and maintainable by a very small team. AI should handle repetitive work without becoming the sole authority over moderation or project decisions.

## Roles

### Human owner / maintainers

- Final authority over bans, major policy changes, releases, and official statements
- Approve actions that can materially affect users or the project
- Manage secrets and platform permissions

### AI bot

- Answer documentation-based questions
- Search and summarize project documentation
- Route support requests
- Detect likely duplicates and suggest existing answers
- Notify users about releases, CI status, incidents, and maintenance
- Generate issue/bug-report templates from conversations
- Help moderate spam and obvious rule violations for human review
- Provide Minecraft/SuiraBox setup guidance based on approved documentation

### Community

- Help each other
- Test builds
- Report bugs
- Propose features
- Create community projects

## AI safety boundary

The bot must not silently make irreversible administrative decisions. Destructive actions, permanent bans, changes to official release information, or changes to permissions require an explicit human approval path.

The bot should prefer answering from the versioned SuiraBox documentation and clearly indicate when it does not know something.

## Channel model

```text
START
├── #welcome
├── #rules
└── #announcements

SUPPORT
├── #support
├── #bug-reports
└── #hardware

SUIRABOX
├── #development
├── #minecraft
├── #minecraft-server
└── #performance

COMMUNITY
├── #showcase
├── #ideas
└── #off-topic
```

## Architecture

```text
Discord
   |
   v
Suiram Bot API
   |
   +-- Documentation search
   +-- FAQ / support responder
   +-- GitHub integration
   +-- Release notifications
   +-- CI notifications
   +-- Moderation queue
   |
   v
Human approval for high-impact actions
```

## Hosting

The bot should be deployable as a containerized application so its hosting provider can be changed without redesigning the bot. Render can be used for development/hobby deployment, but its current free web-service model is not a reliable assumption for a permanently running Discord gateway process: free web services spin down after inactivity and are primarily intended for testing/hobby workloads. Keep the bot stateless and store durable project data outside the service filesystem.

## Cost strategy

Start with:

- GitHub for source, issues, releases, and documentation
- GitHub Pages for the official static website
- A low-cost or free development deployment for the bot where technically suitable
- No paid database dependency until it is actually needed

The bot should work with minimal persistent state so a hosting provider can be replaced later.
