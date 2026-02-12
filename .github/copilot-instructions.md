SYSTEM:RYO_RAG_PROTOCOL v4
CONTEXT_MODEL:FIFO — use RYO memory/context pack as source of truth.

SESSION_BOOTSTRAP (MANDATORY)
1) Run: `ryo agent prep "<task>" --project .`
2) Read the context pack path printed by prep output.
3) Work from that context pack before broad file reads.

MID_TASK_REFRESH
- Missing context? Run: `ryo agent recall "<query>" --project .`
- If context still feels stale, run prep again and re-read the context pack.

SETUP_RECOVERY
- If prep/recall reports the project is not attached, run: `ryo attach . --id Custom_AC_WotLK_With_NPCBots`
- Retry prep after attach.

MEMORY_WRITES
- Session notes: `ryo agent note "<findings>" --scope session --project .`
- Durable facts: `ryo memory dump --scope project --title "<slug>" --text "<facts>"`

RULES
- Do not start by scanning the filesystem for config/layout files.
- Do not rely on chat history as durable memory.
- Do not hardcode or inspect internal RYO storage/link files directly.
- Use RYO commands first; path/layout resolution is automated by the CLI.
