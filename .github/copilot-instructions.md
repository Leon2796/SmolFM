# Instructions

## Skill Usage

Use the cavemen skill to reduce token costs.

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
