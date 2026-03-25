Run a full build + smoke test cycle for Ingnomia.

1. Use the `run_smoke_test` MCP tool from `ingnomia-test` to build and test
2. If the build fails, analyze compiler errors and suggest fixes — stop here
3. If the test times out or crashes, check the game output for clues
4. Report the metrics: build status, whether the save loaded, tick performance, screenshot path
5. If a screenshot was produced, read it and confirm the game rendered correctly
6. If visual regression ran, report the diff percentage
