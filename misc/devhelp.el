(defun devhelp-word-at-point ()
  "runs devhelp"
  (interactive)
  (setq w (current-word))
  (start-process-shell-command "devhelp" nil "devhelp" "-s" w)
  )

; Example: bind F7 to start devhelp and search for the word at the point.
;;(global-set-key [f7] 'devhelp-word-at-point)
