# mod-warlock-pet-rename

Allows Warlocks to rename their demon pets via a custom NPC.

**Fork Maintainer:** Additional maintenance, bug fixes, and improvements by [Ramvars](https://github.com/Ramvaris) (2025-present)

### Fixes (vs upstream)
- **Rename Reliability Fix**: Added forced `SMSG_PET_NAME_QUERY_RESPONSE` packet to guarantee the client updates the pet name display immediately. Uses `DirectExecute()` for synchronous DB commit. Fixes "rename doesn't work most of the time" where the name was set server-side but the client never queried the update.
- **Improved Dialogue**: Replaced ugly `WARLOCKS ONLY` text with proper NPC gossip. Non-warlocks get a polite refusal. NPC now has actual personality.
