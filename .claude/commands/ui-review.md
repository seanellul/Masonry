Review the current state of the Ingnomia UI by building, taking a screenshot, and providing actionable UX feedback.

The goal is a RimWorld-inspired UI/UX: clean layout, readable data, intuitive controls, modern feel. The UI is built with Dear ImGui (src/gui/ui/).

## Steps

1. **Build**: Use the `build_game` MCP tool. If it fails, fix compile errors first.

2. **Screenshot**: Use the `take_screenshot` MCP tool with the default test save. If no test save exists, tell the user.

3. **View**: Read the screenshot image file to see the current UI state.

4. **Analyze**: Evaluate the UI against these criteria, referencing specific ImGui panels visible in the screenshot:
   - **Layout**: Is information well-organized? Are panels cluttered or well-spaced?
   - **Readability**: Are labels, numbers, and status indicators easy to scan?
   - **Controls**: Are interactive elements (buttons, tabs, lists) discoverable and intuitive?
   - **Visual hierarchy**: Does the most important info stand out? Is there good use of spacing, sizing, and grouping?
   - **Consistency**: Are styles, colors, and spacing consistent across panels?

5. **Recommend**: Give 3-5 specific, actionable improvements. For each:
   - Describe what to change and why
   - Reference the specific file in `src/gui/ui/` that would need editing
   - Keep suggestions achievable with ImGui (no custom rendering unless necessary)

6. **Compare** (if reference exists): Use `compare_screenshots` to check against the reference. Note any visual regressions.

If the user provides specific UI areas to focus on (e.g., "look at the tile info panel"), prioritize those in your analysis.
