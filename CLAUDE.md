# Claude Development Guidelines

## Code Style

### Brace Style - ALWAYS USE ALLMAN
**IMPORTANT**: Always use Allman brace style (also known as BSD style) where opening braces go on their own line. This applies to ALL control structures, functions, and blocks.

```c
// Functions
void function_name()
{
    // code
}

// If statements
if (condition)
{
    // code
}
else
{
    // code
}

// Loops
for (int i = 0; i < count; i++)
{
    // code
}

while (condition)
{
    // code
}

// Switch statements
switch (value)
{
    case 1:
    {
        // code
        break;
    }
    default:
    {
        // code
        break;
    }
}

// Structs
struct example
{
    int field;
};
```

Never use K&R style (brace on same line). Always put the opening brace on its own line.

### Section Comments
Use special comment notation `// @section` for searchable sections in code:
- The section name should be short and descriptive
- Typically matches the function prefix
- Examples:
  - `// @file` for file_* operations
  - `// @directory` for directory_* operations  
  - `// @path` for path_* operations
  - `// @init` for initialization functions
  - `// @alloc` for allocation functions
  - `// @types` for type definitions
  - `// @callback` for callback typedefs

This allows quick searching with grep/search for specific sections of code.

## API Design

### Path Functions
- Use `path_t*` instead of `const char*` for all path parameters
- Don't use "get" in function names unnecessarily (e.g., `path_current_directory` not `path_get_current_directory`)

### Memory Management
- Use stack-allocated structs like `path_t`, `name_t`, `text_t` where possible
- These types have fixed-size buffers to avoid dynamic allocation

## Build Commands
- Lint: `npm run lint` (if available)
- Type check: `npm run typecheck` (if available)

## Build System
- **IMPORTANT**: Do NOT run cmake, make, or ninja commands directly
- Development is done in CLion on Windows
- The environment is WSL/Ubuntu but builds are managed through CLion
- Do not attempt to reconfigure or rebuild the project from command line

## STL Usage Policy
- **IMPORTANT**: Do NOT use STL in the main libs/noz library code
- STL is ONLY allowed in the tools directory (libs/noz/tools/*)
- **EXCEPTION**: Files with `// @STL` at the top are allowed to use STL
- The main engine library uses custom containers and types for better control and performance
- Tools (like the importer) can freely use STL since they're not part of the runtime engine