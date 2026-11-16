# N6: Robot tests an agent can run with a person in the loop

**Status:** measured. **Built:** increment 1, in `conq test` (`host/src/conq/robottest.py`). **Used on the robot:** [test report of 2026-11-12](../test-reports/2026-11-12-increment-1/README.md). **Published here:** 2026-11-16.

## The problem

Some checks on a real robot need a person: which way a track turned, how far the robot drove. Test tools that ask the operator expect someone at the keyboard. An AI agent running the tests can't see the robot, and if it answers a prompt itself, or a timeout answers for it, the result claims more than anyone checked.

## The idea

Treat the person as a sensor the agent can ask in advance, and never let a missing answer pass.

1. **Telemetry first.** The test checks everything the robot reports itself, and asks the person only what it can't observe.
2. **Keyed questions.** Each question has a stable key, such as `tracks_correct`. The agent reads the test's brief, asks the person in chat, and passes the answers in with `--answer tracks_correct=yes`. A person at the keyboard is asked the same question directly.
3. **Incomplete, not passed.** A question nobody answered, or a step that couldn't be done, makes the run *incomplete*. It is never counted as passed or failed, and never filled in with a default. The report says which key is missing, so the answer can be given and the test run again.
4. **Motion needs consent.** A test that moves the robot refuses to run without `--confirm-moves`, so an agent can't start the motors by accident.
5. **Traceable.** Each test names the requirements it checks. The report links requirement, test, a JSON Lines log of every message, and the firmware commit the robot reported.

The 2026-11-12 report shows it working: the `directions` test came back incomplete because the person's answer was missing, and the answer given straight after in chat completed it.

## Building blocks, all public

- Hardware tests in Python that check measurements against limits: [OpenHTF](https://github.com/google/openhtf), [labgrid](https://labgrid.readthedocs.io).
- Pausing a test for the operator: OpenHTF's `UserInput`, Robot Framework's [Dialogs](https://robotframework.org/robotframework/latest/libraries/Dialogs.html), NI TestStand's message popups, [pytest-gak](https://pypi.org/project/pytest-gak/), labgrid's manual power and switch drivers.
- Operator input filled in ahead of time: the [UniteLabs SDK](https://docs.unitelabs.io/automate/concepts/human-in-the-loop/), for lab workflows.
- An "incomplete" verdict for tests that couldn't finish: patent [US7631227](https://patents.google.com/patent/US7631227).
- Coding agents driving real embedded boards: [Agentic HIL](https://pypi.org/project/agentic-hil/).

## What we searched

Public projects and documentation on 2026-11-16, plus one quick Google Patents query (not a proper patent search). The nearest matches were OpenHTF (prompts, but answers can't be given in advance), UniteLabs (pre-filled input, but not for testing) and Agentic HIL (agents testing boards, but never asking a person). We didn't find the five parts together. Each part is well known, so this is a combination, written down so anyone can use it.
