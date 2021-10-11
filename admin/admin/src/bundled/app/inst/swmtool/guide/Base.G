;GIL-3
(
(
	:type                   :menu
	:name                   FileMenu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "File"
	:menu-item-labels       ("Save" "Save As..." )
	:menu-item-label-types  (:string :string )
	:menu-item-states       (:active :active )
	:menu-item-defaults     (t nil )
	:initial-selections     (nil nil )
	:menu-item-handlers     (nil nil )
	:menu-item-menus        (nil nil )
	:menu-item-colors       ("" "" )
	:pinnable               t
	:user-data              ()
	:actions                (
		(
		:from                   (FileMenu "Save")
		:when                   (Notify )
		:to                     (FileMenu "Save")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ConfigFile(CONFIG_SAVE);")
		)
		(
		:from                   (FileMenu "Save As...")
		:when                   (Notify )
		:to                     (FileMenu "Save As...")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ConfigFile(CONFIG_SAVEAS);")
		)
	)
)
(
	:type                   :menu
	:name                   ViewMenu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "View"
	:menu-item-labels       ("Prev Level" "Command Window..." "Space Meter..." )
	:menu-item-label-types  (:string :string :string )
	:menu-item-states       (:active :active :active )
	:menu-item-defaults     (t nil nil )
	:initial-selections     (nil nil nil )
	:menu-item-handlers     (nil nil nil )
	:menu-item-menus        (nil nil nil )
	:menu-item-colors       ("" "" "" )
	:pinnable               t
	:user-data              ()
	:actions                (
		(
		:from                   (ViewMenu "Prev Level")
		:when                   (Notify )
		:to                     (ViewMenu "Prev Level")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("Module *prev = GetPreviousLevel();

if (prev != (Module *)0)
	SetCurrentLevel((char *)0, prev);")
		)
	)
)
(
	:type                   :menu
	:name                   EditMenu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Edit"
	:menu-item-labels       ("Native Environment" "Add Client Support..." "" "Select All" "Deselect All" )
	:menu-item-label-types  (:string :string :string :string :string )
	:menu-item-states       (:active :active :inactive :active :active )
	:menu-item-defaults     (t nil nil nil nil )
	:initial-selections     (t nil nil nil nil )
	:menu-item-handlers     (nil nil nil nil nil )
	:menu-item-menus        (nil nil nil nil nil )
	:menu-item-colors       ("" "" "" "" "" "" "" )
	:pinnable               t
	:user-data              ()
	:actions                (
		(
		:from                   (EditMenu "Select All")
		:when                   (Notify )
		:to                     (EditMenu "Select All")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("SelectAllModules(1);")
		)
		(
		:from                   (EditMenu "Deselect All")
		:when                   (Notify )
		:to                     (EditMenu "Deselect All")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("SelectAllModules(0);")
		)
		(
		:from                   (EditMenu "Native Environment")
		:when                   (Notify )
		:to                     (EditMenu "Native Environment")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("if (get_view() != VIEW_NATIVE) {
	set_installed_media((Module *)0);
	BrowseModules(MODE_UNSPEC, VIEW_NATIVE);
}")
		)
		(
		:from                   (EditMenu "Add Client Support...")
		:when                   (Notify )
		:to                     (EditMenu "Add Client Support...")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("InitClientInfo((Xv_opaque)Client_ClientWin, MODE_INSTALL);")
		)
	)
)
(
	:type                   :menu
	:name                   BaseMenu
	:help                   ""
	:columns                1
	:menu-type              :command
	:menu-handler           nil
	:menu-title             "Software"
	:menu-item-labels       ("Package Properties..." "Select Localizations..." "Read About" "Run" "Set Base Directory..." "Expand" )
	:menu-item-label-types  (:string :string :string :string :string :string )
	:menu-item-states       (:active :active :active :active :active :active )
	:menu-item-defaults     (t nil nil nil nil nil )
	:initial-selections     (t nil nil nil nil nil )
	:menu-item-handlers     (nil nil nil nil nil nil )
	:menu-item-menus        (nil nil nil nil nil nil )
	:menu-item-colors       ("" "" "" "" "" "" )
	:pinnable               t
	:user-data              ()
	:actions                (
		(
		:from                   (BaseMenu "Package Properties...")
		:when                   (Notify )
		:to                     (BaseMenu "Package Properties...")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("GetPackageInfo(GetCurrentModule());")
		)
		(
		:from                   (BaseMenu "Select Localizations...")
		:when                   (Notify )
		:to                     (BaseMenu "Select Localizations...")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("GetProductInfo(GetCurrentModule(), 1);")
		)
		(
		:from                   (BaseMenu "Set Base Directory...")
		:when                   (Notify )
		:to                     (BaseMenu "Set Base Directory...")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("GetPackageInfo(GetCurrentModule());")
		)
		(
		:from                   (BaseMenu "Expand")
		:when                   (Notify )
		:to                     (BaseMenu "Expand")
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("Module *mod = GetCurrentModule();

if (mod != (Module *)0)
	SetCurrentLevel((char *)0, mod);")
		)
	)
)
(
	:type                   :base-window
	:name                   BaseWin
	:owner                  nil
	:width                  478
	:height                 325
	:background-color       ""
	:foreground-color       ""
	:label                  "Software Manager"
	:label-type             :string
	:initial-state          :open
	:show-footer            t
	:resizable              t
	:icon-file              "icons/swmtool.icon"
	:icon-label             ""
	:icon-mask-file         ""
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (BaseWin)
		:when                   (Resize )
		:to                     (BaseWin)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("BaseResize((Xv_opaque)ip);")
		)
	)
)
(
	:type                   :control-area
	:name                   BaseCtrl
	:owner                  BaseWin
	:help                   "Main Control Panel
 
  For more detailed information about a specific
  control, use the mouse to position the cursor
  over the control of interest, then press the
  \"Help\" key.
"
	:x                      0
	:y                      0
	:width                  478
	:height                 61
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
	:name                   BaseMode
	:owner                  BaseCtrl
	:help                   "Mode Setting
 
Toggles program mode from installation to removal and vice versa.  The two modes are:
 
  Install
 
    Select this mode if you wish to install soft-
    ware.  You will need to specify the location
    of a software distribution, either by using
    the Props button and Source Media category, or
    by specifying an initial directory with the
    command-line option \"-d.\"  Use the software
    selection area at the bottom of the window to
    select modules to be installed, then press the
    Apply button to initiate the installation(s).
 
  Remove
 
    Select this mode if you wish to remove
    software.  After pressing this button, the
    software selection area at the bottom of the
    window will contain a representation of all
    software packages installed on the local
    system.  Use this area to select the packages
    you wish to remove, then press the Apply
    button to initiate the removal(s).
"
	:x                      303
	:y                      7
	:width                  165
	:height                 23
	:value-x                350
	:value-y                7
	:rows                   1
	:columns                0
	:layout-type            :horizontal
	:foreground-color       ""
	:setting-type           :exclusive
	:selection-required     t
	:label                  "Mode:"
	:label-type             :string
	:notify-handler         nil
	:event-handler          nil
	:choices                ("Install" "Remove" )
	:choice-label-types     (:string :string )
	:choice-colors          ("" "" )
	:initial-selections     (nil nil )
	:initial-state          :active
	:user-data              ()
	:actions                (
		(
		:from                   (BaseWin BaseMode)
		:when                   (Notify )
		:to                     (BaseWin BaseMode)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("if (get_mode() == MODE_REMOVE && value == 0)
	BrowseModules(MODE_INSTALL, VIEW_UNSPEC);
else if (get_mode() == MODE_INSTALL && value == 1)
	BrowseModules(MODE_REMOVE, VIEW_UNSPEC);")
		)
	)
)
(
	:type                   :group
	:name                   BaseModeGrp
	:x                      303
	:y                      7
	:group-type             nil
	:members                (BaseMode )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          BaseCtrl
	:anchor-point           :north-east
	:reference-point        :north-east
	:horizontal-offset      -10
	:vertical-offset        7
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   BaseFile
	:owner                  BaseCtrl
	:help                   "File Button
 
Enables you to save the current program properties to a configuration file.  The File menu contains the following commands:
 
  Save
 
    Use this command if you wish to save the
    current configuration to a file, the name
    of which you specified either on the command
    line via the \"-c\" option, or with a previous
    \"Save As\" operation.
 
  Save As...
 
    Use this command if you wish to specify the
    name of the file to which current program
    properties are to be saved.
"
	:x                      10
	:y                      10
	:width                  54
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "File"
	:label-type             :string
	:initial-state          :active
	:menu                   FileMenu
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   BaseView
	:owner                  BaseCtrl
	:help                   "View Button
 
Displays various pop-up windows and the previous level in the software selection display.  The View menu contains the following commands:
 
  Prev Level
 
    Use this command once you have expanded a
    software cluster in either the software
    selection or level display windows.
    Returning to the previous level enables you
    to view and select the modules contained in
    the cluster you most recently expanded.
 
  Command Window...
 
    Use this command to display the window in
    which swmtool executes programs (the most
    important of which are \"pkgadd\" and \"pkgrm,\"
    the package installation and removal commands.
 
  Space Meter...
 
    Use this command to display a graphical
    representation of the space available on
    your file systems.
"
	:x                      69
	:y                      10
	:width                  62
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "View"
	:label-type             :string
	:initial-state          :active
	:menu                   ViewMenu
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   BaseEdit
	:owner                  BaseCtrl
	:help                   "Edit Button
 
Enables you edit either the native environment or a client support service and
enables selection or deselection of groups of software modules within those environments.  The Edit menu contains the following commands:
 
  Native Environment
 
    Use this command if you wish to select
    software to be installed or removed from
    the set of software used by this [local]
    machine.  This mode is used for almost
    all operations on unbundled software.
 
  Add Client Support...
 
    Use this command if you wish to add
    support [services] for diskless clients.

  Select All
 
    Selects all modules matching the current
    category for either installation or removal,
    depending on the current mode.
 
  Deselect All
 
    Deselects all modules selected for instal-
    lation or removal, depending on the current
    mode.  Note that the semantics of De-select
    All differ slightly from those of Select
    All; deselection operates on all modules,
    visible or not, whereas selection operates
    only on modules matching the current category.
"
	:x                      136
	:y                      10
	:width                  56
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Edit"
	:label-type             :string
	:initial-state          :active
	:menu                   EditMenu
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   BaseProps
	:owner                  BaseCtrl
	:help                   "Props Button
 
Edit the various configurable properties of swmtool.  The Props button displays a pop-up window that provides access to the following groups of properties:
 
  Source Media
 
    The properties accessible from this panel
    enable you to specify the software distri-
    bution you wish to browse.
 
  Package Administration
 
    Properties grouped under this category
    control the actions of the commands \"pkgadd\"
    and \"pkgrm.\"  Most of these properties match
    parameters specified in the packaging
    commands' admin(4) file.
 
  Current Product Category
 
    This property enables you to select a
    \"category\" to which software modules must
    belong in order for them to be displayed in
    the software selection portion of the window.
 
  Browser Display
 
    The properties accessible from this panel
    enable you to control the display format of
    the software selection area.
 
  Remote Hosts
 
    This property enables you to specify hosts
    other than the local host on which you wish
    install or remove packages.
"
	:x                      197
	:y                      10
	:width                  61
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Props..."
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (BaseWin BaseProps)
		:when                   (Notify )
		:to                     (BaseWin BaseProps)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ShowProperties(PROPS_LAST, (caddr_t)0);")
		)
	)
)
(
	:type                   :group
	:name                   BaseTop
	:x                      10
	:y                      10
	:group-type             :row
	:members                (BaseFile BaseView BaseEdit BaseProps )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     5
	:vertical-spacing       10
	:anchor-object          BaseCtrl
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      10
	:vertical-offset        10
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   BaseDoit
	:owner                  BaseCtrl
	:help                   "Apply Button
 
Initiates installation or removal of selected software modules.
 
  This button is activated once you have selected
  software to be either installed or removed,
  depending on the program's current mode.  If
  you have not selected any software modules, the
  button will be rendered inactive (grayed out).
"
	:x                      72
	:y                      34
	:width                  124
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Begin Installation"
	:label-type             :string
	:initial-state          :inactive
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (BaseWin BaseDoit)
		:when                   (Notify )
		:to                     (BaseWin BaseDoit)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("Apply((Xv_opaque)ip);")
		)
	)
)
(
	:type                   :group
	:name                   BaseBottom
	:x                      10
	:y                      34
	:group-type             :row
	:members                (BaseDoit )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     7
	:vertical-spacing       10
	:anchor-object          BaseTop
	:anchor-point           :south-west
	:reference-point        :north-west
	:horizontal-offset      0
	:vertical-offset        5
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   BaseCat
	:owner                  BaseCtrl
	:help                   "Category
 
Indicates to what category software must belong in order to be displayed in the software selection area.
 
  The default category \"All Software\" matches
  all software products.  To change categories
  (and thus limit the number of modules displayed),
  use the Categories... command from the View menu.
"
	:x                      206
	:y                      36
	:width                  263
	:height                 15
	:value-x                277
	:value-y                36
	:value-length           24
	:stored-length          256
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Category:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          ""
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   BaseCatGrp
	:x                      206
	:y                      36
	:group-type             nil
	:members                (BaseCat )
	:rows                   0
	:row-alignment          :top-edges
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          BaseBottom
	:anchor-point           :east
	:reference-point        :west
	:horizontal-offset      10
	:vertical-offset        0
	:user-data              ()
	:actions                ()
)
(
	:type                   :canvas-pane
	:name                   Levels
	:owner                  BaseWin
	:help                   "Level Display
 
Indicates current position in software distribution cluster hierarchy and enables rapid traversal among displayed levels.
 
  Software distributions can be composed of
  packages and clusters, where clusters can
  contain groups of packages or other clusters.
  The resulting tree-like hierarchy facilitates
  rapid selection of groups of modules.  You
  may \"expand\" clusters to view or select their
  individual components.  As you expand a
  cluster, you descend to a new level of the
  distribution's hierarchy.

  Each such level is represented in the level
  display by a box and the name of the level.  The
  level display represents the branch through the
  cluster hierarchy you most recently traversed.
  You may use this display to return to any pre-
  viously-visited level by pressing the SELECT
  mouse button once while the cursor is over the
  box representing the desired level.
"
	:x                      0
	:y                      61
	:width                  478
	:height                 53
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:draggable              nil
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:horizontal-scrollbar   nil
	:scrollable-width       478
	:vertical-scrollbar     nil
	:scrollable-height      53
	:repaint-proc           nil
	:event-handler          nil
	:drawing-model          :xwindows
	:user-data              ()
	:actions                (
		(
		:from                   (BaseWin Levels)
		:when                   (Repaint )
		:to                     (BaseWin Levels)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("RepaintLevels(canvas, paint_window, display, xid, rects);")
		)
		(
		:from                   (BaseWin Levels)
		:when                   (AnyEvent )
		:to                     (BaseWin Levels)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("SelectLevel(win, event, arg, type);")
		)
	)
)
(
	:type                   :canvas-pane
	:name                   Mods
	:owner                  BaseWin
	:help                   "Software Selection Display

Represents software in the current distribution and enables selection for either installation or removal.

  In the bottom area of the window, swmtool
  displays the software modules found in the
  currently-active software distribution.
  The format of this display is controlled by
  the program's display properties.  You may
  limit the number of modules displayed by
  using the Categories... command in the View
  menu to set the current software category.

  Each module icon's border indicates whether
  any version of that module is currently
  installed on the local system.  There are
  three possible border types:

    None

	No border indicates no version of the
	module is installed on the local system.

    Solid

	A solid border around a module's icon
	indicates that the version of the module
	being examined is already installed on
	the local system.

    Broken

	A broken or dashed border around the
	icon indicates a version different from
	that being examined is installed.

  To select a module for installation or removal,
  position the cursor anywhere over the desired
  module and press the mouse SELECT button once.
  Selected items will appear in reverse video.
  Pressing SELECT over the item again removes
  the module from the installation or removal
  list.

  Pressing the MENU button while positioned over
  a module displays the module's menu.  This menu
  contains the following commands, some of which
  may not be active (i.e., grayed-out), depending
  on the module's type:

    Package Properties...

	Use this command to display a pop-up
	window containing additional statistics
	describing the module.

	If you are installing a package, this
	window enables you to specify the direc-
	tory in which the module should be rooted.
	If the module is a cluster, all its com-
	ponents will also be installed relative to
	this root directory.

    Product Properties...

	Use this command to display a pop-up
	window specifying product-level
	properties.  Currently there are two:
	support for multiple architectures and
	multiple localizations.

    Read About

	If active, drag the mouse to the right
	of this command to display a menu
	containing additional textual descriptions
	of the module.

    Run

	If active, drag the mouse to the right
	of this command to display a menu
	containing the names of executable
	demonstrations of the module,

    Expand

	If active, use this command to \"expand\" a
	cluster, enabling you to view and select
	the individual components.

  Double clicking the SELECT button while posi-
  tioned over a module executes the default module
  menu action.  By default, this action displays
  the package property sheet associated with the
  module.  You may change the default by bringing
  up the module menu, positioning the cursor over
  the desired menu item, then pressing and
  releasing the \"Control\" key.
"
	:x                      0
	:y                      114
	:width                  478
	:height                 211
	:background-color       ""
	:foreground-color       ""
	:initial-state          :visible
	:draggable              nil
	:droppable              nil
	:default-drop-site      nil
	:menu                   nil
	:horizontal-scrollbar   nil
	:scrollable-width       478
	:vertical-scrollbar     t
	:scrollable-height      211
	:repaint-proc           nil
	:event-handler          nil
	:drawing-model          :xwindows
	:user-data              ()
	:actions                (
		(
		:from                   (BaseWin Mods)
		:when                   (Repaint )
		:to                     (BaseWin Mods)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("RepaintModules(canvas, paint_window, display, xid, rects);")
		)
		(
		:from                   (BaseWin Mods)
		:when                   (AnyEvent )
		:to                     (BaseWin Mods)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("SelectModules(win, event, arg, type);")
		)
	)
)
)
