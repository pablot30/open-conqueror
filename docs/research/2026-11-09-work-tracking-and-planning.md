# R4: Work tracking and planning

**Background research from 9 November 2026, written before [decision 0006](../decisions/0006-walking-skeleton-first.md) cut stage 2 back to a walking skeleton.** Its recommendations are input, not decisions. Where a decision or the [architecture](../architecture.md) says otherwise, they win.

**In one line:** Track the work where the code lives.

Research date: 9 November 2026. Sources are cited inline. **[unverified]** marks claims from the researcher's background knowledge that were not checked against a source in this research. **(third-party)** marks figures from aggregators rather than the vendor, which should be checked on the vendor's own page before anyone relies on them.

open-conqueror should track its work in **GitHub Issues plus one GitHub Project**, not Jira. This is already what `docs/roadmap.md` says ("GitHub Issues hold the backlog, with one milestone per stage"), and the research supports it. Since April 2025, GitHub has had issue types, sub-issues and advanced search, and since August 2025 it has had "blocked by" dependencies. Its Projects offer board, table and roadmap views with custom fields and iterations, and **"all users have access to the free tier"** ([GitHub changelog](https://github.blog/changelog/2025-04-09-evolving-github-issues-and-projects/); [GitHub changelog](https://github.blog/changelog/2025-08-21-dependencies-on-issues/); [GitHub Features](https://github.com/features/issues)). Jira Cloud Free is a capable tracker. But it is proprietary, caps a site at **10 users**, allows about **100 automation runs a month**, and is deactivated after **120 days** without activity. It would also put a second source of truth beside the repo that outside contributors would need an Atlassian account to reach. The open-source trackers (Plane, OpenProject) are healthy, but each means running a server. For AI agents, GitHub's and Anthropic's guidance agree: **the issue is the prompt, the assignment is the claim, and each agent works on its own branch and pull request**, with human review as the real bottleneck. The rest of this report turns that into a concrete setup. The main open points are whether issue types work on a personal-account repo and the fact that GitHub has no atomic way to claim a task.

## Jira's strengths don't outweigh a second system

The comparison below looks at how well each tool fits this project: one maintainer now, parallel agents soon, and a public repo with outside contributors later.

| Tool | Licence and hosting | Free tier | Link to GitHub | Agent access | Fit |
|---|---|---|---|---|---|
| **GitHub Issues and Projects** | Proprietary SaaS, inside the repo | Included for everyone. Projects hold up to 50,000 items and 50 fields ([changelog](https://github.blog/changelog/2025-04-09-evolving-github-issues-and-projects/); [Docs](https://docs.github.com/en/issues/planning-and-tracking-with-projects/learning-about-projects/about-projects)) | Native: PRs, `Fixes #N`, CI | `gh`, REST, GraphQL, webhooks; dependencies are in the API ([changelog](https://github.blog/changelog/2025-08-21-dependencies-on-issues/)) | **Best** |
| **Jira Cloud Free** | Proprietary SaaS | 10 users, 2 GB storage, about 100 automation runs a month, no permission schemes or audit logs (third-party: [eesel.ai](https://www.eesel.ai/blog/jira-pricing-guide), [eesel.ai](https://www.eesel.ai/blog/is-automation-in-jira-free)). Deactivated after 120 days idle ([Atlassian](https://support.atlassian.com/jira-cloud-administration/docs/explore-jira-cloud-plans/)) | Through the GitHub for Jira app and Jira keys in commits **[unverified]** | Official Rovo MCP Server, generally available ([Atlassian](https://www.atlassian.com/blog/announcements/atlassian-rovo-mcp-ga)) | Poor once public |
| **Linear Free** | Proprietary SaaS | 250 issues, 2 teams. Sources disagree on the seat limit (third-party: [CostBench](https://costbench.com/software/developer-tools/linear/free-plan/)) | Good PR and branch linking (third-party: [automationatlas](https://automationatlas.io/tools/linear/)) | GraphQL API | Too small |
| **Plane Community** | AGPL-3.0, self-hosted on Docker or Kubernetes ([Plane](https://plane.so/self-hosted)) | Free to run. Two-way GitHub sync carries a "Pro" badge ([Plane docs](https://docs.plane.so/integrations/github)) | Paid sync | API | Server plus paid sync |
| **OpenProject** | GPL-3.0, self-hosted. v17.9.2 in October 2026 (third-party: [fastlancer](https://www.fastlancer.org/en/fastlancer-blog/open-source-project-management/)) | Free Community edition with Gantt charts **[unverified]** | Links PRs **[unverified]** | API | Heavy for one person |
| **Taiga, Redmine** | Open source. Licence claims differ between sources (third-party: [fastlancer](https://www.fastlancer.org/en/fastlancer-blog/open-source-project-management/), [projectmanager.com](https://www.projectmanager.com/blog/open-source-project-management-software)) | Free to run | Weak | API | Dated fit |
| **Forgejo or Gitea, GitLab** | Forgejo is GPL, Gitea is MIT, GitLab is open core **[unverified]** | Issues, labels, milestones and boards. GitLab's epics and roadmaps are paid **[unverified]** | Means leaving GitHub | Varies | Only if the project leaves GitHub |

Jira's real advantage is familiarity, and much of that carries over (see the term mapping below). Its costs land at publication. Every outside contributor would need an Atlassian account and would count toward the 10-user cap, while on GitHub anyone with an account can open an issue **[unverified for Jira's free open-source licence programme in 2026]**. Self-hosting Plane or OpenProject means a VPS, backups and patching, which works against the project's "keep computers clean" rule. Plane recommends about 4 GB of RAM ([Plane self-hosted](https://plane.so/self-hosted)). Linear's issue cap would be hit within stage 2. The one idea worth borrowing from Linear is its habit of **putting the issue number in the branch name**, so the work links itself.

## One milestone per stage, one Project for everything

The setup below needs nothing installed beyond the `gh` CLI, which the project already uses for its rulesets (`docs/tools.md`).

**Milestones** follow the roadmap: "Stage 2: Own the UNO", "Stage 3: Own the camera board", and later stages as they come near. A milestone gives a progress bar and an optional due date. Stage 2 has two phases (the plan, then the firmware). The plan fits best as one **parent issue with sub-issues**, one per plan topic: requirements, development loop, quality rules, toolchain, architecture, protocol, calibration and diagnostics, security, releases and backlog. Sub-issues allow up to about 100 children per parent and 8 levels of nesting. That limit comes from a third-party article and one community report, not GitHub's docs ([mojoauth](https://mojoauth.com/blog/enhancing-github-issue-management-with-sub-issues-feature); [community #154148](https://github.com/orgs/community/discussions/154148)).

**Issue types** should be Task, Feature and Bug, with parent issues playing the role of epics. Issue types are configured at the organisation level, and `pablot30/open-conqueror` is a personal-account repo, so they may not be available **[unverified]**. If they aren't, `type:task`, `type:feature` and `type:bug` labels do the same job.

**Labels and fields:**

| Name | Kind | Purpose |
|---|---|---|
| `req:F1` … `req:Q6` | Label, one per requirement ID (60 in all) | One issue can serve several requirements. Agents can query with `gh issue list --label req:S1`. Supports Q6 traceability |
| `needs-robot` | Label | The issue needs the maintainer to flash the robot, run its motors or measure something. Agents never claim these |
| `safety` | Label | Touches an S requirement. Needs the maintainer's sign-off on the robot |
| `agent:<name>` | Label | Shows which agent holds the claim (see below) |
| `good first issue`, `help wanted` | Label | Created at publication |
| Status | Project single-select | Triage, Backlog, Ready, In progress, In review, Needs robot, Done |
| Area | Project single-select | Driving, Modes, Safety, Link and telemetry, Tooling, Quality. Use it to group boards, because labels hold many values |
| Size | Project single-select | S, M or L. Only for spotting issues that are too big for one agent |
| Iteration | Project iteration field | Optional. Iterations support breaks ([GitHub Docs](https://docs.github.com/en/issues/planning-and-tracking-with-projects/learning-about-projects/about-projects)) |

**Views**, which are saved filters on the one Project ([GitHub Docs](https://docs.github.com/en/issues/planning-and-tracking-with-projects/learning-about-projects/about-projects)):

| View | Layout | Filter or grouping | Who uses it |
|---|---|---|---|
| Board | Board | Current milestone, columns by Status | Everyone |
| Backlog | Table | Grouped by milestone, then Area | Planning |
| Ready for agents | Table | Status Ready, no assignee, no `needs-robot` label, not blocked | Agents and the maintainer |
| Needs robot | Table | Label `needs-robot`, or Status Needs robot | The maintainer at the bench |
| Roadmap | Roadmap | Milestones or parent issues on a timeline | Readers after publication |

**Issue forms** live in `.github/ISSUE_TEMPLATE/`, with blank issues turned off in `config.yml` **[unverified syntax]**. Anthropic recommends issue templates so agents get their context up front ([Claude Code Docs](https://code.claude.com/docs/en/github-actions)), and GitHub advises writing an issue for an agent as you would a prompt, with clear acceptance criteria ([GitHub blog](https://github.blog/ai-and-ml/github-copilot/onboarding-your-ai-peer-programmer-setting-up-github-copilot-coding-agent-for-success/)). The task form should require requirement IDs and acceptance criteria, each criterion saying whether it is checked on a computer or on the robot:

```yaml
name: Task
description: A piece of work that serves one or more requirements
labels: ["type:task"]
body:
  - type: input
    id: reqs
    attributes: {label: Requirement IDs, description: "e.g. F1, S3, D7"}
    validations: {required: true}
  - type: textarea
    id: criteria
    attributes:
      label: Acceptance criteria
      description: "One checkbox each. End each with (computer) or (robot)."
      value: "- [ ] "
    validations: {required: true}
  - type: textarea
    id: scope
    attributes: {label: Files or modules in scope, description: "Keeps parallel work apart"}
  - type: dropdown
    id: robot
    attributes: {label: Needs the physical robot?, options: ["No", "Yes"]}
    validations: {required: true}
```

The bug form should ask for the firmware version and release (D4, D16), the requirement the bug breaks, steps to reproduce, and a link to a log or recording (D8, D14).

## Agents claim work by label, branch and pull request

GitHub's coding agent treats an assigned issue as its prompt, works in its own sandbox and opens a draft PR. GitHub now offers a console for steering several agents, including third-party ones ([GitHub Docs](https://docs.github.com/en/copilot/how-tos/copilot-on-github/use-copilot-agents/overview); [GitHub blog](https://github.blog/ai-and-ml/github-copilot/how-to-orchestrate-agents-using-mission-control/)). Anthropic's GitHub Action reacts to `@claude` mentions or runs a fixed prompt on any event. Only users with write access can trigger it, and other bots are refused unless they are listed ([Claude Code Docs](https://code.claude.com/docs/en/github-actions)). Practitioners report that narrow, single-concern issues work best, that agents editing the same files cause merge conflicts, and that about **3–5 parallel sessions per person** is the ceiling, because review capacity runs out first (opinion: [agentpatterns.ai](https://agentpatterns.ai/training/copilot/advanced-patterns/)).

No official source defines a claiming protocol for several independent agents sharing one backlog, so the one below is this project's own convention. One detail shapes it: local Claude Code sessions all act as the maintainer's GitHub account, so the assignee can't tell agents apart. The `agent:<name>` label does that job. Assigning and labelling are also two separate calls, so two agents could claim the same issue at once. The safe default is for **the maintainer, or one coordinating session, to hand out issues**. Agents claim work themselves only when told to. This text is meant to be pasted into `AGENTS.md` or `CLAUDE.md`:

```text
Claiming work (for AI agents)
1. Only take an issue that is in the current milestone, has Status "Ready",
   has no assignee, is not blocked by an open issue, and has no `needs-robot` label.
   Take only what the maintainer assigned to you, unless told to pick for yourself.
2. Claim it: add the label `agent:<your-name>`, assign the maintainer, set Status
   to "In progress", and comment "Claimed by <your-name> on branch <branch>".
3. Re-read the issue. If another `agent:` label or claim comment was added before
   yours, remove your claim and pick something else.
4. Work in your own git worktree on branch `<type>/<issue-number>-<short-slug>`.
   Edit only the files the issue lists as in scope. If the work grows, open a new
   issue and link it instead of widening this one.
5. Open a pull request whose body has "Fixes #<n>" and "Requirements: F1, S3".
   Set Status to "In review". Never merge. The maintainer merges.
6. If you stop before finishing, remove your label, set Status back to "Ready",
   and comment on what is done and what is left.
7. Never flash the robot or run its motors. Anything that needs the robot gets
   the `needs-robot` label and Status "Needs robot", for the maintainer.
```

`Fixes #N` closes the issue only when the PR merges into the default branch **[unverified]**, which matches the project's rule that `main` changes only through merged pull requests. Dependencies should be used for ordering, because a blocked issue never shows up in the Ready view. While the repo is private, local sessions in worktrees are the cheaper choice. Cloud agents spend Actions minutes and API tokens on every run ([Claude Code Docs](https://code.claude.com/docs/en/github-actions)), and GitHub Free's private-repo allowance is quoted as 2,000 minutes a month by a third party ([CostBench](https://costbench.com/software/developer-tools/github/free-plan/)). Check GitHub's own pricing page, as the billing model may have changed **[unverified]**. Whether GitHub's coding agent is available on a free personal account was not confirmed.

## Jira habits map almost one-to-one

| Jira | GitHub |
|---|---|
| Project | Repository plus one GitHub Project |
| Board or backlog | Project board view or table view |
| Epic, then story, then sub-task | Parent issue, then issue, then sub-issue ([changelog](https://github.blog/changelog/2025-04-09-evolving-github-issues-and-projects/)) |
| Issue type | Issue type, or a `type:` label |
| Workflow status | Project Status field, plus open or closed |
| Fix version or release | Milestone (one per stage), and a GitHub Release for D16 packages |
| Sprint | Iteration field |
| Component | Area field |
| Story points | Size field |
| Issue key `OC-12` | `#12` |
| Smart commit `OC-12 #done` | `Fixes #12` in the PR body |
| "Blocks" link | Dependencies ([changelog](https://github.blog/changelog/2025-08-21-dependencies-on-issues/)) |
| JQL | Issue search, Project filters, `gh issue list` |
| Automation rules | Built-in Project workflows, plus GitHub Actions ([GitHub Docs](https://docs.github.com/en/issues/planning-and-tracking-with-projects/learning-about-projects/about-projects)) |
| Dashboards | Project insights charts and status updates |

The biggest habit change is that GitHub has no enforced workflow. Any status can move to any other, so the forms and the claiming convention do the job that Jira's transition rules would.

## Going public opens the backlog, and its history

Issues written now will be published with the repo, so they need the same care as commits under decision 0005: no secrets, no vendor text, and nothing personal. A Project can stay private while the repo is private and be made public later with one setting, so this layout carries over unchanged **[inference]**. On publication day, the main changes are these. Turn on `good first issue` and `help wanted`, which GitHub surfaces to new contributors **[unverified]**. Make the issue forms add a Triage status, so outside reports aren't mistaken for agent-ready work. Keep iterations for the maintainer's own focus rather than as dates promised to others. Public repos also bring safety changes that matter for agents. GitHub withholds secrets from workflows triggered by fork PRs, scheduled workflows stop after 60 days without activity ([Claude Code Docs](https://code.claude.com/docs/en/github-actions)), and outsiders can't trigger the Claude action, because only users with write access can. Branch rulesets become enforceable, so `.github/rulesets/protect-main.json` can finally be applied, as `docs/tools.md` already plans.

## Gaps

Issue-type availability on personal accounts, the 100-sub-issue limit, and every `gh` command above have not been checked against 2026 primary docs. All Jira and Linear prices and limits come from third parties, and Linear's seat count is contradictory. Whether Atlassian's MCP server works on Jira Free is unknown. Plane's GitHub-sync tier, and the Forgejo, Gitea and GitLab details, are unconfirmed. No official multi-agent claiming protocol exists, so the convention above is untested here. Its weak point, a race between two agents claiming at once, is managed by having a coordinator hand out issues, not solved. How small open-source projects use iterations is convention, not measured.

## Decisions for the maintainer

1. **Tool:** GitHub Issues and Projects (recommended), Jira Free, or a self-hosted open-source tracker.
2. **Account:** keep the personal-account repo and use `type:` labels, or move the repo to a free organisation for native issue types. Check what each option actually allows first.
3. **Requirement IDs:** one `req:` label per ID (recommended), a Project text field, or both.
4. **Stage 2 shape:** one milestone with a "Stage 2 plan" parent issue (recommended), or separate milestones for the plan and the firmware. Also whether milestones get due dates.
5. **Status values:** accept the seven above, including "Needs robot", or trim them.
6. **Iterations:** none for now, or two-week iterations for the maintainer's own focus.
7. **Claiming:** a coordinator hands out issues (recommended), or agents claim for themselves. Also how long an untouched claim lasts before the maintainer may release it.
8. **Agents while private:** local worktree sessions only, or also cloud agents that spend Actions minutes and tokens.
9. **Acceptance criteria style:** a plain checklist tagged "(computer)" or "(robot)", or Given/When/Then.
10. **Where the rules live:** `CONTRIBUTING.md` for people and `AGENTS.md` or `CLAUDE.md` for agents, recorded as decision 0006.
11. **Publication-day list:** making the Project public, triage rules, contributor labels, and applying the ruleset.

## Possibly novel ideas: discuss before publishing

None. Requirement-ID labels, issue forms, label-based claims and a branch per issue are established practice that GitHub and Anthropic already describe. The `needs-robot` handoff is a simple workflow convention, not an invention.

## Conclusion

The question changed from "which tool" to "what discipline". GitHub already covers everything Jira would give this project, so what matters is how good the issues are, because for agents an issue is a specification. Forms that require requirement IDs and checkable acceptance criteria do more for parallel agent work than any tracker feature, and they also give Q6 traceability its first link: requirement to issue to pull request.

The physical robot is the real constraint on throughput. Agents can work in parallel on computer-side work, but anything that touches the robot goes through one person at one bench. So the `needs-robot` queue, not the agent count, will set the pace of stage 2, and it is worth watching from the first sprint.
