" Vim syntax file
" Language: BSX

if exists("b:current_syntax")
    finish
endif

setlocal cindent
setlocal cinkeys-=0#
setlocal cinoptions+=+0,p0,(0,W4
setlocal suffixesadd=.bs,.bsx
setlocal commentstring=#%s

syntax clear
syntax match Number "\<0x[0-9a-fA-F]\+\>"
syntax match Number "\<[0-9]\+\(\.[0-9]\+\)\?\>"
syntax match Comment "#.*" contains=Todo
syntax match Identifier "\.\a\w*\>"hs=s+1

syntax keyword Todo TODO XXX FIXME NOTE
syntax keyword Keyword fr fuck nah thicc error redpill vibeof ghost be ayo sayless sike amongus yall yolo yeet slickback fam mf bet
syntax keyword Boolean nocap cap
syntax keyword Constant bruh deez franky is_big_boss

syntax match Function "\<\a\w*\>" contained
syntax keyword Keyword lit skipwhite skipempty nextgroup=Function

syntax match Inheritance "\s*<\s*" contained
syntax match Type "\<\a\w*\>\(\s*<\s*\a\w*\>\)\?" contained contains=Inheritance
syntax keyword Keyword wannabe skipwhite skipempty nextgroup=Type

syntax match Error '\\.' contained
syntax match SpecialChar '\\n\|\\t\|\\"\|\\\\' contained
syntax region Parenthesis contains=TOP matchgroup=NONE start='(' end=')'
syntax region String contains=Error,SpecialChar,Interpolation start='"' skip='\\\\\|\\"' end='"'
syntax region Interpolation contained contains=TOP matchgroup=Special start='\\(' end=')'

let b:current_syntax = "bsx"
