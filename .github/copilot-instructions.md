SYSTEM:RYO_RAG_PROTOCOL v7
CONTEXT_MODEL:FIFO — this file is always attached, so treat it as your persistent operating contract.

SESSION_BOOTSTRAP (MANDATORY)
1) Run once per task start: `ryo agent prep "<task>" --project .`
2) Read the context pack path printed by prep output (once for that prep run).
3) Use that pack as baseline context; do NOT re-open it after every recall.

MID_TASK_REFRESH
- Missing context? Run: `ryo agent recall "<query>" --project .` and use recall output directly.
- If context still feels stale after recall, run prep again and read the newly generated context pack once.

SETUP_RECOVERY
- If prep/recall reports the project is not attached, run: `ryo attach . --id Custom_AC_WotLK_With_NPCBots`
- Retry prep after attach.

MEMORY_SCOPES
- session: temporary worklog/findings for current task progression.
- project: durable architecture/constraints/decisions for this project.
- global: durable cross-project truths.
- persona: durable behavior/identity rules for a specific persona.
- chat: durable continuity for chat/RPG/group contexts.

MEMORY_WRITES
- Session notes: `ryo agent note "<findings>" --scope session --project .`
- Durable facts: `ryo memory dump --scope project --title "<slug>" --text "<facts>"`

DUMP_TRIGGERS
- Decision made (architecture/design/approach)
- Bug/constraint discovered
- Task/subtask completed
- Context feels crowded or chat is long
- Topic/file switch

RECALL_TRIGGERS
- Missing fact needed for current step
- Continuing prior work with partial context
- Uncertain about prior decisions/constraints

COMMANDS (CANONICAL)
- `ryo agent prep "<task>" --project .`
- `ryo agent recall "<query>" --project .`
- `ryo agent note "<findings>" --scope session --project .`
- `ryo memory dump --scope project --title "<slug>" --text "<facts>" --project .`
- `ryo memory dump --scope global --title "<slug>" --text "<facts>" --project .`
- `ryo memory dump --scope persona --persona <name> --title "<slug>" --text "<facts>" --project .`
- `ryo memory dump --scope chat --chat-group <group> --title "<slug>" --text "<facts>" --project .`
- `ryo memory catalog --project .`
- `ryo doctor --project .`

RULES
- Do not start by scanning the filesystem for config/layout files.
- Do not rely on chat history as durable memory.
- Do not hardcode or inspect internal RYO storage/link files directly.
- Do not open raw RAG markdown (`context_pack.md`, memory markdown dumps, launcher markdown) unless the user explicitly requests file-level inspection.
- Avoid direct browsing of `/home/ramires/projects/RYO/*` for context; use `ryo agent prep/recall` output first.
- Use RYO commands first; path/layout resolution is automated by the CLI.
- Never claim memory persistence without writing a note/dump.
