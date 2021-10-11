;GIL-3
(
(
	:type                   :popup-window
	:name                   MeterWin
	:owner                  BaseWin
	:width                  442
	:height                 302
	:background-color       ""
	:foreground-color       ""
	:label                  "Software Manager:  Space Meter"
	:label-type             :string
	:initial-state          :invisible
	:show-footer            nil
	:resizable              nil
	:pinned                 t
	:done-handler           nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :control-area
	:name                   MeterCtrl
	:owner                  MeterWin
	:help                   "Space Meter

Indicates required and available disk space per file system.

  The Space Meter provides an indication of the
  amount of disk space required to install the
  selected software or that would become available
  after removing selected software.  It is auto-
  matically displayed the first time you select a
  software module for which there is insufficient
  disk space.  The columns in the meter are as
  follows:

    File System:

	The first column is a mount point name.
	Swmtool tracks several possible mount
	points and calculates space totals for
	those which correspond to actual mount
	points on your system.  Names which are
	not mount points are grayed-out.

    Percentage Used:
	
	The second column graphically represents
	the combined percentage of disk space used
	and disk space required to install
	selected software.

    Mb Used:

	The third column is the total, in
	Megabytes, of the amount of disk space
	currently in use plus that required to
	install selected software.

    Mb Free:

	The last column indicates the amount of
	space, in Megabytes, that would remain in
	each file system after installation of any
	selected software.  A negative value indi-
	cates the program estimates insufficient
	space is available for the selected
	software and requires corrective action.

  If you do not have enough space on a file system
  to install all selected software modules, you
  might consider some or all of the following
  options:  select fewer software modules for
  installation, change the installation directory
  for some or all selected software, increase the
  amount of space allocated to the file system
  in question, or use symbolic links or new
  partitions to move the installation directory
  to a different file system.
"
	:x                      0
	:y                      0
	:width                  442
	:height                 302
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
	:type                   :gauge
	:name                   DummyGauge
	:owner                  MeterCtrl
	:help                   ""
	:x                      104
	:y                      20
	:width                  120
	:height                 28
	:value-x                104
	:value-y                20
	:slider-width           100
	:ticks                  10
	:foreground-color       ""
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:orientation            :horizontal
	:show-range             t
	:min-value              0
	:max-value              100
	:min-tick-string        ""
	:max-tick-string        ""
	:initial-value          0
	:initial-state          :invisible
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   MeterUsed
	:owner                  MeterCtrl
	:help                   ""
	:x                      254
	:y                      35
	:width                  59
	:height                 13
	:foreground-color       ""
	:label                  "Mb Used"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   MeterFree
	:owner                  MeterCtrl
	:help                   ""
	:x                      343
	:y                      35
	:width                  54
	:height                 13
	:foreground-color       ""
	:label                  "Mb Free"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   HdrSpace
	:x                      104
	:y                      20
	:group-type             :row
	:members                (DummyGauge MeterUsed MeterFree )
	:rows                   1
	:row-alignment          :bottom-edges
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     30
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
	:type                   :gauge
	:name                   RootGauge
	:owner                  MeterCtrl
	:help                   ""
	:x                      89
	:y                      53
	:width                  135
	:height                 28
	:value-x                104
	:value-y                53
	:slider-width           100
	:ticks                  10
	:foreground-color       ""
	:label                  "/"
	:label-type             :string
	:layout-type            :horizontal
	:orientation            :horizontal
	:show-range             t
	:min-value              0
	:max-value              100
	:min-tick-string        ""
	:max-tick-string        ""
	:initial-value          43
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   RootUsed
	:owner                  MeterCtrl
	:help                   ""
	:x                      254
	:y                      53
	:width                  64
	:height                 15
	:value-x                254
	:value-y                53
	:value-length           8
	:stored-length          10
	:rows                   1
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          "000.00"
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   RootFree
	:owner                  MeterCtrl
	:help                   ""
	:x                      348
	:y                      53
	:width                  64
	:height                 15
	:value-x                348
	:value-y                53
	:value-length           8
	:stored-length          10
	:rows                   1
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          "000.00"
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   RootSpace
	:x                      89
	:y                      53
	:group-type             :row
	:members                (RootGauge RootUsed RootFree )
	:rows                   1
	:row-alignment          :top-edges
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     30
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
	:type                   :gauge
	:name                   UsrGauge
	:owner                  MeterCtrl
	:help                   ""
	:x                      66
	:y                      86
	:width                  158
	:height                 28
	:value-x                104
	:value-y                86
	:slider-width           100
	:ticks                  10
	:foreground-color       ""
	:label                  "/usr"
	:label-type             :string
	:layout-type            :horizontal
	:orientation            :horizontal
	:show-range             t
	:min-value              0
	:max-value              100
	:min-tick-string        ""
	:max-tick-string        ""
	:initial-value          53
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   UsrUsed
	:owner                  MeterCtrl
	:help                   ""
	:x                      254
	:y                      86
	:width                  64
	:height                 15
	:value-x                254
	:value-y                86
	:value-length           8
	:stored-length          10
	:rows                   1
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          "000.00"
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   UsrFree
	:owner                  MeterCtrl
	:help                   ""
	:x                      348
	:y                      86
	:width                  64
	:height                 15
	:value-x                348
	:value-y                86
	:value-length           8
	:stored-length          10
	:rows                   1
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          "000.00"
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   UsrSpace
	:x                      66
	:y                      86
	:group-type             :row
	:members                (UsrGauge UsrUsed UsrFree )
	:rows                   1
	:row-alignment          :top-edges
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     30
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
	:type                   :gauge
	:name                   OptGauge
	:owner                  MeterCtrl
	:help                   ""
	:x                      67
	:y                      119
	:width                  157
	:height                 28
	:value-x                104
	:value-y                119
	:slider-width           100
	:ticks                  10
	:foreground-color       ""
	:label                  "/opt"
	:label-type             :string
	:layout-type            :horizontal
	:orientation            :horizontal
	:show-range             t
	:min-value              0
	:max-value              100
	:min-tick-string        ""
	:max-tick-string        ""
	:initial-value          25
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   OptUsed
	:owner                  MeterCtrl
	:help                   ""
	:x                      254
	:y                      119
	:width                  64
	:height                 15
	:value-x                254
	:value-y                119
	:value-length           8
	:stored-length          10
	:rows                   1
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          "000.00"
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   OptFree
	:owner                  MeterCtrl
	:help                   ""
	:x                      348
	:y                      119
	:width                  64
	:height                 15
	:value-x                348
	:value-y                119
	:value-length           8
	:stored-length          10
	:rows                   1
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          "000.00"
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   OptSpace
	:x                      67
	:y                      119
	:group-type             :row
	:members                (OptGauge OptUsed OptFree )
	:rows                   1
	:row-alignment          :top-edges
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     30
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
	:type                   :gauge
	:name                   VarGauge
	:owner                  MeterCtrl
	:help                   ""
	:x                      67
	:y                      152
	:width                  157
	:height                 28
	:value-x                104
	:value-y                152
	:slider-width           100
	:ticks                  10
	:foreground-color       ""
	:label                  "/var"
	:label-type             :string
	:layout-type            :horizontal
	:orientation            :horizontal
	:show-range             t
	:min-value              0
	:max-value              100
	:min-tick-string        ""
	:max-tick-string        ""
	:initial-value          15
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   VarUsed
	:owner                  MeterCtrl
	:help                   ""
	:x                      254
	:y                      152
	:width                  64
	:height                 15
	:value-x                254
	:value-y                152
	:value-length           8
	:stored-length          10
	:rows                   1
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          "000.00"
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   VarFree
	:owner                  MeterCtrl
	:help                   ""
	:x                      348
	:y                      152
	:width                  64
	:height                 15
	:value-x                348
	:value-y                152
	:value-length           8
	:stored-length          10
	:rows                   1
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          "000.00"
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   VarSpace
	:x                      67
	:y                      152
	:group-type             :row
	:members                (VarGauge VarUsed VarFree )
	:rows                   1
	:row-alignment          :top-edges
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     30
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
	:type                   :gauge
	:name                   ExpGauge
	:owner                  MeterCtrl
	:help                   ""
	:x                      46
	:y                      185
	:width                  178
	:height                 28
	:value-x                104
	:value-y                185
	:slider-width           100
	:ticks                  10
	:foreground-color       ""
	:label                  "/export"
	:label-type             :string
	:layout-type            :horizontal
	:orientation            :horizontal
	:show-range             t
	:min-value              0
	:max-value              100
	:min-tick-string        ""
	:max-tick-string        ""
	:initial-value          58
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   ExpUsed
	:owner                  MeterCtrl
	:help                   ""
	:x                      254
	:y                      185
	:width                  64
	:height                 15
	:value-x                254
	:value-y                185
	:value-length           8
	:stored-length          10
	:rows                   1
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          "000.00"
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   ExpFree
	:owner                  MeterCtrl
	:help                   ""
	:x                      348
	:y                      185
	:width                  64
	:height                 15
	:value-x                348
	:value-y                185
	:value-length           8
	:stored-length          10
	:rows                   1
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          "000.00"
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   ExpSpace
	:x                      46
	:y                      185
	:group-type             :row
	:members                (ExpGauge ExpUsed ExpFree )
	:rows                   1
	:row-alignment          :top-edges
	:columns                0
	:column-alignment       :left-edges
	:horizontal-spacing     30
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
	:type                   :gauge
	:name                   OwnGauge
	:owner                  MeterCtrl
	:help                   ""
	:x                      30
	:y                      218
	:width                  194
	:height                 28
	:value-x                104
	:value-y                218
	:slider-width           100
	:ticks                  10
	:foreground-color       ""
	:label                  "/openwin"
	:label-type             :string
	:layout-type            :horizontal
	:orientation            :horizontal
	:show-range             t
	:min-value              0
	:max-value              100
	:min-tick-string        ""
	:max-tick-string        ""
	:initial-value          32
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   OwnUsed
	:owner                  MeterCtrl
	:help                   ""
	:x                      254
	:y                      218
	:width                  64
	:height                 15
	:value-x                254
	:value-y                218
	:value-length           8
	:stored-length          10
	:rows                   1
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          "000.00"
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   OwnFree
	:owner                  MeterCtrl
	:help                   ""
	:x                      348
	:y                      218
	:width                  64
	:height                 15
	:value-x                348
	:value-y                218
	:value-length           8
	:stored-length          10
	:rows                   1
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  ""
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       nil
	:initial-value          "000.00"
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :group
	:name                   OwnSpace
	:x                      30
	:y                      218
	:group-type             :row
	:members                (OwnGauge OwnUsed OwnFree )
	:rows                   1
	:row-alignment          :top-edges
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     30
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
	:name                   AllSpace
	:x                      30
	:y                      20
	:group-type             :column
	:members                (HdrSpace RootSpace UsrSpace OptSpace VarSpace ExpSpace OwnSpace )
	:rows                   0
	:row-alignment          :top-edges
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       5
	:anchor-object          MeterCtrl
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      30
	:vertical-offset        20
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   MeterDismiss
	:owner                  MeterCtrl
	:help                   "Dismiss Button

Dismisses the Space Meter window from view.

  Alternatively, you may remove the push-pin from
  the window's frame, or use the window manager's
  Window menu and the Dismiss command.  These
  procedures are window-manager dependent; consult
  the documentation supplied with your window
  manager if you are not using olwm.
"
	:x                      189
	:y                      276
	:width                  65
	:height                 19
	:constant-width         nil
	:button-type            :normal
	:foreground-color       ""
	:label                  "Dismiss"
	:label-type             :string
	:initial-state          :active
	:menu                   nil
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                (
		(
		:from                   (MeterWin MeterDismiss)
		:when                   (Notify )
		:to                     (MeterWin)
		:function_type          :user_defined
		:arg_type               ()
		:action                 (Hide)
		)
	)
)
(
	:type                   :group
	:name                   SpaceDismiss
	:x                      189
	:y                      276
	:group-type             nil
	:members                (MeterDismiss )
	:rows                   0
	:row-alignment          :top-edges
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       10
	:anchor-object          AllSpace
	:anchor-point           :south
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        30
	:user-data              ()
	:actions                ()
)
)
