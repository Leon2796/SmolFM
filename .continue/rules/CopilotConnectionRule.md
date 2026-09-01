# Copilot Connection Rule

This rule applies to all code generation and file edits in the current repository.

1. Source of Truth: Check if you are in git repository folder. Use the instructions defined in `./github/copilot-instructions.md` defined in the root directory of the repository
as authoritative for all code generation and updates.
2. Scope: All operations regarding file modification (create_new_file, edit_existing_file, etc.).
3. Behavior: Use git command "git rev-parse --is-inside-work-tree" to check if you need to check if you are in git repository.