" vim macro to jump to devhelp topics.
" Idea copied from the same "emacs intergration" script by <author/>
" -- Gert
function! DevHelpCurrentWord()
	let word = expand("<cword>")
	exe "!devhelp -s " . word
endfunction

" Example: bind <ESC>h to start devhelp and search for the word under the
" cursor
nmap <ESC>h :call DevHelpCurrentWord()<CR>
