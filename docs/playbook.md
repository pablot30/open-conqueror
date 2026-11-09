# Playbook

How we run this project, written down as we go. Each step says **when** it matters, **what** to do and **why**, with what we learned here.

This isn't a blueprint we designed in advance. It grows from what the project actually needed, and it's written so the steps carry over to other hardware projects. Newest steps go at the end.

## 1. Start with the idea and its limits

**When:** before anything else.

**What:** write one sentence that says what the project is. Add goals, a staged roadmap, and a "not now" list.

**Why:** a short "not now" list stopped us buying hardware and adding an RTOS, ROS 2 or AI features before the basics worked. See the [vision](vision.md) and [decision 0001](decisions/0001-stock-hardware-first.md).

## 2. Set up the repository properly before the first commit

**When:** before the first commit, because everything committed stays in the history.

**What:** a license, `.gitattributes` for line endings, `.editorconfig`, a `.gitignore` that keeps out secrets and vendor files, and a commit email that doesn't expose a personal address. On GitHub, also turn on "Keep my email addresses private" before the first merge on the website. Otherwise GitHub writes merge commits under your account's email. We missed this and had to rewrite the history and recreate the repo.

**Why:** check what you're allowed to publish before you publish it. The robot kit came with no license to redistribute its code or manuals, so none of it goes in the repo, and we describe hardware facts in our own words.

## 3. Record decisions when you make them

**When:** whenever a choice would be expensive to reverse or would surprise a newcomer.

**What:** a short record with the context, the decision and its consequences. When a decision changes, write a new record that replaces the old one, rather than editing it.

**Why:** we changed our minds twice in the first week: about the stock phone app, and about when to publish. The records show both the old reasoning and why it changed. See [decisions](decisions/README.md).

## 4. Keep a lab notebook, and say how you know

**When:** from the first measurement.

**What:** dated entries saying how each fact is known: measured, compiled, or only read in the source. Record failures and wrong conclusions too.

**Why:** two of our early conclusions were wrong, and the notebook shows why: [left and right](notebook/2026-11-06-first-sensor-readings.md#left-and-right-match-elegoos-naming), and a prediction from reading code that the robot didn't follow.

## 5. Back up before you change anything, and check the backup's contents

**When:** before the first flash, or any other irreversible change.

**What:** back up the factory state, read it twice, verify it against the device, and look at what's inside.

**Why:** our first EEPROM backup read cleanly twice and still [held the wrong memory](notebook/2026-11-04-backing-up-the-uno.md#the-lesson-a-clean-read-can-still-be-wrong). Reads that repeat aren't the same as reads that are correct.

## 6. Get to know the hardware one piece at a time

**When:** before writing your own firmware.

**What:** identify the parts, then test each sensor alone, then each actuator, with the robot on a stand and a hand near the power switch.

**Why:** small, single-purpose tests give answers you can trust. Agree on conventions first, such as "left means the robot's own left, seen from behind". Use a fixed camera, a clear start signal, and one line of result per step. Each of these came from a test that went wrong. See [motor channels](notebook/2026-11-06-motor-channels.md#what-we-learned-about-testing).

## 7. Only measure what serves a decision

**When:** whenever a task's purpose gets vague.

**What:** ask what decision a measurement will feed. Drop it, or move it to when it's needed, if the answer is "none yet".

**Why:** we planned to film the stock modes as a reference, then realised a phone video couldn't be compared fairly with later measurements, and the backups let us measure the stock firmware whenever we need to. See the [roadmap](roadmap.md#stage-1-order).

## 8. Plan before you build

**When:** before the first line of production code.

**What:** requirements with IDs and a way to check each one. They cover features, technical goals, safety goals derived from hazards, development needs and quality rules. Research the big choices before deciding them.

**Why:** safety and quality are much cheaper to design in than to add later. See the [requirements](requirements.md).

**But:** plan only as far as the next thing that runs. Our stage 2 plan grew into MISRA mappings, MC/DC and signed releases before any firmware existed, and we had to cut it back to a walking skeleton ([0006](decisions/0006-walking-skeleton-first.md)). Keep the full requirements as the target, and pull each one in when an increment needs it.

## 9. Protect the main line

**When:** as soon as the code lives on a shared server.

**What:** changes reach `main` only through reviewed pull requests, with no force pushes or deletions. If the platform can't enforce that, enforce it locally until it can.

**Why:** GitHub only protects branches on private repos with a paid plan. We use a [pre-push hook](tools.md#repository-setup) until the repo is public.

## 10. Keep the computer clean

**When:** every time something gets installed.

**What:** use the system's package manager or a project-local environment, install only what a step needs, and keep a [list of what's installed](tools.md) with how to remove it.

**Why:** a development machine that collects tools nobody remembers becomes impossible to reproduce on another computer, or for another person.
