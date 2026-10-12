# AGENTS.md

## Regression Prevention
**NEVER** introduce breaking changes without explicit approval
**NEVER** modify existing public APIs unless specifically requested
**NEVER** change existing test assertions or remove tests without approval
**NEVER** modify CI/build configuration without explicit request
**ALWAYS** run the complete test suite before committing changes
**ALWAYS** test for NULL pointers and segfaults.

## Code Change Philosophy
**Prefer addition over modification**: Add new functionality alongside existing code
**Preserve existing behavior**: Do not change current behavior unless asked to
**No "cowboy-style" refactoring**: Avoid large-scale structural changes
**Conservative by default**: Only make changes that directly address the specific request

## Comments
Do not comment code, preserve existing comments

## File System
src/ - source files with all the logic
include/ - headerfiles with macros and function definition

## code style
Short and verbose.
Function names need to be easly understanbable.
Use prefer to expand current systems rather than making new ones.
Make sure the code doesnt contain NULL pointers and segfaults.

