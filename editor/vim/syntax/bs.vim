" Vim syntax file
" Language: BS

if exists("b:current_syntax")
    finish
endif

setlocal cindent
setlocal cinkeys-=0#
setlocal cinoptions+=+0,p0,(0,W4
setlocal suffixesadd=.bs
setlocal commentstring=#%s

syntax clear
syntax match Number "\<0x[0-9a-fA-F]\+\>"
syntax match Number "\<[0-9]\+\(\.[0-9]\+\)\?\>"
syntax match Comment "#.*" contains=Todo
syntax match Identifier "\.\s*\a\w*\>"hs=s+1

syntax keyword Todo TODO XXX FIXME NOTE
syntax keyword Keyword len panic assert import typeof delete if then else in is for while break continue pub var return
syntax keyword Boolean true false
syntax keyword Constant nil this super is_main_module

syntax match Function "\<\a\w*\>" contained
syntax keyword Keyword fn skipwhite skipempty nextgroup=Function

syntax match Inheritance "\s*<\s*" contained
syntax match Type "\<\a\w*\>\(\s*<\s*\a\w*\>\)\?" contained contains=Inheritance
syntax keyword Keyword class skipwhite skipempty nextgroup=Type

syntax match Error '\\.' contained
syntax match SpecialChar '\\e\|\\n\|\\r\|\\t\|\\0\|\\"\|\\\\' contained
syntax region Parenthesis contains=TOP matchgroup=NONE start='(' end=')'
syntax region String contains=Error,SpecialChar,Interpolation start='"' skip='\\\\\|\\"' end='"'
syntax region Interpolation contained contains=TOP matchgroup=Special start='\\(' end=')'

let b:current_syntax = "bs"
