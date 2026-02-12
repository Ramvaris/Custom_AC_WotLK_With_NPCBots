SYSTEM:RYO_RAG_PROTOCOL v2
CONTEXT_MODEL:FIFO — only this file persists. All other context falls off as the window fills.

STEP_0:IDENTITY
On every session start, BEFORE any other action:
  ryo agent prep "<task>" --project .
Then READ .ryo/exports/context_pack.md — it contains your persona, identity, and project context.
Your persona data defines WHO you are, HOW you work, and WHAT you value. Load it. Internalize it. Work from it.

STEP_1:RECALL_OR_PREP
If you need info not visible in context:
  ryo agent recall "<query>" --project .
If starting fresh or context feels stale:
  ryo agent prep "<task>" --project .
Then read .ryo/exports/context_pack.md

STEP_2:WORK
Do the task.

STEP_3:DUMP
After completing work, making decisions, or learning something:
  ryo agent note "<findings>" --scope session --project .
For durable facts (architecture, constraints, rules):
  ryo memory dump --scope project --title "<slug>" --text "<facts>"

LOOP: STEP_1 -> STEP_2 -> STEP_3 -> repeat

DUMP_TRIGGERS — dump immediately when:
- Decision made (architecture, approach, design)
- Bug or constraint discovered
- Task or subtask completed
- About to produce large output (dump understanding first)
- Conversation exceeds ~10 exchanges
- User corrected you or gave new info
- Switching topics or files

RECALL_TRIGGERS — recall immediately when:
- Info needed but not visible in context
- Continuing previous work without details
- Modifying a file not recently read
- User references something from earlier
- Uncertain about any fact or prior decision

MEMORY_SCOPES:
- session: working notes, temp findings (ephemeral)
- project: architecture, decisions, constraints (permanent)
- global: cross-project facts, identity (permanent)
- persona: character behavior rules (permanent)
- chat: RPG/story continuity (permanent)

COMMANDS:
  ryo agent prep "<task>" --project .
  ryo agent prep "<task>" --project . --chat-mode
  ryo agent recall "<query>" --project .
  ryo agent recall "<query>" --project . --json
  ryo agent note "<text>" --scope session --project .
  ryo memory dump --scope project --title <t> --text "<notes>"
  ryo memory dump --scope global --title <t> --text "<notes>"
  ryo memory dump --scope persona --persona <name> --title <t> --text "<notes>"
  ryo memory dump --scope chat --chat-group <group> --title <t> --text "<notes>"
  ryo doctor --project .
  ryo memory catalog --project .
  ryo persona show --project .
  ryo chat show --project .

RULES:
- NEVER say "I'll remember that" — you won't. Dump it.
- NEVER assume prior context is available — recall it.
- NEVER start complex work without prep or recall first.
- NEVER go more than 5 exchanges without a note dump.
- ALWAYS load your persona at session start. It defines you.
- If in doubt: dump first, ask later.
