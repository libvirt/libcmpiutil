/*
 * Copyright IBM Corp. 2006, 2007
 *
 * Authors:
 *  Gareth Bestor <bestorga@us.ibm.com>
 *  Dan Smith <danms@us.ibm.com>
 *  Richard Maciel <richardm@br.ibm.com>
 *
 */
	/*** WARNING - COMMENTS IN LEX MUST BE TAB INDENTED ***/

	/* DEFINITIONS SECTION */
%{
#include <cmpidt.h>
#include <string.h>
#include "eo_util_parser.h"
#ifndef YY_FLEX_LEX_COMPAT
int eolineno = 1;
#endif

void eo_parse_error(char *errmsg);
%}

DIGIT		([0-9])
ALPHA		([A-Za-z])
ALPHANUMERIC	([A-Za-z0-9])
PUNCTUATION	([\,\.\:\(\)\-\!\"\'\`\^])
ARITHMETIC	([\+\-\*\/\<\>\=])
WHITESPACE	([\t ]+)
EOLN		(\n)
NUMBER		([0-9]+)
INTEGER		(-?[0-9]+)
REAL		(-?[0-9]+\.[0-9]+)
WORD		([A-Za-z]+)
QUOTEDTEXT	(\"[^"\n]*["\n])
SINGLEQUOTEDTEXT	(\'[^'\n]*['\n])
FILENAME	([A-Za-z0-9\.][A-Za-z0-9\.\-\+\_]*)
PATHNAME	(\/?[A-Za-z0-9\.][A-Za-z0-9\.\-\+\_]*(\/[A-Za-z0-9\.][A-Za-z0-9\.\-\+\_]*)*\/?)
HOSTNAME	([A-Za-z][A-Za-z0-9\-\_]*(\.[A-Za-z][A-Za-z0-9\-\_]*)*)
IPADDR		([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)

	/* Special mode to read CLASSNAME after reading INSTANCE OF,
	 * otherwise cannot distinguish this token from a PROPERTYNAME */
%x READCLASSNAME

%option  noyywrap

	/* END OF DEFINITIONS SECTION */
%%
	/* RULES SECTION */
instance |
Instance |
INSTANCE {
	return(INSTANCE);
	}

of |
Of |
OF 	{
        BEGIN READCLASSNAME; /* Read in classname as next token */
	return(OF);
	}

true |
True |
TRUE	{
	eo_parse_lval.boolean = 1;
	return(BOOLEAN);
	}

false |
False |
FALSE	{
	eo_parse_lval.boolean = 0;
	return(BOOLEAN);
	}

null |
Null |
NULL	{
	return(CIMNULL);
	}

{INTEGER} {
	eo_parse_lval.sint64 = atoll(eo_parse_text);
	return(INTEGER);
	}

{QUOTEDTEXT} |
{SINGLEQUOTEDTEXT} {
	/* FIXME: This looks dubious */
	eo_parse_lval.string = (char *)strdup(eo_parse_text+1);
        eo_parse_lval.string[strlen(eo_parse_lval.string)-1] = '\0';
	return(STRING);
	}

\{      {
        return(OPENBRACKET);
        }

\}      {
        return(CLOSEBRACKET);
        }

\,      {
        return(COMMA);
        }

	/* NOTE - this rule only applies after a 'INSTANCE OF' has been read in */
<READCLASSNAME>[A-Za-z][A-Za-z0-9_]* {
	BEGIN INITIAL; /* Go back to normal parsing rules now */
	eo_parse_lval.string = (char *)strdup(eo_parse_text);
	return(CLASS);
	}

	/* Propertyname */
[A-Za-z][A-Za-z0-9_]* {
        eo_parse_lval.string = (char *)strdup(eo_parse_text);
        return(PROPERTYNAME);
        }

	/* Ignore end-of-line */
{EOLN}	{
	eolineno++;
	}

<<EOF>> {
	return(ENDOFFILE);
	}

	/* Ignore whitespace */
{WHITESPACE} ;

	/* Return single character punctuation characters as-is */
.       {
	return(eo_parse_text[0]);
	}

	/* END OF RULES SECTION */
%%
	/* USER SUBROUTINE SECTION */
	/* Everything below is copied verbatim to the end of the lex generated C code. */

void eo_parse_error(char *errmsg)
{
   fprintf(stderr, "error line %d: %s in '%s'\n",
           eolineno, errmsg, eo_parse_text);
}

