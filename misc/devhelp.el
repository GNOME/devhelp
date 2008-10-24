;; Emacs integration by Richard Hult <richard@imendio.com>
;;

(defun devhelp-word-at-point ()
  "Searches for the current word in Devhelp"
  (interactive)
  (setq w (current-word))
  (start-process-shell-command "devhelp" nil "devhelp" "-s" w)
  (set-process-query-on-exit-flag (get-process "devhelp") nil)
  )
(defun devhelp-assistant-word-at-point ()
  "Searches for the current work in the Devhelp assistant"
  (interactive)
  (setq w (current-word))
  (start-process-shell-command "devhelp" nil "devhelp" "-a" w)
  (set-process-query-on-exit-flag (get-process "devhelp") nil)
  )

(defvar devhelp-timer nil)
(defun devhelp-disable-assistant ()
  (message "Devhelp assistant disabled")
  (cancel-timer devhelp-timer)
  (setq devhelp-timer nil)
)
(defun devhelp-enable-assistant ()
  (message "Devhelp assistant enabled")
  (setq devhelp-timer (run-with-idle-timer 0.6 t 'devhelp-assistant-word-at-point))
)
(defun devhelp-toggle-automatic-assistant ()
  "Toggles automatic Devhelp assistant on and off"
  (interactive)
  (if devhelp-timer (devhelp-disable-assistant) (devhelp-enable-assistant))
)

;; Examples:
;;
;; Bind F7 to start devhelp and search for the word at the point.
;; (global-set-key [f7] 'devhelp-word-at-point)
;;
;; Bind F6 to enable the automatic assistant.
;; (global-set-key [f6] 'devhelp-toggle-automatic-assistant)
;;
;; Bind F6 to search with the assistant window.
;; (global-set-key [f6] 'devhelp-assistant-word-at-point)

(provide 'devhelp)
