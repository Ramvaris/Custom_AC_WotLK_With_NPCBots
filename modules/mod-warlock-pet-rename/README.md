# mod-warlock-pet-rename

Allows Warlocks to rename their demon pets via a custom NPC.

**Fork Maintainer:** Additional maintenance, bug fixes, and improvements by [Ramvaris](https://github.com/Ramvaris) (2025-present)

### Fixes (vs upstream)
- **Rename Reliability Fix**: Added forced `SMSG_PET_NAME_QUERY_RESPONSE` packet to guarantee the client updates the pet name display immediately. DB write uses async `Execute()` — the prepared statement (`CHAR_UPD_CHAR_PET_NAME`) is registered as `CONNECTION_ASYNC` only, so `DirectExecute` (synchronous) would crash. The name is already applied in-memory via `SetName()` and the SMSG packet updates the client, so async DB persistence is safe.
- **Improved Dialogue**: Replaced ugly `WARLOCKS ONLY` text with proper NPC gossip. Non-warlocks get a polite refusal. NPC now has actual personality.
