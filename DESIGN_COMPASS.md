# Otso Design Compass

This file is the visual and product compass for every UI/UX and implementation decision in this repository.

## The 10 Principles of Good Design

1. **Is Innovative**
   - Technology and design advance together.
2. **Makes a Product Useful**
   - Function first, distractions removed.
3. **Is Aesthetic**
   - Clean, calm, and well-executed visual quality.
4. **Makes a Product Understandable**
   - UI explains itself through clear structure and labels.
5. **Is Unobtrusive**
   - Neutral, restrained, tool-like.
6. **Is Honest**
   - No fake complexity, no false sense of capability.
7. **Is Long-lasting**
   - Avoid trend-driven visuals that age quickly.
8. **Is Thorough Down to the Last Detail**
   - Spacing, alignment, contrast, and behavior are intentional.
9. **Is Environmentally Friendly**
   - Efficient runtime, lower resource usage, less waste.
10. **Is As Little Design as Possible**
   - Keep only what is essential.

## Project Translation (Non-Negotiables)

- Keep UI architecture lightweight (Win32-native first; avoid heavy framework drift unless justified).
- Prioritize readability and consistency over novelty.
- Keep interaction predictable and discoverable.
- Minimize visual noise: fewer controls, clear hierarchy, restrained color use.
- Favor performance-stable rendering paths and avoid unnecessary redraw complexity.
- Every new UI element must justify utility and maintenance cost.

## UI Decision Checklist (Before Merge)

- Is this feature/tooling **actually useful** for editing workflows?
- Does it reduce or increase visual/cognitive load?
- Is spacing, typography, and alignment consistent with existing chrome?
- Is it still intuitive for first-time users without explanation?
- Does it preserve startup speed and memory goals?
- Did we avoid adding decorative complexity?

If any answer is "no", revise before merging.

