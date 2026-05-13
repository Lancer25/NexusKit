# Contributing

NexusKit uses CMake, C++17, CTest, and focused module boundaries.

Before submitting changes:

1. Configure the project with the relevant preset.
2. Build the project.
3. Run CTest.
4. Add or update tests for behavior changes.
5. Keep public headers platform-neutral unless a platform extension is intentional.

Do not introduce private SDK dependencies.
