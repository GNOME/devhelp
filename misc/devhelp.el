; Emacs integration by Richard Hult <richard@imendio.com>

(defun devhelp-word-at-point ()
  "runs devhelp"
  (interactive)
  (setq w (current-word))
  (start-process-shell-command "devhelp" nil "devhelp" "-s" w)
  )

; Example: bind F7 to start devhelp and search for the word at the point.
; (global-set-key [f7] 'devhelp-word-at-point)

; Tips: use -g WIDTHxHEIGHT+XOFF+YOFF to set the size and position of
; the window
