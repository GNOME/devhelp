" vim macro to jump to devhelp topics.
" Idea copied from the same "emacs integration" script by 
" Richard Hult <richard@imendio.com>.
" -- Gert
function! DevHelpCurrentWord()
	let word = expand("<cword>")
	exe "!devhelp -s " . word
endfunction

" Example: bind <ESC>h to start devhelp and search for the word under the
" cursor
nmap <ESC>h :call DevHelpCurrentWord()<CR>

" Tips: use -g WIDTHxHEIGHT+XOFF+YOFF to set the size and position of
" the window
