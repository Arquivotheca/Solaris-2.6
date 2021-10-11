;GIL-3
(
(
	:type                   :menu
	:name                   MailMenu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Scrolling list"
	:menu-item-labels       ("Locate Choice" )
	:menu-item-label-types  (:string )
	:menu-item-states       (:active )
	:menu-item-defaults     (t )
	:initial-selections     (nil )
	:menu-item-handlers     (nil )
	:menu-item-menus        (nil )
	:menu-item-colors       ("" )
	:pinnable               nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :menu
	:name                   HostAddMenu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             ""
	:menu-item-labels       ("Before" "After" "Top" "Bottom" )
	:menu-item-label-types  (:string :string :string :string )
	:menu-item-states       (:active :active :active :active )
	:menu-item-defaults     (nil nil nil t )
	:initial-selections     (nil nil nil nil )
	:menu-item-handlers     (nil nil nil nil )
	:menu-item-menus        (nil nil nil nil )
	:menu-item-colors       ("" "" "" "" )
	:pinnable               nil
	:user-data              ()
	:actions                (
		(
		:from                   (PropsWin HostAddMenu "Before")
		:when                   (Notify )
		:to                     (PropsWin HostAddMenu "Before")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("AddHost(0);")
		)
		(
		:from                   (PropsWin HostAddMenu "After")
		:when                   (Notify )
		:to                     (PropsWin HostAddMenu "After")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("AddHost(1);")
		)
		(
		:from                   (PropsWin HostAddMenu "Top")
		:when                   (Notify )
		:to                     (PropsWin HostAddMenu "Top")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("AddHost(2);")
		)
		(
		:from                   (PropsWin HostAddMenu "Bottom")
		:when                   (Notify )
		:to                     (PropsWin HostAddMenu "Bottom")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("AddHost(3);")
		)
	)
)
(
	:type                   :menu
	:name                   HostEditMenu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Edit"
	:menu-item-labels       ("Add Entry" "Modify Entry" "Remove Entry" )
	:menu-item-label-types  (:string :string :string )
	:menu-item-states       (:active :active :active )
	:menu-item-defaults     (t nil nil )
	:initial-selections     (nil nil nil )
	:menu-item-handlers     (nil nil nil )
	:menu-item-menus        (HostAddMenu nil nil )
	:menu-item-colors       ("" "" "" )
	:pinnable               t
	:user-data              ()
	:actions                (
		(
		:from                   (PropsWin HostEditMenu "Modify Entry")
		:when                   (Notify )
		:to                     (PropsWin HostEditMenu "Modify Entry")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ChangeHost();")
		)
		(
		:from                   (PropsWin HostEditMenu "Remove Entry")
		:when                   (Notify )
		:to                     (PropsWin HostEditMenu "Remove Entry")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("DeleteHost();")
		)
	)
)
(
	:type                   :popup-window
	:name                   PropsWin
	:owner                  BaseWin
	:width                  453
	:height                 546
	:background-color       ""
	:foreground-color       ""
	:label                  "Software Manager:  Properties"
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
	:name                   PropsCtrl
	:owner                  PropsWin
	:help                   "Properties Control Panel

  For more detailed information about a specific
  control, use the mouse to position the cursor
  over the control of interest, then press the
  \"Help\" key.
"
	:x                      0
	:y                      0
	:width                  453
	:height                 46
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
	:type                   :setting
	:name                   PropsSet
	:owner                  PropsCtrl
	:help                   "Properties Category

Controls which properties are displayed in the property window's lower control panel.

  To edit a particular set of properties,
  position the cursor over the down arrow,
  press the mouse MENU button and release
  it when the desired category is highlighted.

  The available categories include:

    Source Media -- specify a software
    distribution to browse or from which
    to select modules to be installed.

    Package Administration -- specify
    parameters used during the execution of
    \"pkgadd\" and \"pkgrm\" commands.

    Current Product Category -- specify a
    software \"category\" products must
    match in order to be displayed.

    Browser Display -- specify parameters
    affecting the format of the Software
    Selection Display.

    Remote Hosts -- specify hosts other than
    the local host on which to install or
    remove software.

  After making your changes, press the Apply
  button at the bottom of the window.
"
	:x                      30
	:y                      10
	:width                  293
	:height                 23
	:value-x                101
	:value-y                10
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Category:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Source Media" "Package Administration" "Current Product Category" "Browser Display" "Remote Hosts" )
	:choice-label-types     (:string :string :string :string :string )
	:choice-colors          ("" "" "" "" "" )
	:choice-defaults        (nil nil nil nil nil )
	:initial-selections     (t nil nil nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (PropsWin PropsSet "Source Media")
		:when                   (Notify )
		:to                     (PropsWin)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ShowProperties(PROPS_MEDIA, (caddr_t)0);")
		)
		(
		:from                   (PropsWin PropsSet "Package Administration")
		:when                   (Notify )
		:to                     (PropsWin)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ShowProperties(PROPS_ADMIN, (caddr_t)0);")
		)
		(
		:from                   (PropsWin PropsSet "Current Product Category")
		:when                   (Notify )
		:to                     (PropsWin)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ShowProperties(PROPS_CATEGORY, (caddr_t)0);")
		)
		(
		:from                   (PropsWin PropsSet "Browser Display")
		:when                   (Notify )
		:to                     (PropsWin)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ShowProperties(PROPS_BROWSER, (caddr_t)0);")
		)
		(
		:from                   (PropsWin PropsSet "Remote Hosts")
		:when                   (Notify )
		:to                     (PropsWin)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ShowProperties(PROPS_HOSTS, (caddr_t)0);")
		)
	)
)
(
	:type                   :group
	:name                   PropsSetGrp
	:x                      30
	:y                      10
	:group-type             nil
	:members                (PropsSet )
	:rows                   0
	:row-alignment          :top-edges
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          PropsCtrl
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      30
	:vertical-offset        10
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   DispCtrl
	:owner                  PropsWin
	:help                   "Display Properties Control Panel
 
  For more detailed information about a specific
  control, use the mouse to position the cursor
  over the control of interest, then press the
  \"Help\" key.
"
	:x                      0
	:y                      46
	:width                  469
	:height                 539
	:background-color       ""
	:foreground-color       ""
	:initial-state          :invisible
	:show-border            t
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   DispIconic
	:owner                  DispCtrl
	:help                   "Display Format
 
Specifies representation of software modules in the software selection display.  The available formats are:
 
  Automatic
 
    If your vendor supplied any of the modules in
    the current display an icon, all modules are
    displayed primarily as icons.  If in this
    format any particular module does not have an
    icon, a generic icon is used.  If no modules
    have icons, all modules are primarily dis-
    played as text, with a small generic icon
    representing each module's type.  This mode is
    the default.
 
  Iconic
 
    Use this mode to force all modules to be
    displayed as icons.  If no icon is explicitly
    associated with a module, a generic icon is
    used.
 
  Textual
 
     Use this mode to force representation of
     modules primarily as names.  Each name is
     accompanied by a small generic icon that
     denotes the module's type (individual package
     or cluster) and an indication of the total
     amount of disk space required to install the
     module.
