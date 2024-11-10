augroup bs
    autocmd!
    autocmd BufRead,BufNewFile *.bs setlocal filetype=bs
    autocmd BufRead,BufNewFile * if getline(1) =~# '^#!.*\<bs$' | setlocal filetype=bs | endif
augroup END
