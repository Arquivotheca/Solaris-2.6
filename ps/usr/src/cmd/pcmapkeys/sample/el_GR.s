# Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
#
#	"@(#)el_GR.s	1.1	93/03/02 SMI"
#
# This mapfile is provided as example only. Sun Microsystems Inc.,
# does not warrant that this file is error-free.
#
#	el_EL.<code_set_name>
#
# ttymap file for keyboard mapping in IBM Greek.
#
# This mapping file is indended for use with the Greek loadfont.
# It implements a keyboard mapping which allows the use of the
# 437 code set, using the Danish Standards Association's recommended
# short names (but not their "compose" value), such that the "compose" key,
# followed by the 2-letter code, generates the character.
# In addition, more "graphic" combinations are also encoded
# (for instance, E^ in addition to E>, and ?? in addition to ?I
# for inverted question mark). The graphic characters are encoded
# using special names.
#
# This ttymap file modifies the scan codes, thus allowing concurrent
# usage of Greek and Latin letters. However, that implies the loss of
# the traditional ALT+char values used by e.g. 10+ and other applications.
# If your application uses these, the mapfile el_GR.t could be used.
#
input:
#
# The toggle key is CTRL t.
#
toggle: 0x14
#
dead: <tonos-accent>		# tonos (acute>
<ALPHA>	<ALPHA-accent>          # ALPHA accent
<EPSILON>	<EPSILON-accent>         # EPSILON accent
<ETA>	<ETA-accent>          # ETA accent
<IOTA>	<IOTA-accent>          # IOTA accent
<OMIKRON>	<OMIKRON-accent>          # OMIKRON accent
<UPSILON>	<UPSILON-accent>          # UPSILON accent
<OMEGA>	<OMEGA-accent>          # OMEGA accent
<alpha>	<alpha-accent>          # alpha accent
<epsilon>	<epsilon-accent>         # epsilon accent
<eta>	<eta-accent>          # eta accent
<iota>	<iota-accent>          # iota accent
<omikron>	<omikron-accent>          # omikron accent
<upsilon>	<upsilon-accent>          # upsilon accent
<omega>	<omega-accent>          # omega accent
#
dead:	<diaeresis>		# diaeresis
<IOTA>	<IOTA-diaeresis>         # IOTA diaeresis
<UPSILON>	<UPSILON-diaeresis>         # UPSILON diaeresis
<iota>	<iota-diaeresis>         # iota diaeresis
<upsilon>	<upsilon-diaeresis>         # upsilon diaeresis
#
dead: <diaeresis-tonos>			# diaeresis and tonos
' '   ' '
<iota>	<iota-accent-diaeresis>         # iota diaeresis & accent
<upsilon>	<upsilon-acc-diaeresis>         # upsilon diaresis & accent
#
# The "compose" character is CTRL x.
# 
# Compose mode: 
# 
#
# The "compose" character is CTRL x.
# Compose mode: 
# 
compose:  0x18      # unfrequently used ctrl character
#			
' '  ' '  <no-break-space>          # no breaking space 
' '  '1'  <superscript-1>          # superscript 1
' '  '2'  <superscript-2>          # superscript 2
' '  '3'  <superscript-3>          # superscript 3
' '  'n'  <superscript-n>          # superscript n
'!'  '!'  <inverted-exclamation>          # inverted exclamation mark 
'!'  'I'  <inverted-exclamation>          # inverted exclamation mark 
'#'  '$'  <currency-sign>          # currency symbol
'$'  '$'  <paragraph-sign>          # paragraph sign
'%'  '"'  <Cyrillic-Soft-sign>          # SOFT SIGN
'%'  '''  <Cyrillic-soft-sign>          # soft sign
'''  ' '  <apostrophe>          # apostrophe
'''  '"'  <double-acute-accent>          # double-acute-accent
'''  '''  <acute-accent>          # acute accent
'''  '('  <breve>          # breve
'''  ','  <cedilla>          # cedilla
'''  '-'  <macron>          # macron
'''  '.'  <dot-above>          # dot above
'''  ':'  <diaeresis>          # diaeresis
'''  ';'  <ogonek>          # ogonek
'''  '<'  <caron>          # caron
'('  'U'  <intersection>          # intersection
'*'  '*'  <degree-sign>          # degree
'*'  '.'  <multiplication-sign>          # multiplication sign
'*'  'X'  <multiplication-sign>          # multiplication sign
'*'  's'  <sigma-final>          # final sigma
'+'  '-'  <plus-minus-sign>          # plus-minus
','  ','  <cedilla>          # cedilla
'-'  '-'  <soft-hyphen>          # soft hyphen
'-'  ':'  <division-sign>          # division sign
'-'  'a'  <feminine-ordinal-a>          # ordinal (feminine) a
'-'  'o'  <masculine-ordinal-o>          # ordinal (masculine) o
'-'  '|'  <not-sign>          # not sign
'.'  '.'  <middle-dot>          # middle dot
'.'  'M'  <middle-dot>          # middle dot
'.'  'S'  <light-shade>          # box drawing light shade (25%)
'1'  'S'  <superscript-1>          # superscript 1
'1'  'h'  <one-half>          # vulgar fraction 1/2
'1'  'q'  <one-quarter>          # vulgar fraction 1/4
'2'  'S'  <superscript-2>          # superscript 2
'3'  'S'  <superscript-3>          # superscript 3
'3'  'q'  <three-quarters>          # vulgar fraction 3/4
':'  'S'  <medium-shade>          # box drawing medium shade (50%)
'<'  '<'  <left-angle-quotation>          # left double angle quotation mark
'<'  '='  <less-than-or-equal>          # less than or equal
'='  '"'  <Cyrillic-Hard-sign>          # HARD SIGN
'='  '''  <Cyrillic-hard-sign>          # hard sign
'='  '2'  <double-underscore>          # double underline
'='  '3'  <identical-to-sign>          # Is identical to
'>'  '='  <greater-than-or-equal>          # greater than or equal to
'>'  '>'  <right-angle-quotation>          # right double angle quotation mark
'?'  '2'  <almost-equals-sign>          # almost equal
'?'  '?'  <inverted-question>          # inverted question mark
'?'  'I'  <inverted-question>          # inverted question mark
'?'  'S'  <dark-shade>          # box drawing dark shade (75%)
'A'  '!'  <A-grave>          # A grave
'A'  '%'  <ALPHA-accent>          # Alpha accent
'A'  '''  <A-acute>          # A acute
'A'  '('  <A-breve>          # A breve
'A'  '*'  <ALPHA>          # Greek letter ALPHA
'A'  '-'  <A-macron>          # A macron
'A'  '.'  <A-ring>          # A ring above
'A'  ':'  <A-diaeresis>          # A diaeresis
'A'  ';'  <A-ogonek>          # A ogonek
'A'  '='  <Cyrillic-A>          # A
'A'  '>'  <A-circumflex>          # A circumflex
'A'  '?'  <A-tilde>          # A tilde
'A'  'A'  <A-ring>          # A ring above
'A'  'E'  <AE>          # AE diphtong
'A'  '^'  <A-circumflex>          # A circumflex
'A'  '`'  <A-grave>          # A grave
'A'  '~'  <A-tilde>          # A tilde
'B'  '*'  <BETA>          # Beta
'B'  '='  <Cyrillic-BE>          # BE
'B'  'B'  <broken-bar>          # broken bar
'B'  'T'  <bottom-mid-heavy>          # "Bottom T Intersection Double"
'B'  't'  <bottom-mid-heavy-vert>          # "Bottom T (_||_)
'C'  '%'  <Cyrillic-CHE>          # CHE
'C'  '''  <C-acute>          # C acute
'C'  '*'  <XI>          # Xi
'C'  ','  <C-cedilla>          # C cedilla
'C'  '.'  <C-dot>          # C dot above
'C'  '<'  <C-caron>          # C caron
'C'  '='  <Cyrillic-TSE>          # TSE
'C'  '>'  <C-circumflex>          # C circumflex
'C'  '^'  <C-circumflex>          # C circumflex
'C'  'o'  <copyright-sign>          # copyright sign
'C'  't'  <cent-sign>          # cent sign
'C'  'u'  <currency-sign>          # currency symbol
'D'  '%'  <Serbian-DJE>          # DJE (Serbocroatian)
'D'  '*'  <DELTA>          # Delta
'D'  '-'  <Eth>          # Capital eth
'D'  '/'  <D-stroke>          # D stroke
'D'  '<'  <D-caron>          # D caron
'D'  '='  <Cyrillic-DE>          # DE
'D'  'G'  <degree-sign>          # degree
'D'  'S'  <Macedonian-DZE>          # DZE (Macedonian)
'D'  'Z'  <Cyrillic-DZHE>          # DZHE
'E'  '!'  <E-grave>          # E grave
'E'  '%'  <EPSILON-accent>          # Epsilon accent
'E'  '''  <E-acute>          # E acute
'E'  '*'  <EPSILON>          # Epsilon
'E'  '-'  <E-macron>          # E macron
'E'  '.'  <E-dot>          # E dot
'E'  ':'  <E-diaeresis>          # E diaeresis
'E'  ';'  <E-ogonek>          # E ogonek
'E'  '<'  <E-caron>          # E caron
'E'  '='  <Cyrillic-IE>          # IE
'E'  '>'  <E-circumflex>          # E circumflex
'E'  '^'  <E-circumflex>          # E circumflex
'E'  '`'  <E-grave>          # E grave
'F'  '*'  <PHI>          # Greek letter PHI
'F'  '='  <Cyrillic-EF>          # EF
'F'  'B'  <solid-full-block>          # "Solid Full Block"
'G'  '%'  <Macedonian-GJE>          # GJE (Macedonian)
'G'  '('  <G-breve>          # G breve
'G'  '*'  <GAMMA>          # Greek letter GAMMA
'G'  ','  <G-cedilla>          # G cedilla
'G'  '.'  <G-dot>          # G dot above
'G'  '='  <Cyrillic-GHE>          # GHE
'G'  '>'  <G-circumflex>          # G circumflex
'G'  '^'  <G-circumflex>          # G circumflex
'H'  '*'  <THETA>          # Greek letter THETA
'H'  '/'  <H-stroke>          # H stroke
'H'  '='  <Cyrillic-HA>          # HA
'H'  '>'  <H-circumflex>          # H circumflex
'H'  'L'  <horizontal-heavy>          # "Horizontal Line Double" (||)
'H'  '^'  <H-circumflex>          # H circumflex
'I'  '!'  <I-grave>          # I grave
'I'  '%'  <IOTA-accent>          # Iota accent
'I'  '''  <I-acute>          # I acute
'I'  '*'  <IOTA>          # Iota
'I'  '-'  <I-macron>          # I macron
'I'  '.'  <I-dot>          # I dot above
'I'  ':'  <I-diaeresis>          # I diaresis
'I'  ';'  <I-ogonek>          # I ogonek
'I'  '='  <Cyrillic-I>          # I
'I'  '>'  <I-circumflex>          # I circumflex
'I'  '?'  <I-tilde>          # I tilde
'I'  'B'  <integral-upper>          # Integral sign (bottom half)
'I'  'E'  <Ukrainian-IE>          # IE (Ukrainian)
'I'  'I'  <Ukrainian-I>          # I (Byelorussian-Ukrainian)
'I'  'N'  <inverted-not-sign>          # Inverted not sign
'I'  'O'  <Cyrillic-IO>          # IO
'I'  'T'  <integral-lower>          # Integral sign (top half)
'I'  '^'  <I-circumflex>          # I circumflex
'I'  '`'  <I-grave>          # I grave
'J'  '%'  <Cyrillic-JE>          # JE
'J'  '*'  <IOTA-diaeresis>          # Iota diaeresis
'J'  '='  <Cyrillic-SHORT-I>          # SHORT I
'J'  '>'  <J-circumflex>          # J circumflex
'J'  'A'  <Cyrillic-YA>          # YA
'J'  'E'  <Cyrillic-E>          # E
'J'  'U'  <Cyrillic-YU>          # YU
'J'  '^'  <J-circumflex>          # J circumflex
'K'  '*'  <KAPPA>          # Kappa
'K'  ','  <K-cedilla>          # K cedilla
'K'  '='  <Cyrillic-KA>          # KA
'K'  'J'  <Macedonian-KJE>          # KJE (Macedonian)
'L'  '''  <L-acute>          # L acute
'L'  '*'  <LAMBDA>          # Lambda
'L'  ','  <L-cedilla>          # L cedilla
'L'  '-'  <pound-sign>          # pound sign
'L'  '/'  <L-stroke>          # L stroke
'L'  '<'  <L-caron>          # L caron
'L'  '='  <Cyrillic-EL>          # EL
'L'  'B'  <solid-lower-half>          # "Solid Lower Half Block"
'L'  'J'  <Cyrillic-LJE>          # LJE
'L'  'L'  <lower-left-heavy>          # "Lower Left Corner Double"
'L'  'R'  <lower-right-heavy>          # "Lower Right Corner Double"
'L'  'T'  <left-mid-heavy>          # "Left T Intersection Double" (|-) 
'L'  'l'  <lower-left-heavy-hor>          # "Lower Left Corner" (h. double)
'L'  'r'  <lower-right-heavy-hor>          # "Lower Right Corner" (h. double)
'L'  't'  <left-mid-heavy-horiz>          # "Left T Intersection" (|=)
'M'  '*'  <MU>          # Mu
'M'  '='  <Cyrillic-EM>          # EM
'M'  'y'  <micro-sign>          # micro
'N'  '*'  <NU>          # Nu
'N'  ','  <N-cedilla>          # N cedilla
'N'  '<'  <N-caron>          # N caron
'N'  '='  <Cyrillic-EN>          # EN
'N'  '?'  <N-tilde>          # N tilde
'N'  'G'  <Eng>          # Eng
'N'  'J'  <Cyrillic-NJE>          # NJE
'N'  'O'  <not-sign>          # not sign
'N'  'S'  <no-break-space>          # no breaking space 
'N'  'o'  <not-sign>          # not sign
'N'  '~'  <N-tilde>          # N tilde
'O'  '!'  <O-grave>          # O grave
'O'  '"'  <O-double-acute>          # O double acute
'O'  '%'  <OMIKRON-accent>          # Omikron accent
'O'  '''  <O-acute>          # O acute
'O'  '*'  <OMIKRON>          # Omikron
'O'  '-'  <O-macron>          # O macron
'O'  '/'  <O-slash>          # O slash
'O'  ':'  <O-diaeresis>          # O diaeresis
'O'  '='  <Cyrillic-O>          # O
'O'  '>'  <O-circumflex>          # O circumflex
'O'  '?'  <O-tilde>          # O tilde
'O'  '^'  <O-circumflex>          # O circumflex
'O'  '`'  <O-grave>          # O grave
'O'  '~'  <O-tilde>          # O tilde
'P'  '*'  <PI>          # Pi
'P'  '='  <Cyrillic-PE>          # PE
'P'  'I'  <pilcrow-sign>          # pilcrow
'P'  'P'  <pilcrow-sign>          # pilcrow
'P'  'd'  <pound-sign>          # pound sign
'Q'  '*'  <PSI>          # Psi
'R'  '''  <R-acute>          # R acute
'R'  '*'  <RHO>          # Rho
'R'  ','  <R-cedilla>          # R cedilla
'R'  '<'  <R-caron>          # R caron
'R'  '='  <Cyrillic-ER>          # ER
'R'  'B'  <solid-right-half>          # Solid block right half
'R'  'O'  <registered-mark>          # registered mark
'R'  'T'  <right-mid-heavy>          # "Right T intersection Double" (-|)
'R'  'g'  <registered-mark>          # registered mark
'S'  '%'  <Cyrillic-SHA>          # SHA
'S'  '''  <S-acute>          # S acute
'S'  '*'  <SIGMA>          # Greek letter SIGMA
'S'  ','  <S-cedilla>          # S cedilla
'S'  '<'  <S-caron>          # S caron
'S'  '='  <Cyrillic-ES>          # ES
'S'  '>'  <S-circumflex>          # S circumflex
'S'  'E'  <paragraph-sign>          # paragraph sign
'S'  '^'  <S-circumflex>          # S circumflex
'S'  'c'  <Cyrillic-SHCHA>          # SHCHA
'T'  '*'  <TAU>          # Tau
'T'  ','  <T-cedilla>          # T cedilla
'T'  '/'  <T-stroke>          # T stroke
'T'  '<'  <T-caron>          # T caron
'T'  '='  <Cyrillic-TE>          # TE
'T'  'H'  <Thorn>          # Thorn
'T'  's'  <Serbian-TSHE>          # TSHE (Serbocroatian)
'U'  '!'  <U-grave>          # U grave
'U'  '"'  <U-double-acute>          # U double acute
'U'  '%'  <UPSILON-accent>          # Upsilon accent
'U'  '''  <U-acute>          # U acute
'U'  '('  <U-breve>          # U breve
'U'  '*'  <UPSILON>          # Upsilon
'U'  ','  <U-cedilla>          # U cedilla
'U'  '-'  <U-macron>          # U macron
'U'  '.'  <U-ring>          # U ring above
'U'  ':'  <U-diaeresis>          # U diaeresis
'U'  '='  <Cyrillic-U>          # U
'U'  '>'  <U-circumflex>          # U circumflex
'U'  '?'  <U-tilde>          # U tilde
'U'  'B'  <solid-upper-half>          # "Solid Upper Half Block"
'U'  'L'  <upper-left-heavy>          # "Upper Left Corner Double" (|~)
'U'  'R'  <upper-right-heavy>          # "Upper Right Corner Double" (~|)
'U'  'T'  <top-mid-heavy>          # "Upper T Intersection Double"
'U'  '^'  <U-circumflex>          # U circumflex
'U'  '`'  <U-grave>          # U grave
'U'  'l'  <upper-left-heavy-hor>          # "Upper Left Corner" (h. double)
'U'  'r'  <upper-right-heavy-hor>          # "Upper Right Corner" (h. double)
'U'  't'  <top-mid-heavy-horiz>          # "Upper T" (h. double)
'U'  '~'  <U-tilde>          # U tilde
'V'  '%'  <Byelorussian-SHORT-U>          # SHORT U (Byelorussian)
'V'  '*'  <UPSILON-diaeresis>          # Upsilon diaeresis
'V'  '='  <Cyrillic-VE>          # VE
'V'  'L'  <vertical-heavy>          # "Vertical line Double" (=)
'W'  '%'  <OMEGA-accent>          # Omega accent
'W'  '*'  <OMEGA>          # Greek letter OMEGA
'X'  '*'  <CHI>          # Chi
'X'  'T'  <intersection-heavy>          # "Middle Cross Heavy" (=||=)
'X'  't'  <intersect-heavy-horiz>          # "Middle Cross" (-||-)
'Y'  '%'  <ETA-accent>          # Eta accent
'Y'  '''  <Y-acute>          # Y acute
'Y'  '*'  <ETA>          # Eta
'Y'  '-'  <yen-sign>          # yen sign
'Y'  '='  <Cyrillic-YERU>          # YERU
'Y'  'I'  <Ukrainian-YI>          # YI (Ukrainian)
'Y'  'e'  <yen-sign>          # yen sign
'Z'  '%'  <Cyrillic-ZHE>          # ZHE
'Z'  '''  <Z-acute>          # Z acute
'Z'  '*'  <ZETA>          # Zeta
'Z'  '.'  <Z-dot>          # Z dot
'Z'  '<'  <Z-caron>          # Z caron
'Z'  '='  <Cyrillic-ZE>          # ZE
'_'  '_'  <double-underscore>          # "Double underline"
'a'  '!'  <a-grave>          # a grave
'a'  '%'  <alpha-accent>          # alpha accent
'a'  '''  <a-acute>          # a acute
'a'  '('  <a-breve>          # a breve
'a'  '*'  <alpha>          # Greek letter alpha
'a'  '.'  <a-ring>          # a ring above
'a'  ':'  <a-diaeresis>          # a diaeresis
'a'  ';'  <a-ogonek>          # a ogonek
'a'  '='  <Cyrillic-a>          # a
'a'  '>'  <a-circumflex>          # a circumflex
'a'  '?'  <a-tilde>          # a tilde
'a'  '^'  <a-circumflex>          # a circumflex
'a'  '`'  <a-grave>          # a grave
'a'  'a'  <a-ring>          # a ring above
'a'  'e'  <ae>          # ae diphtong
'a'  '~'  <a-tilde>          # a tilde
'b'  '*'  <beta>          # Greek letter beta
'b'  '='  <Cyrillic-be>          # be
'b'  't'  <bottom-mid>          # "Bottom T intersection" (_|_)
'c'  '%'  <Cyrillic-che>          # che
'c'  '''  <c-acute>          # c acute
'c'  '*'  <xi>          # xi
'c'  ','  <c-cedilla>          # c cedilla
'c'  '.'  <c-dot>          # c dot
'c'  '/'  <cent-sign>          # cent sign
'c'  '<'  <c-caron>          # c caron
'c'  '='  <Cyrillic-tse>          # tse
'c'  '>'  <c-circumflex>          # c circumflex
'c'  'O'  <copyright-sign>          # copyright sign
'c'  '^'  <c-circumflex>          # c circumflex
'd'  '%'  <Serbian-dje>          # dje (Serbocroatian)
'd'  '*'  <delta>          # Greek letter delta
'd'  '-'  <eth>          # eth
'd'  '/'  <d-stroke>          # d stroke
'd'  '<'  <d-caron>          # d caron
'd'  '='  <Cyrillic-de>          # de
'd'  's'  <Macedonian-dze>          # dze (Macedonian)
'd'  'z'  <Cyrillic-dzhe>          # dzhe
'e'  '!'  <e-grave>          # e grave
'e'  '%'  <epsilon-accent>          # epsilon accent
'e'  '''  <e-acute>          # e acute
'e'  '*'  <epsilon>          # Greek letter epsilon
'e'  '-'  <e-macron>          # e macron
'e'  '.'  <e-dot>          # e dot above
'e'  ':'  <e-diaeresis>          # e diaeresis
'e'  ';'  <e-ogonek>          # e ogonek
'e'  '<'  <e-caron>          # e caron
'e'  '='  <Cyrillic-ie>          # ie
'e'  '>'  <e-circumflex>          # e circumflex
'e'  '^'  <e-circumflex>          # e circumflex
'e'  '`'  <e-grave>          # e grave
'f'  '*'  <phi>          # Greek letter phi
'f'  '='  <Cyrillic-ef>          # ef
'f'  'l'  <f-script>          # florin
'g'  '%'  <Macedonian-gje>          # gje (Macedonian)
'g'  '('  <g-breve>          # g breve
'g'  '*'  <gamma>          # gamma
'g'  ','  <g-cedilla>          # g cedilla
'g'  '.'  <g-dot>          # g dot
'g'  '='  <Cyrillic-ghe>          # ghe
'g'  '>'  <g-circumflex>          # g circumflex
'g'  '^'  <g-circumflex>          # g circumflex
'h'  '*'  <theta>          # theta
'h'  '/'  <h-stroke>          # h stroke
'h'  '='  <Cyrillic-ha>          # ha
'h'  '>'  <h-circumflex>          # h circumflex
'h'  '^'  <h-circumflex>          # h circumflex
'h'  'l'  <horizontal>          # "Horizontal Line"
'i'  ' '  <i-dotless>          # I without dot
'i'  '!'  <i-grave>          # i grave
'i'  '%'  <iota-accent>          # iota accent
'i'  '''  <i-acute>          # i acute
'i'  '*'  <iota>          # iota
'i'  ','  <i-cedilla>          # i cedilla
'i'  '-'  <i-macron>          # i macron
'i'  '.'  <i-dotless>          # Dotless i
'i'  ':'  <i-diaeresis>          # i diaresis
'i'  '='  <Cyrillic-i>          # i
'i'  '>'  <i-circumflex>          # i circumflex
'i'  '?'  <i-tilde>          # i tilde
'i'  '^'  <i-circumflex>          # i circumflex
'i'  '`'  <i-grave>          # i grave
'i'  'e'  <Ukrainian-ie>          # ie (Ukrainian)
'i'  'i'  <Ukrainian-i>          # i (Byelorussian-Ukrainian)
'i'  'o'  <Cyrillic-io>          # io
'j'  '%'  <Cyrillic-je>          # je
'j'  '*'  <iota-diaeresis>          # iota diaeresis
'j'  '='  <Cyrillic-short-i>          # short i
'j'  '>'  <j-circumflex>          # j circumflex
'j'  '^'  <j-circumflex>          # j circumflex
'j'  'a'  <Cyrillic-ya>          # ya
'j'  'e'  <Cyrillic-e>          # e
'j'  'u'  <Cyrillic-yu>          # yu
'k'  '*'  <kappa>          # kappa
'k'  ','  <k-cedilla>          # k cedilla
'k'  '='  <Cyrillic-ka>          # ka
'k'  'j'  <Macedonian-kje>          # kje (Macedonian)
'k'  'k'  <kra>          # kra
'l'  '''  <l-acute>          # l acute
'l'  '*'  <lambda>          # lambda
'l'  ','  <l-cedilla>          # l cedilla
'l'  '/'  <l-stroke>          # l stroke
'l'  '<'  <l-caron>          # l caron
'l'  '='  <Cyrillic-el>          # el
'l'  'B'  <solid-left-half>          # Solid block left half
'l'  'L'  <lower-left-heavy-vert>          # "Lower Left Corner" (||_)
'l'  'R'  <lower-right-heavy-vert>          # "Lower Right Corner" (_||)
'l'  'T'  <left-mid-heavy-vert>          # "Left T Intersection" (|=))
'l'  'j'  <Cyrillic-lje>          # lje
'l'  'l'  <lower-left>          # "Lower Left Corner" (|_)
'l'  'r'  <lower-right>          # "Lower Right Corner" (_|)
'l'  't'  <left-mid>          # "Left T Intersection" (|-)
'm'  '*'  <mu>          # Greek letter mu
'm'  '='  <Cyrillic-em>          # em
'n'  '''  <n-acute>          # n acute
'n'  '*'  <nu>          # nu
'n'  ','  <n-cedilla>          # n cedilla
'n'  '<'  <n-caron>          # n caron
'n'  '='  <Cyrillic-en>          # en
'n'  '?'  <n-tilde>          # n tilde
'n'  'g'  <eng>          # eng
'n'  'j'  <Cyrillic-nje>          # nje
'n'  '~'  <n-tilde>          # n tilde
'o'  '!'  <o-grave>          # o grave
'o'  '"'  <o-double-acute>          # o double acute
'o'  '%'  <omikron-accent>          # omikron accent
'o'  '''  <o-acute>          # o acute
'o'  '*'  <omikron>          # omikron
'o'  '-'  <o-macron>          # o macron 
'o'  '/'  <o-slash>          # o slash
'o'  ':'  <o-diaeresis>          # o diaeresis
'o'  '='  <Cyrillic-o>          # o
'o'  '>'  <o-circumflex>          # o circumflex
'o'  '?'  <o-tilde>          # o tilde
'o'  '^'  <o-circumflex>          # o circumflex
'o'  '`'  <o-grave>          # o grave
'o'  '~'  <o-tilde>          # o tilde
'p'  '*'  <pi>          # Greek letter pi
'p'  '='  <Cyrillic-pe>          # pe
'p'  's'  <pound-sign>          # pound sign
'q'  '*'  <psi>          # psi
'r'  '''  <r-acute>          # r acute
'r'  '*'  <rho>          # rho
'r'  ','  <r-cedilla>          # r cedilla
'r'  '<'  <r-caron>          # r caron
'r'  '='  <Cyrillic-er>          # er
'r'  'T'  <right-mid-heavy-vert>          # "Right T Intersection" (=|)
'r'  't'  <right-mid>          # "Right T Intersection" (-|)
's'  '%'  <Cyrillic-sha>          # sha
's'  '''  <s-acute>          # s acute
's'  '*'  <sigma>          # Greek letter sigma
's'  ','  <s-cedilla>          # s cedilla
's'  '<'  <s-caron>          # s caron
's'  '='  <Cyrillic-es>          # es
's'  '>'  <s-circumflex>          # s circumflex
's'  '^'  <s-circumflex>          # s circumflex
's'  'c'  <Cyrillic-shcha>          # shcha
's'  'q'  <square-solid>          # solid square
's'  's'  <sharp-s>          # German double s
't'  '*'  <tau>          # Greek letter tau
't'  ','  <t-cedilla>          # t cedilla
't'  '/'  <t-stroke>          # t slash
't'  '<'  <t-caron>          # t caron
't'  '='  <Cyrillic-te>          # te
't'  'h'  <thorn>          # thorn
't'  's'  <Serbian-tshe>          # tshe (Serbocroatian)
'u'  '!'  <u-grave>          # u grave
'u'  '"'  <u-double-acute>          # u double acute
'u'  '%'  <upsilon-accent>          # upsilon accent
'u'  '''  <u-acute>          # u acute
'u'  '('  <u-breve>          # u breve
'u'  '*'  <upsilon>          # upsilon
'u'  ','  <u-cedilla>          # u cedilla
'u'  '-'  <u-macron>          # u macron
'u'  '.'  <u-ring>          # u ring above
'u'  ':'  <u-diaeresis>          # u diaeresis
'u'  '='  <Cyrillic-u>          # u
'u'  '>'  <u-circumflex>          # u circumflex
'u'  '?'  <u-tilde>          # u tilde
'u'  'L'  <upper-left-heavy-vert>          # "Upper Left Corner" (||~)
'u'  'R'  <upper-right-heavy-vert>          # "Upper Right Corner" (~||))
'u'  'T'  <top-mid-heavy-vert>          # "Upper T Intersection" (~||~)
'u'  '^'  <u-circumflex>          # u circumflex
'u'  '`'  <u-grave>          # u grave
'u'  'l'  <upper-left>          # "Upper Left Corner" (|~)
'u'  'r'  <upper-right>          # "Upper Right Corner" (~|)
'u'  't'  <top-mid>          # "Upper T intersection" (~|~)
'u'  '|'  <micro-sign>          # micro
'u'  '~'  <u-tilde>          # u tilde
'u'  '%'  <Byelorussian-short-u>          # short u (Byelorussian)
'v'  '%'  <upsilon-acc-diaeresis>          # upsilon with diaeresis and accent
'v'  '*'  <upsilon-diaeresis>          # upsilon diaeresis
'v'  '='  <Cyrillic-ve>          # ve
'v'  'l'  <vertical>          # "Vertical Line" (-)
'w'  '%'  <omega-accent>          # omega accent
'w'  '*'  <omega>          # omega
'x'  '*'  <chi>          # chi
'x'  'T'  <intersect-heavy-horiz>          # "Middle Cross Intersection" (-||-)
'x'  't'  <intersection>          # "Middle Cross(Intersection" (-|-)
'y'  '%'  <eta-accent>          # eta accent
'y'  '''  <y-acute>          # y acute
'y'  '*'  <eta>          # eta
'y'  '-'  <yen-sign>          # yen sign
'y'  ':'  <y-diaeresis>          # y diaeresis
'y'  '='  <Cyrillic-yeru>          # yeru
'y'  'i'  <Ukrainian-yi>          # yi (Ukrainian)
'z'  '%'  <Cyrillic-zhe>          # zhe
'z'  '''  <z-acute>          # z acute
'z'  '*'  <zeta>          # zeta
'z'  '.'  <z-dot>          # z dot above
'z'  '<'  <z-caron>          # z caron
'z'  '='  <Cyrillic-ze>          # ze
'|'  '|'  <broken-bar>          # broken bar
'~'  '~'  <macron>          # macron
#
#
# The following output section maps '9b' (an ANSI CSI code,
# which unfortunately is also an IBM character) such that
# it prints.
#
output:
0x9b	0x1b 0x9b
#
#
#
scancodes:
#scan codes for el_EL
#      NORM    SHIFT     ALT   ALT_SHIFT
0x3     '2'     '"'     '@'|C   '@'|C
0x4	'3'	<middle-dot>	'3'|N	<middle-dot>|N
0x10	<less-than-or-equal>	<greater-than-or-equal>	'q'|C	'Q'|C
0x11	<sigma-final>	<square-root-sign>	'w'|C	'W'|C
0x12	<epsilon>	<EPSILON>	'e'|C	'E'|C	CAPS
0x13	<rho>	<RHO>	'r'|C	'R'|C	CAPS
0x14	<tau>	<TAU>	't'|C	'T'|C	CAPS
0x15	<upsilon>	<UPSILON>	'y'|C	'Y'|C	CAPS
0x16	<theta>	<THETA>	'u'|C	'U'|C	CAPS
0x17	<iota>	<IOTA>	'i'|C	'I'|C	CAPS
0x18	<omikron>	<OMIKRON>	'o'|C	'O'|C	CAPS
0x19	<pi>	<PI>	'p'|C	'P'|C	CAPS
0x1a	'['|C	'{'	'['|N	'{'|N
0x1b	']'|C	'}'	']'|N	'}'|N
0x1e	<alpha>	<ALPHA>	'a'|C	'A'|C	CAPS
0x1f	<sigma>	<SIGMA>	's'|C	'S'|C	CAPS
0x20	<delta>	<DELTA>	'd'|C	'D'|C	CAPS
0x21	<phi>	<PHI>	'f'|C	'F'|C	CAPS
0x22	<gamma>	<GAMMA>	'g'|C	'G'|C	CAPS
0x23	<eta>	<ETA>	'h'|C	'H'|C	CAPS
0x24	<xi>	<XI>	'j'|C	'J'|C	CAPS
0x25	<kappa>	<KAPPA>	'k'|C	'K'|C	CAPS
0x26	<lambda>	<LAMBDA>	'l'|C	'L'|C	CAPS
0x28	'''	<diaeresis>	<diaeresis-tonos>	-
0x2b	'#'	'~'	'#'|N	'~'|N
0x2c	<zeta>	<ZETA>	'z'|C	'Z'|C	CAPS
0x2d	<chi>	<CHI>	'x'|C	'X'|C	CAPS
0x2e	<psi>	<PSI>	'c'|C	'C'|C	CAPS
0x2f	<omega>	<OMEGA>	'v'|C	'V'|C	CAPS
0x30	<beta>	<BETA>	'b'|C	'B'|C	CAPS
0x31	<nu>	<NU>	'n'|C	'N'|C	CAPS
0x32	<mu>	<MU>	'm'|C	'M'|C	CAPS
0x29	'<'	'>'	'<'|N	'>'|N
0x56	'\'|C	'|'	'\'|N	'|'|N
#
# map CTRL SHIFT F1  to be 0x18 for the compose character key
F37     0x18
# map CTRL SHIFT F2 to be 0x14 for the toggle key
F38     0x14
