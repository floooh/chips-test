;**************************************************************************
;*
;* FILE  macros.i
;* Copyright (c) 1995-1996, 2002, 2008 Daniel Kahlin <daniel@kahlin.net>
;* Written by Daniel Kahlin <daniel@kahlin.net>
;* $Id: macros.i,v 1.4 2003/10/10 14:15:53 tlr Exp $
;*
;* DESCRIPTION
;*   Useful macros 
;*
;******
	IFNCONST MACROS_I
MACROS_I	EQU	1

;**************************************************************************
;*
;* NAME  INFO, ERROR
;*
;* SYNOPSIS
;*   INFO <str>,<str>,...
;*   ERROR <str>,<str>,...
;*
;* DESCRIPTION
;*   info & error macros.
;*
;******
	MAC	INFO
	echo	"INFO:", {0}
	ENDM
	MAC	ERROR
	echo	"ERROR!:", {0}
	ERR
	ENDM

;**************************************************************************
;*
;* NAME  START_SAMEPAGE, END_SAMEPAGE
;*
;* SYNOPSIS
;*   START_SAMEPAGE [<label>]
;*   END_SAMEPAGE
;* 
;* DESCRIPTION
;*   These macros marks a section that must stay within the same page.
;*   If the condition is violated, assembly will be aborted.
;*   An optional label argument may be given to START_SAMEPAGE.  This
;*   label is output when showing the address range of the section.
;*
;******
	MAC	START_SAMEPAGE
_SAMEPAGE_MARKER	SET	.
_SAMEPAGE_NAME		SET	{0}
	ENDM

	MAC	END_SAMEPAGE
	IF	>_SAMEPAGE_MARKER != >.
	IF	_SAMEPAGE_NAME!=""
	ERROR	"[SAMEPAGE] Page crossing not allowed! (",_SAMEPAGE_NAME,"@",_SAMEPAGE_MARKER,"-",.,")"
	ELSE
	ERROR	"[SAMEPAGE] Page crossing not allowed! (",_SAMEPAGE_MARKER,"-",.,")"
	ENDIF	
	ELSE
	IF	_SAMEPAGE_NAME!=""
	INFO	"[SAMEPAGE] successful SAMEPAGE. (",_SAMEPAGE_NAME,"@",_SAMEPAGE_MARKER,"-",.,")"
	ELSE
	INFO	"[SAMEPAGE] successful SAMEPAGE. (",_SAMEPAGE_MARKER,"-",.,")"
	ENDIF
	ENDIF
	ENDM

	ENDIF ;MACROS.I
; eof






