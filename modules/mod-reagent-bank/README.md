# mod-reagent-bank

Additional storage for crafting reagents — deposit and withdraw trade goods via NPC Ling.

**Fork Maintainer:** Additional maintenance, bug fixes, and improvements by [Ramvaris](https://github.com/Ramvaris) (2025-present)

### Fixes (vs upstream)
- **Withdraw Lag Fix**: Switched from async `Execute()` to synchronous `DirectExecute()` for withdraw operations. Fixes the "1-stack-behind" display bug where the gossip menu showed stale amounts after withdrawing. Safe for multi-player concurrent access because each player's withdraw operates on their own `character_id` rows — no cross-player data races.
