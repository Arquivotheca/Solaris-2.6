;GIL-3
(
(
	:type                   :popup-window
	:name                   CmdWin
	:owner                  BaseWin
	:width                  587
	:height                 294
	:background-color       ""
	:foreground-color       ""
	:label                  "Software Manager:  Command Input/Output"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            nil
	:resizable              t
	:pinned                 t
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :term-pane
	:name                   Term
	:owner                  CmdWin
	:help                   ""
	:x                      0
	:y                      0
	:width                  587
	:height                 294
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
)