"
	:x                      64
	:y                      20
	:width                  213
	:height                 23
	:value-x                179
	:value-y                20
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Display Format:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Automatic" "Iconic" "Textual" )
	:choice-label-types     (:string :string :string )
	:choice-colors          ("" "" "" )
	:choice-defaults        (nil nil nil )
	:initial-selections     (t nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   DispName
	:owner                  DispCtrl
	:help                   "Name Length
 
Specifies the format of module names in the software selection area.  The two formats are as follows:
 
  Long
 
    Use this format to display the full name
    of each module.
 
  Short
 
    Use this format to display the abbreviated
    name of each module.  This format is useful
    if using multiple columns.
"
	:x                      78
	:y                      48
	:width                  169
	:height                 23
	:value-x                179
	:value-y                48
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Name Length:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Long" "Short" )
	:choice-label-types     (:string :string )
	:choice-colors          ("" "" )
	:choice-defaults        (nil nil )
	:initial-selections     (t nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   DispCols
	:owner                  DispCtrl
	:help                   "Number of Columns
 
Specifies the number of columns used in textual mode.
"
	:x                      30
	:y                      76
	:width                  193
	:height                 23
	:value-x                179
	:value-y                76
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Number of Columns:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("1" "2" "3" )
	:choice-label-types     (:string :string :string )
	:choice-colors          ("" "" "" )
	:choice-defaults        (nil nil nil )
	:initial-selections     (t nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   DispGrp
	:x                      30
	:y                      20
	:group-type             :column
	:members                (DispIconic DispName DispCols )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       5
	:anchor-object          DispCtrl
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      30
	:vertical-offset        20
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   DispApply
	:owner                  DispCtrl
	:help                   "Apply Button
 
Commits the current display settings and updates display.
 
  When you are ready to apply the current para-
  meters to the display, press the Apply button.
  Note that this does not save the settings to a
  file; this must be done using the Props menu
  items Save or Save As.
"
	:x                      96
	:y                      129
	:width                  53
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
		:from			(PropsWin DispApply)
		:when			(Notify )
		:to			(PropsWin DispApply)
		:function_type		ExecuteCode
		:arg_type		(:string)
		:action			("SetDisplayModes();")
		)
	)
)
(
	:type                   :button
	:name                   DispReset
	:owner                  DispCtrl
	:help                   "Reset Button
 
Resets settings to match display.
 
  Use this button to reset the display settings
  to reflect their current program values.  These
  values are those in force when you last pressed
  Apply, or those taken from the program's confi-
  guration file (if you have not yet applied any
  changes).
"
	:x                      159
	:y                      129
	:width                  51
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
		:from			(PropsWin DispReset)
		:when			(Notify )
		:to			(PropsWin DispReset)
		:function_type		ExecuteCode
		:arg_type		(:string)
		:action			("ResetDisplayModes();")
		)
	)
)
(
	:type                   :group
	:name                   DispButtons
	:x                      96
	:y                      129
	:group-type             :row
	:members                (DispApply DispReset )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          DispGrp
	:anchor-point           :south
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        30
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   LoadCtrl
	:owner                  PropsWin
	:help                   "Source Media Control Panel
 
  For more detailed information about a specific
  control, use the mouse to position the cursor
  over the control of interest, then press the
  \"Help\" key.
"
	:x                      0
	:y                      46
	:width                  453
	:height                 500
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:show-border            t
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   LoadType
	:owner                  LoadCtrl
	:help                   "Media Type
 
Specifies the physical format of the software distribution you intend to examine.  Currently, swmtool supports the following software distribution formats:
 
  Local CD-ROM
 
    Use this setting if you intend to examine a
    software distribution from a locally-attached
    CD-ROM device and have not yet mounted it as
    a local file system.
 
    This option will not work with devices
    controlled through volume management; use the
    Mounted Directory option instead.
 
  Mounted Directory
 
    Use this setting if your software distribution
    has already been locally mounted.  If your
    software is CD-based and you do not have a
    local CD-ROM device, you will need to use this
    setting and mount the disk manually using NFS.
"
	:x                      86
	:y                      30
	:width                  305
	:height                 23
	:value-x                176
	:value-y                30
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Media Type:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Local CD-ROM" "Mounted Directory" )
	:choice-label-types     (:string :string )
	:choice-colors          ("" "" )
	:choice-defaults        (t nil )
	:initial-selections     (t nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (PropsWin LoadType)
		:when                   (Notify )
		:to                     (PropsWin LoadType)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("SetMediaType((int)xv_get(ip->LoadType, PANEL_VALUE),
	    ip->LoadCtrl,
	    ip->LoadDev,
	    ip->LoadDir,
	    ip->LoadEject);")
		)
	)
)
(
	:type                   :button
	:name                   LoadEject
	:owner                  LoadCtrl
	:help                   "Eject Button
 
Ejects a Compact Disk from the specified device.
 
  To eject a compact disk, choose the media type
  CD-ROM, enter the name of the device in the
  Device Name field, then press the Eject button.
"
	:x                      346
	:y                      83
	:width                  47
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Eject"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (PropsWin LoadEject)
		:when                   (Notify )
		:to                     (PropsWin LoadEject)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("EjectMedia(
    LoadTypeToMediaType((int)xv_get(ip->LoadType, PANEL_VALUE)),
    (char *)xv_get(ip->LoadDev, PANEL_VALUE),
    (char *)xv_get(ip->LoadDir, PANEL_VALUE));")
		)
	)
)
(
	:type                   :text-field
	:name                   LoadDev
	:owner                  LoadCtrl
	:help                   "Device Name
 
Specifies the name of the device containing the software distribution.
 
  Use this field if you indicated your software
  is CD-based.  For a standard, local CD-ROM
  device, the default device, \"/dev/dsk/c0t6d0s0\",
  should suffice.  The device name you specify
  must not be mounted or must have been mounted
  from within swmtool.  Consult the documentation
  included with your product distribution for
  further information.
"
	:x                      76
	:y                      85
	:width                  260
	:height                 15
	:value-x                176
	:value-y                85
	:value-length           20
	:stored-length          1024
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Device Name:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          "/dev/dsk/c0t6d0s0"
	:initial-state          :active
	:read-only              nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   LoadDevGrp
	:x                      76
	:y                      83
	:group-type             :row
	:members                (LoadDev LoadEject )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   LoadDir
	:owner                  LoadCtrl
	:help                   "Directory Name
 
Specifies the name of the directory on which your software distribution should be mounted (Local CD-ROM) or the location of a mounted software distribution (Mounted Directory).
 
  If you indicated your software is CD-based,
  this value specifies the directory on which
  the CD-ROM device should be mounted.  This
  directory does not need to exist; swmtool will
  create it for you if you desire.  The directory
  name you specify must not be a mount point or
  must correspond to a mount point used within
  swmtool.
 
  If you indicated your software is already
  mounted, use this field to enter the path name
  of the distribution.  This path can be relative;
  however, swmtool does not recognize the home
  directory expansion character, tilde \"~\").
  Typing \"Return\" after entering your text causes
  the program to begin the load operation.
"
	:x                      59
	:y                      117
	:width                  277
	:height                 15
	:value-x                176
	:value-y                117
	:value-length           20
	:stored-length          1024
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Directory Name:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          "/cdrom"
	:initial-state          :active
	:read-only              nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (PropsWin LoadDir)
		:when                   (Notify )
		:to                     (PropsWin LoadDir)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("int	type = (int)xv_get(ip->LoadType, PANEL_VALUE);

if (type == 1) {	/* XXX directory */
	xv_set(ip->LoadApply, PANEL_BUSY, TRUE, NULL);
	if (LoadMedia(
	    LoadTypeToMediaType(type),
	    (char *)xv_get(ip->LoadDev, PANEL_VALUE),
	    (char *)xv_get(ip->LoadDir, PANEL_VALUE),
	    1) && get_mode() == MODE_INSTALL) {
		reset_selections(1, 1, 1);
		BrowseModules(MODE_UNSPEC, VIEW_UNSPEC);
	}
	xv_set(ip->LoadApply, PANEL_BUSY, FALSE, NULL);
}")
		)
	)
)
(
	:type                   :group
	:name                   LoadText
	:x                      59
	:y                      83
	:group-type             :column
	:members                (LoadDevGrp LoadDir )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       15
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   LoadParams
	:x                      59
	:y                      30
	:group-type             :column
	:members                (LoadType LoadText )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       20
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   LoadApply
	:owner                  LoadCtrl
	:help                   "Apply Button
 
Makes a software distribution available for installation.
 
  When you are ready to start examining a software
  distribution, press the Apply button.  If your
  distribution format is an unmounted CD-ROM, the
  program will attempt to mount the CD using the
  device name and mount point parameters you
  specified.  After mounting, or if you specified
  a mounted file system format, the distribution
  is examined for available software.
"
	:x                      165
	:y                      367
	:width                  53
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
		:from                   (PropsWin LoadApply)
		:when                   (Notify )
		:to                     (PropsWin LoadApply)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("if (LoadMedia(
    LoadTypeToMediaType((int)xv_get(ip->LoadType, PANEL_VALUE)),
    (char *)xv_get(ip->LoadDev, PANEL_VALUE),
    (char *)xv_get(ip->LoadDir, PANEL_VALUE),
    1) && get_mode() == MODE_INSTALL) {
	reset_selections(1, 1, 1);
	BrowseModules(MODE_UNSPEC, VIEW_UNSPEC);
}")
		)
	)
)
(
	:type                   :button
	:name                   LoadReset
	:owner                  LoadCtrl
	:help                   ""
	:x                      200
	:y                      367
	:width                  55
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
		:from                   (PropsWin LoadReset)
		:when                   (Notify )
		:to                     (PropsWin LoadReset)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ResetMedia(
    ip->LoadType, ip->LoadDev, ip->LoadEject, ip->LoadDir);")
		)
	)
)
(
	:type                   :group
	:name                   LoadButtons
	:x                      165
	:y                      367
	:group-type             :row
	:members                (LoadApply LoadReset )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     15
	:vertical-spacing       10
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   LoadMedia
	:x                      59
	:y                      30
	:group-type             :column
	:members                (LoadParams LoadButtons )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :vertical-centers
	:horizontal-spacing     10
	:vertical-spacing       30
	:anchor-object          LoadCtrl
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      30
	:vertical-offset        30
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   ConfigCtrl
	:owner                  PropsWin
	:help                   "Package Command Control Panel
 
  For more detailed information about a specific
  control, use the mouse to position the cursor
  over the control of interest, then press the
  \"Help\" key.
"
	:x                      0
	:y                      46
	:width                  469
	:height                 539
	:background-color       ""
	:foreground-color       ""
	:initial-state          :invisible
	:show-border            t
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :scrolling-list
	:name                   ConfigMailList
	:owner                  ConfigCtrl
	:help                   "Mail Recipients
 
Specifies recipients of installation and removal completion mail messages.
 
  The package commands \"pkgadd\" and \"pkgrm\" can
  send mail containing completion status to a list
  of addresses.  If this list is empty, no mail
  will be sent.  Use the Add, Delete, and Change
  buttons in conjunction with the text field
  Recipient Name to modify the contents of the
  list.
"
	:x                      31
	:y                      20
	:width                  250
	:height                 74
	:value-x                149
	:value-y                20
	:rows                   3
	:foreground-color       ""
	:label                  "Mail Recipients:"
	:title                  ""
	:label-type             :string
	:layout-type            :horizontal
	:read-only              t
	:multiple-selections    nil
	:selection-required     nil
	:initial-state          :active
	:droppable              nil
	:default-drop-site      nil
	:menu                   MailMenu
	:notify-handler         nil
	:event-handler          nil
	:initial-list-values    ()
	:initial-list-glyphs    ()
	:initial-selections     ()
	:user-data              ()
	:actions                (
		(
		:from                   (PropsWin ConfigMailList)
		:when                   (Notify )
		:to                     (PropsWin ConfigMailList)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("if (op == PANEL_LIST_OP_SELECT)
	xv_set(ip->ConfigRecip, PANEL_VALUE, string, NULL);")
		)
	)
)
(
	:type                   :button
	:name                   ConfigMailAdd
	:owner                  ConfigCtrl
	:help                   "Add Button
 
Adds an address to the recipient list.
 
  To add an address to the list, click SELECT
  on the address after which you wish to add the
  new recipient.  This highlights the selected
  address.  If you do not select an address or
  the list is empty, the new address will be
  added to the end of the list.  Next, enter
  the name of the new recipient after Recipient
  Name.  Finally, press the Add button.  You
  should see the new address added to the list.
"
	:x                      133
	:y                      104
	:width                  42
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Add"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (PropsWin ConfigMailAdd)
		:when                   (Notify )
		:to                     (PropsWin ConfigMailAdd)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("AddMail();")
		)
	)
)
(
	:type                   :button
	:name                   ConfigMailDelete
	:owner                  ConfigCtrl
	:help                   "Delete Button
 
Deletes an address from the recipient list.
 
  To delete an address from the list, click SELECT
  on the address you wish to remove.  This high-
  lights the selected address.  Next, press the
  Delete button.  You should see the address
  removed from the list.
"
	:x                      185
	:y                      104
	:width                  57
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Delete"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (PropsWin ConfigMailDelete)
		:when                   (Notify )
		:to                     (PropsWin ConfigMailDelete)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("DeleteMail();")
		)
	)
)
(
	:type                   :button
	:name                   ConfigMailChange
	:owner                  ConfigCtrl
	:help                   "Change Button
 
Modifies an address in the recipient list.
 
  To change an address already in the list,
  click SELECT on the address you wish to modify.
  This will highlight the selected address.  Next,
  enter the new address in the Recipient Name
  field.  Finally, press the Change button.  You
  should see the new address appear in place of
  the old one in the recipient list.
"
	:x                      252
	:y                      104
	:width                  64
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Change"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (PropsWin ConfigMailChange)
		:when                   (Notify )
		:to                     (PropsWin ConfigMailChange)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ChangeMail();")
		)
	)
)
(
	:type                   :group
	:name                   ConfigMailButtons
	:x                      133
	:y                      104
	:group-type             :row
	:members                (ConfigMailAdd ConfigMailDelete ConfigMailChange )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          nil
	:anchor-point           :south
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        10
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   ConfigMailListGrp
	:x                      31
	:y                      20
	:group-type             :column
	:members                (ConfigMailList ConfigMailButtons )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :vertical-centers
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   ConfigRecip
	:owner                  ConfigCtrl
	:help                   "Recipient Name:
 
Specifies a recipient name.
 
  Use the Recipient Name field in conjunction
  with the Add and Change buttons to add new
  addresses to the recipient list and modify
  existing recipient addresses, respectively.
  After selecting an entry from the recipient
  list, enter the new recipient name, then
  press the button corresponding to the desired
  function.
"
	:x                      30
	:y                      133
	:width                  359
	:height                 15
	:value-x                149
	:value-y                133
	:value-length           30
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Recipient Name:"
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
	:type                   :group
	:name                   ConfigMailGrp
	:x                      30
	:y                      20
	:group-type             :column
	:members                (ConfigMailListGrp ConfigRecip )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   ConfigConflict
	:owner                  ConfigCtrl
	:help                   "Existing Files
 
Controls attempts to overwrite existing files.  The options are as follows:
 
  Ask
 
    Use this option if you would like the package
    installation software to prompt you before
    overwriting existing files.
 
  Overwrite
 
    Use this option if you would like the package
    software to always replace existing files with
    the version contained in the package.
 
  Skip
 
    Use this option if you would like the package
    software to install new files but skip instal-
    lation of files that already exist.
 
  Abort
 
    Use this option if you would like the package
    software to abort without making any changes
    to the system if it detects conflicts with
    existing files.
"
	:x                      185
	:y                      163
	:width                  200
	:height                 23
	:value-x                288
	:value-y                163
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Existing Files:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Ask" "Overwrite" "Skip" "Abort" )
	:choice-label-types     (:string :string :string :string )
	:choice-colors          ("" "" "" "" )
	:choice-defaults        (t nil nil nil )
	:initial-selections     (t nil nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   ConfigInstance
	:owner                  ConfigCtrl
	:help                   "Existing Packages
 
Controls attempts to install packages more than once.  The options are as follows:
 
  Ask
 
    Use this option if you would like the package
    software to prompt you before attempting to
    install a package that has already been
    installed (this check is based on package
    name).
 
  Overwrite
 
    Use this option if you would like the package
    software to install packages in place of any
    previous installations.  Note that you are
    only allowed to re-install the package \"on
    top\" of the old one.  You are not allowed to
    install the package in a different location.
 
  Install Unique
 
    Use this option if you would like the package
    software to install packages as uniquely-
    named instances, should they be installed
    already.  Note that this option may overwrite
    existing files.  This option is useful if you
    are installing packages that are architecture-
    or locale-specific pieces of a product, or you
    wish to install a new version of a previously-
    installed package in a different location.
 
  Abort
 
    Use this option if you would like the package
    software to abort without making any changes
    to the system should you attempt to install
    a package more than once.
"
	:x                      155
	:y                      191
	:width                  254
	:height                 23
	:value-x                288
	:value-y                191
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Existing Packages:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Ask" "Overwrite" "Install Unique" "Abort" )
	:choice-label-types     (:string :string :string :string )
	:choice-colors          ("" "" "" "" )
	:choice-defaults        (t nil nil nil )
	:initial-selections     (t nil nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   ConfigPartial
	:owner                  ConfigCtrl
	:help                   "Existing Partial Installation
 
Controls attempts to re-install a partially-installed package.  The options are as follows:
 
  Ask
 
    Use this option if you would like the package
    software to prompt you before attempting to
    re-install a partially-installed package.
 
  Ignore
 
    Use this option if you would like the package
    software to ignore previous attempts to
    install the package, overwriting any previous
    partial installations.
 
  Abort
 
    Use this option if you would like the package
    software to abort if it detects a previous
    partial installation.  In this case, it will
    not make any changes to the system.
"
	:x                      89
	:y                      219
	:width                  273
	:height                 23
	:value-x                288
	:value-y                219
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Existing Partial Installation:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Ask" "Ignore" "Abort" )
	:choice-label-types     (:string :string :string )
	:choice-colors          ("" "" "" )
	:choice-defaults        (t nil nil )
	:initial-selections     (t nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   ConfigSetuid
	:owner                  ConfigCtrl
	:help                   "Install Setuid/Setgid Files
 
Controls attempts to install files with set-ID bits activated.  The options are as follows:
 
  Ask
 
    Use this option if you would like the package
    software to prompt you before installing a
    file with setuid or setgid bits activated.
 
  Yes
 
    Use this option if you would like the package
    software to install files regardless of the
    states of their setuid and setgid bits.
 
  No
 
    Use this option if you would like the package
    software to install setuid and setgid files
    with their setuid and setgid bits turned off.
 
  Abort
 
    Use this option if you would like the package
    software to abort without making any changes
    to the system if it detects an attempt to
    install setuid or setgid files.
"
	:x                      99
	:y                      247
	:width                  258
	:height                 23
	:value-x                288
	:value-y                247
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Install Setuid/Setgid Files:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Ask" "Yes" "No" "Abort" )
	:choice-label-types     (:string :string :string :string )
	:choice-colors          ("" "" "" "" )
	:choice-defaults        (t nil nil nil )
	:initial-selections     (t nil nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   ConfigAction
	:owner                  ConfigCtrl
	:help                   "Run Setuid/Setgid Scripts
 
Controls attempts to run scripts with set-ID bits activated.  The options are as follows:
 
  Ask
 
    Some packages contain scripts to be executed
    after the package is installed or removed.
    These \"action\" scripts potentially impact
    system security if their setuid or setgid bits
    are enabled.  Use the Ask option if you would
    like the package software to prompt you before
    executing a setuid or setgid action script.
 
  Yes
 
    Use this option if you would like the package
    software to install packages regardless of the
    security impact of their action scripts.
 
  Abort
 
    Use this option if you would like the package
    software to abort without making any changes
    to the system if it encounters a package
    containing setuid or setgid action scripts.
"
	:x                      102
	:y                      275
	:width                  255
	:height                 23
	:value-x                288
	:value-y                275
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Run Setuid/Setgid Scripts:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Ask" "Yes" "Abort" )
	:choice-label-types     (:string :string :string )
	:choice-colors          ("" "" "" )
	:choice-defaults        (t nil nil )
	:initial-selections     (t nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   ConfigIdepend
	:owner                  ConfigCtrl
	:help                   "Installation Dependencies Not Met
 
Controls attempts to install packages without satisfying all prerequisites.  The options are as follows:
 
  Ask
 
    Use this option if you would like the package
    software to prompt you before installing a
    package with unsatisfied dependencies (not all
    prerequisite packages installed).
 
  Ignore
 
    Use this option if you would like the package
    software to install a package even if all
    packages on which it depends have not been
    installed.
 
  Abort
 
    Use this option if you would like the package
    software to abort without making any changes
    to the system if prerequisite packages are not
    completely installed.
"
	:x                      40
	:y                      303
	:width                  322
	:height                 23
	:value-x                288
	:value-y                303
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Installation Dependencies Not Met:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Ask" "Ignore" "Abort" )
	:choice-label-types     (:string :string :string )
	:choice-colors          ("" "" "" )
	:choice-defaults        (t nil nil )
	:initial-selections     (t nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   ConfigRdepend
	:owner                  ConfigCtrl
	:help                   "Removal Dependencies Not Met
 
Controls attempts to remove packages depended upon by others.  The options are as follows:
 
  Ask
 
    Use this option if you would like the package
    software to prompt you before removing a
    package on which other packages depend.
 
  Ignore
 
    Use this option if you would like the package
    software to remove a package even if it is
    depended upon by other packages.
 
  Abort
 
    Use this option if you would like the package
    software to abort without making any changes
    to the system if you attempt to remove a
    package depended upon by other packages.
"
	:x                      61
	:y                      331
	:width                  301
	:height                 23
	:value-x                288
	:value-y                331
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Removal Dependencies Not Met:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Ask" "Ignore" "Abort" )
	:choice-label-types     (:string :string :string )
	:choice-colors          ("" "" "" )
	:choice-defaults        (t nil nil )
	:initial-selections     (t nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   ConfigRunlevel
	:owner                  ConfigCtrl
	:help                   "Incorrect Run Level
 
Controls attempts to install packages while the system run level does not match that specified by the package.  The options are as follows:
 
  Ask
 
    Use this option if you would like the package
    software to prompt you before installing a
    package with the system at a run level not
    specified in the package.
 
  Ignore
 
    Use this option if you would like the package
    software to install packages regardless of the
    current or specified run levels (installation
    of the package is still subject to other con-
    sistency checks).
 
  Abort
 
    Use this option if you would like the package
    software to abort without making any changes
    to the system if you attempt to install a
    package when the system's run level does not
    match one of those specified by the package.
"
	:x                      144
	:y                      359
	:width                  218
	:height                 23
	:value-x                288
	:value-y                359
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Incorrect Run Level:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Ask" "Ignore" "Abort" )
	:choice-label-types     (:string :string :string )
	:choice-colors          ("" "" "" )
	:choice-defaults        (t nil nil )
	:initial-selections     (t nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   ConfigSpace
	:owner                  ConfigCtrl
	:help                   "Insufficient Space
 
Controls attempts to install packages for which there is insufficient space.  The options are as follows:
 
  Ask
 
    Use this option if you would like the package
    software to prompt you before installing a
    package for which there is not enough disk
    space.
 
  Ignore
 
    Use this option if you would like the package
    software to install packages regardless of the
    amount of disk space available.
 
  Abort
 
    Use this option if you would like the package
    software to abort without making any changes
    to the system if you attempt to install a
    package for which there is not enough disk
    space.
"
	:x                      151
	:y                      387
	:width                  211
	:height                 23
	:value-x                288
	:value-y                387
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Insufficient Space:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Ask" "Ignore" "Abort" )
	:choice-label-types     (:string :string :string )
	:choice-colors          ("" "" "" )
	:choice-defaults        (t nil nil )
	:initial-selections     (t nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   ConfigVerbose
	:owner                  ConfigCtrl
	:help                   "Show Copyrights
 
Controls display of verbose messages such as copyrights during package installation.
 
  Usually each software package is accompanied
  by a rather long copyright notice which is
  displayed when the package is installed.  This
  message can take a significant amount of time
  and resources to display.  If you wish to see
  these copyright notices, select Yes, otherwise
  select No.
"
	:x                      162
	:y                      415
	:width                  183
	:height                 23
	:value-x                288
	:value-y                415
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Show Copyrights:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Yes" "No" )
	:choice-label-types     (:string :string )
	:choice-colors          ("" "" )
	:choice-defaults        (nil nil )
	:initial-selections     (t nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :setting
	:name                   ConfigInteract
	:owner                  ConfigCtrl
	:help                   "Install/Remove Interactively
 
Controls ability to interact with package installation and removal commands.
 
  Package installation and removal can either be
  interactive or non-interactive.  if run inter-
  actively, the underlying package software will
  prompt you for appropriate action should any of
  several error or abnormal conditions arise.  If
  you disable interactivity, these conditions
  result in the failure of the package operation
  in progress (other operations may proceed).  If
  you would like to execute package installation
  and removal commands interactively, select Yes,
  otherwise select No.
"
	:x                      80
	:y                      443
	:width                  265
	:height                 23
	:value-x                288
	:value-y                443
	:rows                   0
	:columns                1
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :stack
	:selection-required     t
	:label                  "Install/Remove Interactively:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Yes" "No" )
	:choice-label-types     (:string :string )
	:choice-colors          ("" "" )
	:choice-defaults        (nil nil )
	:initial-selections     (t nil )
	:initial-state          :active
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   ConfigParams
	:x                      40
	:y                      163
	:group-type             :column
	:members                (ConfigConflict ConfigInstance ConfigPartial ConfigSetuid ConfigAction ConfigIdepend ConfigRdepend ConfigRunlevel ConfigSpace ConfigVerbose ConfigInteract )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       5
	:anchor-object          nil
	:anchor-point           :south
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        20
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   ConfigGrp
	:x                      30
	:y                      20
	:group-type             :column
	:members                (ConfigMailGrp ConfigParams )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :vertical-centers
	:horizontal-spacing     10
	:vertical-spacing       15
	:anchor-object          ConfigCtrl
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      30
	:vertical-offset        20
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   ConfigApply
	:owner                  ConfigCtrl
	:help                   "Apply Button
 
Commits current package control settings for use during software installation and removal.
 
  When you are ready to commit the current package
  control parameters to the program, press the
  Apply button.  Subsequent installations and
  removals during this session will use the values
  you just applied.  Note that this does not save
  the settings to a file; this must be done using
  the File menu items Save or Save As.
"
	:x                      167
	:y                      496
	:width                  53
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
		:from                   (PropsWin ConfigApply)
		:when                   (Notify )
		:to                     (PropsWin ConfigApply)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("SetAdmin();")
		)
	)
)
(
	:type                   :button
	:name                   ConfigReset
	:owner                  ConfigCtrl
	:help                   "Reset Button
 
Resets package control settings to match current program values.
 
  Use this button to reset the package control
  settings to reflect their current program
  values.  These values are those in force when
  you last pressed Apply or those taken from the
  program's configuration file (if you have not
  yet applied any changes).
"
	:x                      200
	:y                      496
	:width                  51
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
		:from                   (PropsWin ConfigReset)
		:when                   (Notify )
		:to                     (PropsWin ConfigReset)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("GetAdmin();")
		)
	)
)
(
	:type                   :group
	:name                   ConfigButtons
	:x                      167
	:y                      496
	:group-type             :row
	:members                (ConfigApply ConfigReset )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :vertical-centers
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          ConfigGrp
	:anchor-point           :south
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        30
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   CatCtrl
	:owner                  PropsWin
	:help                   "Current Category Control Panel
 
  For more detailed information about a specific
  control, use the mouse to position the cursor
  over the control of interest, then press the
  \"Help\" key.
"
	:x                      0
	:y                      46
	:width                  469
	:height                 539
	:background-color       ""
	:foreground-color       ""
	:initial-state          :invisible
	:show-border            t
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :scrolling-list
	:name                   CatList
	:owner                  CatCtrl
	:help                   "Categories List
 
Lists the software categories to which all modules in the currently active software distribution belong.
 
  Software vendors have the option of attaching
  arbitrary \"category\" names to the modules in
  their distributions.  You may use these cate-
  gories to limit the number of modules displayed
  in the software selection window by selecting
  the desired category (the selected category will
  appear highlighted), then pressing the Apply
  button.  Only modules belonging to the current
  category will be displayed.  To display all
  modules, select All Software from the list,
  then press Apply.
"
	:x                      30
	:y                      20
	:width                  200
	:height                 200
	:value-x                114
	:value-y                20
	:rows                   10
	:foreground-color       ""
	:label                  "Categories:"
	:title                  ""
	:label-type             :string
	:layout-type            :horizontal
	:read-only              t
	:multiple-selections    nil
	:selection-required     t
	:initial-state          :active
	:droppable              nil
	:default-drop-site      nil
	:menu                   HostExMenu
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
	:name                   CatListGroup
	:x                      30
	:y                      20
	:group-type             :column
	:members                (CatList )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :vertical-centers
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          CatCtrl
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      30
	:vertical-offset        20
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   CatApply
	:owner                  CatCtrl
	:help                   "Apply Button
 
Commits the selected (highlighted) name as the currently active category.
 
  To limit the number of products displayed in
  the software selection area, select a category
  from the Categories list, then press Apply.
"
	:x                      124
	:y                      250
	:width                  53
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
		:from                   (PropsWin CatApply)
		:when                   (Notify )
		:to                     (PropsWin CatApply)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("SetCategory();
BrowseModules(MODE_UNSPEC, VIEW_UNSPEC);")
		)
	)
)
(
	:type                   :button
	:name                   CatReset
	:owner                  CatCtrl
	:help                   "Reset Button
 
Resets the Categories list such that the current category is selected.
 
  To make the selection indication in the
  Categories list match the current category,
  press the Reset button.  This operation does
  not change the current category.
"
	:x                      187
	:y                      250
	:width                  51
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
		:from                   (PropsWin CatReset)
		:when                   (Notify )
		:to                     (PropsWin CatReset)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ResetCategory();")
		)
	)
)
(
	:type                   :group
	:name                   CatButtonGroup
	:x                      100
	:y                      250
	:group-type             :row
	:members                (CatApply CatReset )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :right-edges
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          CatListGroup
	:anchor-point           :south
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        30
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   HostCtrl
	:owner                  PropsWin
	:help                   "Host Selection Control Panel
 
  For more detailed information about a specific
  control, use the mouse to position the cursor
  over the control of interest, then press the
  \"Help\" key.
"
	:x                      0
	:y                      46
	:width                  469
	:height                 539
	:background-color       ""
	:foreground-color       ""
	:initial-state          :invisible
	:show-border            t
	:menu                   nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   HostEdit
	:owner                  HostCtrl
	:help                   "Edit Button

Controls the contents of the target host list.  The Edit button provides access to a menu containing the following commands:

  Add Entry

    Creates a new target host entry initialized
    from the contents of the Host Name and Root
    Password fields.  The command's pull-right
    sub-menu controls where in the list the new
    entry is created (at the top, the bottom,
    before or after the currently-selected
    entry).

    By default, pressing SELECT on the Edit
    button adds a new host entry to the end of
    the list.

    Note that hosts are not marked as selected
    for installation and removal by default;
    this is accomplished by using the Toggle
    Selection button.

  Modify Entry

    Replaces the name and password associated
    with the currently-selected host entry with
    the values of the Host Name and Root Password
    fields.

  Delete Entry

    Removes the currently-selected entry from
    the list of target hosts.

  When ready to commit your changes, press the
  Apply button.
"
	:x                      102
	:y                      20
	:width                  56
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Edit"
	:label-type             :string
	:initial-state          :active
	:menu                   HostEditMenu
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :scrolling-list
	:name                   HostList
	:owner                  HostCtrl
	:help                   "Host List
 
Lists hosts on which software can be installed or removed.
 
  The target host list specifies hosts on which
  swmtool can install or remove software.  Each
  list entry contains three fields:  a glyph
  indicating the host's system status (up, down,
  requires a root password, or unknown), its name,
  and its selection status.

  Pressing SELECT while positioned over one of
  the entries in the list makes that entry the
  current selection and the target of subsequent
  Edit and Toggle Selection commands.

  For information about adding, removing, or
  modifying host entries, press the \"Help\" key
  while the cursor is positioned over the Edit
  button.

  For information about changing a host's
  selection status, press the \"Help\" key while
  positioned over the Toggle Selection button.
"
	:x                      30
	:y                      49
	:width                  400
	:height                 163
	:value-x                30
	:value-y                66
	:rows                   6
	:foreground-color       ""
	:label                  "Target Hosts:"
	:title                  ""
	:label-type             :string
	:layout-type            :vertical
	:read-only              t
	:multiple-selections    nil
	:selection-required     t
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
	:actions                (
		(
		:from                   (PropsWin HostList)
		:when                   (Notify )
		:to                     (PropsWin HostList)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action			("if (op == PANEL_LIST_OP_SELECT)
	SetSelectedHost((Xv_opaque)ip, item, row, client_data);")
		)
	)
)
(
	:type                   :button
	:name                   HostToggle
	:owner                  HostCtrl
	:help                   "Toggle Selection

Toggles the selection status of the currently-selected host.

  To enable software installation and removal
  on a host, mark it as SELECTED by following
  these steps:

    1) Position the cursor over the entry corres-
      ponding to the host you wish to select and
      press the SELECT mouse button.

    2) Press the Toggle Selection button.

    3) Press Apply to commit your selection(s).

  For information about adding, removing, and
  modifying host entries, press the \"Help\" key
  while the cursor is positioned over the Edit
  button.
