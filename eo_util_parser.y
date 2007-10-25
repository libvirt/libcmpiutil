/*
 * Copyright IBM Corp. 2006, 2007
 *
 * Authors:
 *  Gareth Bestor <bestorga@us.ibm.com>
 *  Dan Smith <danms@us.ibm.com>
 */
/* DEFINITIONS SECTION */

%{
#include <stdlib.h>
#include <stdio.h>

#include "cmpidt.h"
#include "cmpift.h"

    /* specify prototypes to get rid of warnings */
int eo_parse_lex (void);
void eo_parse_error(char *);

#define RC_OK 0
#define RC_EOF EOF

/* DEFINE ANY GLOBAL VARS HERE */
static CMPIBroker * _BROKER;
static CMPIInstance ** _INSTANCE;

#ifdef EODEBUG
#define EOTRACE(fmt, arg...) fprintf(stderr, fmt, ##arg)
#else
#define EOTRACE(fmt, arg...)
#endif

int eo_parse_parseinstance(CMPIBroker *broker, CMPIInstance **instance);

%}

/* List all possible CIM property types that can be returned by the lexer */
%union {
   /* Note - we override the CIM definition of string to make this data type
      easier to handle in the lexer/parser. Instead implemented as simple text string. */
   char *               string;
   CMPIBoolean          boolean;
   CMPISint64           sint64;
}

/* Define simple (untyped) lexical tokens */
%token INSTANCE OF ENDOFFILE

 /* Define lexical tokens that return a value and their return type */
%token <string> CLASSNAME
%token <string> PROPERTYNAME
%token <string> STRING
%token <boolean> BOOLEAN
%token <sint64> INTEGER

%%

/* Rules section */

instance:	/* empty */
	|	INSTANCE OF CLASSNAME '{'
			{
			EOTRACE("classname = %s\n",$3);
			CMPIObjectPath *op;
			op = CMNewObjectPath(_BROKER,
					     "root/ibmsd",
					     $3,
					     NULL);
			*_INSTANCE = CMNewInstance(_BROKER,
						   op,
						   NULL);
			free($3);
			}
		properties '}' ';'
			{
			/* Return after reading in each instance */
			return RC_OK;
			}

	|       ENDOFFILE { return RC_EOF; }
	;

properties:	/* empty */
	|	ENDOFFILE
	|	property
	|	property properties
	;

property:	PROPERTYNAME '=' STRING ';'
			{
			EOTRACE("propertyname = %s\n"
				"\ttype = CMPI_chars\n"
				"\tvalue = %s\n",
				$1, $3);
			CMSetProperty(*_INSTANCE, $1, $3, CMPI_chars);
			free($1);
			free($3);
			}

	|	PROPERTYNAME '=' INTEGER ';'
			{
			EOTRACE("propertyname = %s"
				"\ttype = CMPI_sint64\n"
				"\tvalue = %lld\n",
				$1, $3);
			unsigned long long value = $3;
			CMSetProperty(*_INSTANCE, $1, &(value), CMPI_uint64);
			free($1);
			}

	|	PROPERTYNAME '=' BOOLEAN ';'
			{
			EOTRACE("propertyname = %s\n"
				"\ttype = CMPI_boolean\n"
				"\tvalue = %d\n",
				$1, $3);
			CMSetProperty(*_INSTANCE, $1, &($3), CMPI_boolean);
			free($1);
			}
	;

/* END OF RULES SECTION */
%%

/* USER SUBROUTINE SECTION */

int eo_parse_parseinstance(CMPIBroker *broker, CMPIInstance **instance)
{
   _BROKER = broker;
   _INSTANCE = instance;

   /* Parse the next instance */
   return(eo_parse_parse());
}

