;GIL-3
(
(
	:type                   :popup-window
	:name                   ProdWin
	:owner                  BaseWin
	:width                  392
	:height                 326
	:background-color       ""
	:foreground-color       ""
	:label                  "Software Manager:  Product Localizations"
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
	:name                   ProdCtrl
	:owner                  ProdWin
	:help                   ""
	:x                      0
	:y                      0
	:width                  392
	:height                 326
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
	:name                   ProdName
	:owner                  ProdCtrl
	:help                   ""
	:x                      30
	:y                      30
	:width                  297
	:height                 15
	:value-x                135
	:value-y                30
	:value-length           40
	:stored-length          1024
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Product Name:"
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
	:type                   :scrolling-list
	:name                   ProdLocList
	:owner                  ProdCtrl
	:help                   ""
	:x                      33
	:y                      65
	:width                  200
	:height                 200
	:value-x                135
	:value-y                65
	:rows                   10
	:foreground-color       ""
	:label                  "Localizations:"
	:title                  ""
	:label-type             :string
	:layout-type            :horizontal
	:read-only              t
	:multiple-selections    t
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
	:type                   :group
	:name                   ProdGrp
	:x                      30
	:y                      30
	:group-type             :column
	:members                (ProdName ProdLocList )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       20
	:anchor-object          ProdCtrl
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      30
	:vertical-offset        30
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   ProdApply
	:owner                  ProdCtrl
	:help                   ""
	:x                      133
	:y                      295
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
		:from                   (ProdWin ProdApply)
		:when                   (Notify )
		:to                     (ProdWin ProdApply)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("SetProductInfo((Xv_opaque)ip);")
		)
	)
)
(
	:type                   :button
	:name                   ProdReset
	:owner                  ProdCtrl
	:help                   ""
	:x                      201
	:y                      295
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
		:from                   (ProdWin ProdReset)
		:when                   (Notify )
		:to                     (ProdWin ProdReset)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("GetProductInfo((Module *)xv_get(ip->ProdCtrl, PANEL_CLIENT_DATA), 0);")
		)
	)
)
(
	:type                   :group
	:name                   ProdButtons
	:x                      133
	:y                      295
	:group-type             :row
	:members                (ProdApply ProdReset )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     15
	:vertical-spacing       10
	:anchor-object          ProdGrp
	:anchor-point           :south
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        30
	:user-data              ()
	:actions                ()
)
(
	:type                   :popup-window
	:name                   PkgWin
	:owner                  BaseWin
	:width                  565
	:height                 486
	:background-color       ""
	:foreground-color       ""
	:label                  "Software Manager:  Package Information"
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
	:name                   PkgCtrl
	:owner                  PkgWin
	:help                   "Package Properties Control Panel
 
  For more detailed information about a specific
  control, use the mouse to position the cursor
  over the control of interest, then press the
  \"Help\" key."
	:x                      0
	:y                      0
	:width                  565
	:height                 486
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
	:name                   PkgName
	:owner                  PkgCtrl
	:help                   ""
	:x                      96
	:y                      30
	:width                  290
	:height                 15
	:value-x                146
	:value-y                30
	:value-length           30
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Name:"
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
	:type                   :text-field
	:name                   PkgProd
	:owner                  PkgCtrl
	:help                   ""
	:x                      84
	:y                      60
	:width                  302
	:height                 15
	:value-x                146
	:value-y                60
	:value-length           30
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Product:"
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
	:type                   :text-field
	:name                   PkgAbbrev
	:owner                  PkgCtrl
	:help                   ""
	:x                      46
	:y                      90
	:width                  340
	:height                 15
	:value-x                146
	:value-y                90
	:value-length           30
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Abbreviation:"
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
	:type                   :text-field
	:name                   PkgVend
	:owner                  PkgCtrl
	:help                   ""
	:x                      87
	:y                      120
	:width                  299
	:height                 15
	:value-x                146
	:value-y                120
	:value-length           30
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Vendor:"
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
	:type                   :text-field
	:name                   PkgVers
	:owner                  PkgCtrl
	:help                   ""
	:x                      82
	:y                      150
	:width                  304
	:height                 15
	:value-x                146
	:value-y                150
	:value-length           30
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Version:"
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
	:type                   :text-field
	:name                   PkgDesc
	:owner                  PkgCtrl
	:help                   ""
	:x                      55
	:y                      180
	:width                  474
	:height                 71
	:value-x                146
	:value-y                180
	:value-length           40
	:stored-length          1024
	:rows                   5
	:foreground-color       ""
	:text-type              :multiline
	:label                  "Description:"
	:label-type             :string
	:layout-type            :horizontal
	:value-underlined       t
	:initial-value          ""
	:initial-state          :active
	:read-only              t
	:notify-handler         nil
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   PkgArch
	:owner                  PkgCtrl
	:help                   ""
	:x                      30
	:y                      266
	:width                  356
	:height                 15
	:value-x                146
	:value-y                266
	:value-length           30
	:stored-length          1024
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Supported Arch:"
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
	:type                   :text-field
	:name                   PkgStatus
	:owner                  PkgCtrl
	:help                   ""
	:x                      92
	:y                      296
	:width                  294
	:height                 15
	:value-x                146
	:value-y                296
	:value-length           40
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Status:"
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
	:type                   :message
	:name                   PkgSize
	:owner                  PkgCtrl
	:help                   ""
	:x                      35
	:y                      327
	:width                  103
	:height                 13
	:foreground-color       ""
	:label                  "Estimated Size:"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   PkgMbytes
	:owner                  PkgCtrl
	:help                   ""
	:x                      190
	:y                      327
	:width                  60
	:height                 13
	:foreground-color       ""
	:label                  "(Mbytes)"
	:label-type             :string
	:label-bold             t
	:initial-state          :active
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :message
	:name                   PkgDummy
	:owner                  PkgCtrl
	:help                   ""
	:x                      388
	:y                      327
	:width                  53
	:height                 13
	:foreground-color       ""
	:label                  "Dummy"
	:label-type             :string
	:label-bold             t
	:initial-state          :invisible
	:event-handler          nil
	:user-data              ()
	:actions                ()
)
(
	:type                   :text-field
	:name                   PkgRoot
	:owner                  PkgCtrl
	:help                   ""
	:x                      131
	:y                      351
	:width                  79
	:height                 15
	:value-x                146
	:value-y                351
	:value-length           8
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "/"
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
	:type                   :text-field
	:name                   PkgOpt
	:owner                  PkgCtrl
	:help                   ""
	:x                      221
	:y                      351
	:width                  101
	:height                 15
	:value-x                258
	:value-y                351
	:value-length           8
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "/opt"
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
	:type                   :text-field
	:name                   PkgExport
	:owner                  PkgCtrl
	:help                   ""
	:x                      391
	:y                      351
	:width                  122
	:height                 15
	:value-x                449
	:value-y                351
	:value-length           8
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "/export"
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
	:type                   :text-field
	:name                   PkgUsr
	:owner                  PkgCtrl
	:help                   ""
	:x                      108
	:y                      376
	:width                  102
	:height                 15
	:value-x                146
	:value-y                376
	:value-length           8
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "/usr"
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
	:type                   :text-field
	:name                   PkgVar
	:owner                  PkgCtrl
	:help                   ""
	:x                      221
	:y                      376
	:width                  101
	:height                 15
	:value-x                258
	:value-y                376
	:value-length           8
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "/var"
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
	:type                   :text-field
	:name                   PkgOpenwin
	:owner                  PkgCtrl
	:help                   ""
	:x                      345
	:y                      376
	:width                  168
	:height                 15
	:value-x                449
	:value-y                376
	:value-length           8
	:stored-length          80
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "/usr/openwin"
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
	:name                   PkgFs
	:x                      35
	:y                      326
	:group-type             :row-column
	:members                (PkgSize PkgMbytes PkgDummy PkgRoot PkgOpt PkgExport PkgUsr PkgVar PkgOpenwin )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                3
	:column-alignment       :labels
	:horizontal-spacing     -20
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
	:name                   PkgBase
	:owner                  PkgCtrl
	:help                   ""
	:x                      35
	:y                      406
	:width                  367
	:height                 15
	:value-x                146
	:value-y                406
	:value-length           32
	:stored-length          1024
	:rows                   3
	:foreground-color       ""
	:text-type              :alphanumeric
	:label                  "Base Directory:"
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
	:name                   PkgGrp
	:x                      30
	:y                      30
	:group-type             :column
	:members                (PkgName PkgProd PkgAbbrev PkgVend PkgVers PkgDesc PkgArch PkgStatus PkgFs PkgBase )
	:rows                   0
	:row-alignment          :horizontal-centers
	:columns                1
	:column-alignment       :labels
	:horizontal-spacing     10
	:vertical-spacing       15
	:anchor-object          PkgCtrl
	:anchor-point           :north-west
	:reference-point        :north-west
	:horizontal-offset      30
	:vertical-offset        30
	:user-data              ()
	:actions                ()
)
(
	:type                   :button
	:name                   PkgApply
	:owner                  PkgCtrl
	:help                   ""
	:x                      220
	:y                      451
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
		:from                   (PkgWin PkgApply)
		:when                   (Notify )
		:to                     (PkgWin PkgApply)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("SetPackageInfo((Xv_opaque)ip);")
		)
	)
)
(
	:type                   :button
	:name                   PkgReset
	:owner                  PkgCtrl
	:help                   ""
	:x                      288
	:y                      451
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
		:from                   (PkgWin PkgReset)
		:when                   (Notify )
		:to                     (PkgWin PkgReset)
		:function_type          ExecuteCode
		:arg_type               (:string)
		:action                 ("ResetPackageInfo((Xv_opaque)ip);")
		)
	)
)
(
	:type                   :group
	:name                   PkgButtons
	:x                      220
	:y                      451
	:group-type             :row
	:members                (PkgApply PkgReset )
	:rows                   1
	:row-alignment          :horizontal-centers
	:columns                0
	:column-alignment       :labels
	:horizontal-spacing     15
	:vertical-spacing       10
	:anchor-object          PkgGrp
	:anchor-point           :south
	:reference-point        :north
	:horizontal-offset      0
	:vertical-offset        30
	:user-data              ()
	:actions                ()
)
)
