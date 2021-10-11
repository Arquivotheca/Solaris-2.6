;GIL-3
(
(
	:type                   :popup-window
	:name                   FileWin
	:owner                  BaseWin
	:width                  394
	:height                 115
	:background-color       ""
	:foreground-color       ""
	:label                  "Software Manager:  File Chooser"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            t
	:resizable              t
	:pinned                 nil
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   FileCtrl
	:owner                  FileWin
	:help                   "File Chooser Control Panel
 
  For more detailed information about a specific
  control, use the mouse to position the cursor
  over the control of interest, then press the
  \"Help\" key.
"
	:x                      0
	:y                      0
	:width                  394
	:height                 115
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
	:type                   :text-field
	:name                   FileDir
	:owner                  FileCtrl
	:help                   "Directory Name
 
Specifies the directory in which to store configuration files.
 
  Use this field to enter the name of the
  directory in which you wish to save a
  configuration file.
"
	:x                      30
	:y                      15
	:width                  330
	:height                 15
	:value-x                104
	:value-y                15
	:value-length           32
	:stored-length          1024
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Directory:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          ""
	:initial-state          :active
	:read-only              nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   FileName
	:owner                  FileCtrl
	:help                   "File Name
 
Specifies configuration file name.
 
  Use this field to enter the name of the file in
  which you wish to save the program's current
  properties.  The full path of this file will be
  the concatenation of the values of the Directory
  and Name fields.  The program will attempt to
  save its configuration either when you type a
  Return or press the Save button.
"
	:x                      54
	:y                      45
	:width                  306
	:height                 15
	:value-x                104
	:value-y                45
	:value-length           32
	:stored-length          1024
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Name:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          ""
	:initial-state          :active
	:read-only              nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (FileWin FileName)
		:when                   (Notify )
		:to                     (FileWin FileName)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ConfigFile(CONFIG_SAVE);")
		)
	)
)
(
	:type                   :group
	:name                   FileGrp
	:x                      30
	:y                      15
	:group-type             :column
	:members                (FileDir FileName )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       15
	:anchor-object          FileCtrl
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      30
	:vertical-offset        15
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   FileDoit
	:owner                  FileCtrl
	:help                   "Save Button
 
Saves current configuration to specified file.
 
  Use this button when you are ready to save the
  current configuration.  If you are saving to a
  file which already exists, you will be asked for
  confirmation.
"
	:x                      172
	:y                      90
	:width                  46
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Save"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (FileWin FileDoit)
		:when                   (Notify )
		:to                     (FileWin FileDoit)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ConfigFile((CF_mode)xv_get(item, PANEL_CLIENT_DATA));")
		)
	)
)
(
	:type                   :group
	:name                   FileButtons
	:x                      172
	:y                      90
	:group-type             nil
	:members                (FileDoit )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          FileGrp
	:anchor-point           :south
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        30
	:user-data              ()
	:actions                ()
)
)
