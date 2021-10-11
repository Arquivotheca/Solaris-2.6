;GIL-3
(
(
	:type                   :popup-window
	:name                   ClientWin
	:owner                  BaseWin
	:width                  480
	:height                 337
	:background-color       ""
	:foreground-color       ""
	:label                  "Software Manager:  Client Support Chooser"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            t
	:resizable              t
	:pinned                 t
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   ClientCtrl
	:owner                  ClientWin
	:help                   "Client Support Control Panel

  For more detailed information about a specific
  control, use the mouse to position the cursor
  over the control of interest, then press the
  \"Help\" key.
"
	:x                      0
	:y                      0
	:width                  480
	:height                 337
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            nil
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :scrolling-list
	:name                   ClientOs
	:owner                  ClientCtrl
	:help                   "Operating System Version List

Lists available or installed operating system releases.

  The OS list enables you to select the OS
  releases for which you wish to install or
  remove client support.  If you are adding
  client support, this list will contain an
  entry for each release available on the
  currently-loaded source distribution.  If
  you are removing client support, the list
  will contain an entry for each release for
  which you have installed client support.

  Click the SELECT mouse button while the
  cursor is positioned over the desired entry
  to select a release for installation or
  removal.  The architecture list will be
  updated to reflect architectural support
  you may install or remove.
"
	:x                      18
	:y                      10
	:width                  350
	:height                 128
	:value-x                105
	:value-y                10
	:rows                   6
	:foreground-color       ""
	:label                  "OS Version:"
	:title                  ""
	:label-type             :string
	:layout-type            :horizontal
	:read-only              t
	:multiple-selections    nil
	:selection-required     nil
	:initial-state          :active
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:initial-list-values    ()
	:initial-list-glyphs    ()
	:initial-selections     ()
	:user-data              ()
	:actions                ()
)
(
	:type                   :scrolling-list
	:name                   ClientArch
	:owner                  ClientCtrl
	:help                   "Architecture List

Lists available or installed architectural support for the selected OS release.

  The Architecture list enables you to
  select for installation or removal the
  architectural support for the selected OS
  release.  If you are adding client support,
  this list will contain an entry for each
  architecture supported in the selected OS
  release.  If you are removing client
  support, the list will contain an entry
  for each architecture which you have
  installed client support.

  Click the SELECT mouse button while the
  cursor is positioned over the desired entry
  to select an architecture for installation
  or removal.  You may select more than one
  architecture.  Press Apply when ready to
  commit your selections.
"
	:x                      10
	:y                      148
	:width                  350
	:height                 128
	:value-x                105
	:value-y                148
	:rows                   6
	:foreground-color       ""
	:label                  "Architecture:"
	:title                  ""
	:label-type             :string
	:layout-type            :horizontal
	:read-only              t
	:multiple-selections    t
	:selection-required     t
	:initial-state          :inactive
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:initial-list-values    ()
	:initial-list-glyphs    ()
	:initial-selections     ()
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   ClientLists
	:x                      10
	:y                      10
	:group-type             :column
	:members                (ClientOs ClientArch )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          ClientCtrl
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      10
	:vertical-offset        10
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   ClientApply
	:owner                  ClientCtrl
	:help                   "Apply Button

Commits the current OS release and architecture list selections and updates the software display.
 
  When you have selected the OS release and
  architectural support to be installed or
  removed, commit your selections with the
  Apply button.  The software display will
  be updated to reflect your selections.

  In the software display window, choose the
  software you wish to install or remove
  and press the Begin... button to initiate
  the installation or removal.
"
	:x                      123
	:y                      306
	:width                  42
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Apply"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (ClientWin ClientApply)
		:when                   (Notify )
		:to                     (ClientWin ClientApply)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("SetClientInfo(item);")
		)
	)
)
(
	:type                   :button
	:name                   ClientReset
	:owner                  ClientCtrl
	:help                   "Reset Button

Resets the current OS release and architecture list selections to match those currently being edited.
 
  The Reset button restores the OS release
  and architectural support list contents
  and selection statuses to those in force
  when you last pressed Apply accessed the
  Client Support functions from the Edit
  menu.
"
	:x                      245
	:y                      306
	:width                  66
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Reset"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (ClientWin ClientReset)
		:when                   (Notify )
		:to                     (ClientWin ClientReset)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ResetClientInfo(item);")
		)
	)
)
(
	:type                   :group
	:name                   ClientButtons
	:x                      123
	:y                      306
	:group-type             :row
	:members                (ClientApply ClientReset )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          ClientLists
	:anchor-point           :south
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        30
	:user-data              ()
	:actions                ()
)
)
