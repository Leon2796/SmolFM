# Instructions

## Coding Style

- Do **not** use `auto`. Always write explicit types so the code is readable at a glance.

## Assertions

`jassert` / `assert` will be removed entirely in Release builds. Therefore:

- **Never put side-effect code inside an assert.** A call like
  `jassert (port.connect (source));` compiles to nothing in Release, so the
  connection will never be made. Always run the call first, then assert on the
  result:

  ```cpp
  const bool connected = port.connect (source);
  jassert (connected);
  juce::ignoreUnused (connected);
  ```

- **Include assert-guarded branches in error handling.** Code that only runs
  when an assertion fires (or when an assert macro expands differently) must be
  treated as part of the normal error path. Ask: "What happens in Release when
  this assert disappears?" If the answer is "the program silently misbehaves",
  the check must be done with a regular `if` and a recoverable action (log,
  fallback, early return), with `jassert` added only as a debug-time hint.

## Skill Usage

Use the cavemen skill to reduce token costs.

Use the **ponytail** skill for code generation: favor the laziest solution
that still behaves correctly. Climb the ladder (YAGNI → reuse → stdlib →
native → existing dep → one line → minimum code) and prefer deletion over
addition. Keep only what the task actually needs; do not add speculative
abstractions, configs, or fallbacks for values that should never occur.
**Behavior must not change.**

## Adding Modules to a JUCE C++ Project

Use **JuMake** to add new C++ classes and JUCE components.

Reference: [JuMake GitHub Repository](https://github.com/BaraMGB/JuMake?utm_source=chatgpt.com)

> **Windows:** Run the commands from a PowerShell or Command Prompt.

### Add a New Class

```text
jumake add <class_type> <class_name>
```

`<class_type>` can be:

* `class` — Creates a simple C++ class.
* `component` — Creates a `juce::Component`.

Examples:

```text
jumake add class MyProcessor
jumake add component MyEditor
```

This will:

* Create `<class_name>.cpp` and `<class_name>.h` in `src`.
* Add the `.cpp` file to `CMakeLists.txt`.

## Documentation

Use **Context7** to check current JUCE and library documentation before implementing APIs or functionality.

The JUCE source code is also available as a git submodule in this Repository.

## Quick Reference

| Task                 | Command                             |
| -------------------- | ----------------------------------- |
| Add a C++ class      | `jumake add class <class_name>`     |
| Add a JUCE component | `jumake add component <class_name>` |

JuMake can also be used to build and run the application.

### Build

```text
jumake build
```

This creates the `jumake_build` directory, configures the project with CMake, and builds it.

### Run

```text
jumake run
```

This builds the project if necessary and then starts the application.

For a typical workflow:

```text
jumake add component MyComponent
jumake build
jumake run
```