"
	:x                      131
	:y                      222
	:width                  117
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Toggle Selection"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (PropsWin HostToggle)
		:when                   (Notify )
		:to                     (PropsWin HostToggle)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action			("ToggleHost((Xv_opaque)ip);")
		)
	)
)
(
	:type                   :group
	:name                   HostListGrp
	:x                      30
	:y                      49
	:group-type             :column
	:members                (HostList HostToggle )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :vertical-centers
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   HostGrp
	:x                      30
	:y                      20
	:group-type             :column
	:members                (HostEdit HostListGrp )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :left-edges
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          HostCtrl
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      30
	:vertical-offset        20
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   HostName
	:owner                  HostCtrl
	:help                   "Host Name
 
Specifies the name of a new host.
 
  Use this field in conjunction with the Add and
  Change buttons to add and modify host entries,
  respectively.
"
	:x                      55
	:y                      261
	:width                  245
	:height                 15
	:value-x                140
	:value-y                261
	:value-length           20
	:stored-length          64
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Host Name:"
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
	:type                   :button
	:name                   HostPwdClear
	:owner                  HostCtrl
	:help                   "Clear Button

Clears the existing value in the Root Password field.

  To guard against eavesdropping, root passwords
  are not echoed in the Root Password field.
  The Clear button provides both an indication
  that the Root Password field is non-empty and
  the means to erase the field's value.  Pressing
  this button clears the field and de-activates
  the button.  The button will be re-activated
  when you enter a value in the Password field.
"
	:x                      310
	:y                      286
	:width                  50
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Clear"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (PropsWin HostPwdClear)
		:when                   (Notify )
		:to                     (PropsWin HostPwdClear)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("xv_set(ip->HostPwd, PANEL_VALUE, "", NULL);
xv_set(item, PANEL_INACTIVE, TRUE, NULL);")
		)
	)
)
(
	:type                   :text-field
	:name                   HostPwd
	:owner                  HostCtrl
	:help                   "Root Password:
 
Specifies the root password for a host.
 
  In order to execute programs on remote systems,
  swmtool requires either a .rhosts entry on the
  remote system permitting unrestricted root
  access from this system, or that you know the
  remote system's root password.  If a root
  password is required, you will be notified
  by pop-up notices and the \"password required\"
  glyph (a circle with a diagonal line) in the
  host list.
 
  To add a password to an existing host entry,
  select the desired name from the host list,
  enter the password in the Root Password field,
  then select the Change Entry command from the
  Edit menu.  If you are creating a new host
  entry and know in advance you will need a
  password, you may enter the password in con-
  junction with the host's name, prior to
  executing the Add Entry function.
 
  To guard against eavesdropping, the password
  is not echoed as you enter characters.  Use
  the Clear button to determine whether the
  Root Password field contains a value; if the
  button is active the field contains a value.
  The existing value will be automatically
  re-used.  If you need to enter a different
  value, you may use the line kill character,
  the Delete key, or the Clear button to erase
  the previous value.
"
	:x                      30
	:y                      288
	:width                  270
	:height                 15
	:value-x                140
	:value-y                288
	:value-length           20
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Root Password:"
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
		:from                   (PropsWin HostPwd)
		:when                   (AnyEvent )
		:to                     (PropsWin HostPwd)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("if (event_is_up(event)) {
	char *passwd = (char *)xv_get(item, PANEL_VALUE);
	int inactive = (int)xv_get(ip->HostPwdClear, PANEL_INACTIVE);

	if (passwd && *passwd) {
		if (inactive == TRUE)
			xv_set(ip->HostPwdClear, PANEL_INACTIVE, FALSE, NULL);
	} else {
		if (inactive == FALSE)
			xv_set(ip->HostPwdClear, PANEL_INACTIVE, TRUE, NULL);
	}
}")
		)
	)
)
(
	:type                   :group
	:name                   HostPwdGrp
	:x                      30
	:y                      286
	:group-type             :row
	:members                (HostPwd HostPwdClear )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          nil
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   HostParmGrp
	:x                      30
	:y                      261
	:group-type             :column
	:members                (HostName HostPwdGrp )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          HostGrp
	:anchor-point           :south-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        20
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   HostApply
	:owner                  HostCtrl
	:help                   "Apply Button
 
Commits the host list and host selections to the program.
 
  When you are ready to commit the current host
  list and selections to the program, press the
  Apply button.  Subsequent installations and
  removals during this session will operate on
  the hosts selected in the list, in the order
  in which they appear in the list. Note that
  this does not save the list to a file; this
  must be done using the File menu items Save
  or Save As.
"
	:x                      138
	:y                      335
	:width                  53
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
		:from                   (PropsWin HostApply)
		:when                   (Notify )
		:to                     (PropsWin HostApply)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("SetHosts();")
		)
	)
)
(
	:type                   :button
	:name                   HostReset
	:owner                  HostCtrl
	:help                   "Reset Button
 
Resets the host list and host selection to their program values.
 
  Use this button to reset the host list and
  selections to reflect their current program
  values.  These values are those in force when
  you last pressed Apply or those taken from the
  program's configuration file (if you have not
  yet applied any changes).  Note that the file-
  based configuration does not save selection
  status or root passwords.
"
	:x                      201
	:y                      335
	:width                  51
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
		:from                   (PropsWin HostReset)
		:when                   (Notify )
		:to                     (PropsWin HostReset)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("GetHosts();")
		)
	)
)
(
	:type                   :group
	:name                   HostLowButtons
	:x                      138
	:y                      335
	:group-type             :row
	:members                (HostApply HostReset )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          HostParmGrp
	:anchor-point           :south
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        30
	:user-data              ()
	:actions                ()
)
)
(
	:type                   :stack
	:name                   PropsStack
	:owner                  PropsWin
	:members                (LoadCtrl ConfigCtrl DispCtrl CatCtrl HostCtrl )
	:user-data              ()
)
)
