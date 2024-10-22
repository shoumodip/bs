# CLI Task Management APP

<!-- embed: examples/tasks/tasks.bs -->

```console
$ bs tasks.bs add "Finish reading book"
Added: Finish reading book

$ bs tasks.bs add "Submit project report"
Added: Submit project report

$ bs tasks.bs add "Submit homework"
Added: Submit homework

$ bs tasks.bs list
[0] Finish reading book
[1] Submit project report
[2] Submit homework

$ bs tasks.bs list 'Submit.*report'
[1] Submit project report

$ bs tasks.bs done 0
Done: Finish reading book

$ bs tasks.bs list
[0] Submit project report
[1] Submit homework
```
