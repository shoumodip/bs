# CLI Task Management APP

<!-- embed: examples/tasks/tasks.bs -->

```console
# Adding a new task (Default priority is medium)
$ bs tasks.bs add "Finish reading book"
Added: Finish reading book [Priority: medium]

# Adding a task with a high priority and tag
$ bs tasks.bs add "Submit project report" -due "2024-10-22" -priority high -tag work
Added: Submit project report (Due: 2024-10-22) [Priority: high] [Tags: work]

# Listing all tasks
$ bs tasks.bs list
Pending Tasks:
[0] Finish reading book [Priority: medium]
[1] Submit project report (Due: 2024-10-22) [Priority: high] [Tags: work]

# Marking a task as complete
$ bs tasks.bs done 1
Completed: Submit project report (Due: 2024-10-22) [Priority: high] [Tags: work]

# Listing all tasks
$ bs tasks.bs list
Pending Tasks:
[0] Finish reading book [Priority: medium]

Completed Tasks:
[0] Submit project report (Due: 2024-10-22) [Priority: high] [Tags: work]

# Filtering tasks by tag
$ bs tasks.bs list -tag work
Completed Tasks:
[0] Submit project report (Due: 2024-10-22) [Priority: high] [Tags: work]

# Remove a completed task
$ bs tasks.bs remove 0
Removed: Submit project report (Due: 2024-10-22) [Priority: high] [Tags: work]

$ bs tasks.bs add Finish
Added: Finish [Priority: medium]

$ bs tasks.bs add foo
Added: foo [Priority: medium]

$ bs tasks.bs add bar
Added: bar [Priority: medium]

$ bs tasks.bs done 2
Completed: Finish [Priority: medium]

# Search content of tasks using Regex
$ bs tasks.bs search '[fF]'
Pending Tasks:
[0] Finish reading book [Priority: medium]
[1] foo [Priority: medium]

Completed Tasks:
[0] Finish [Priority: medium]
```
