/*
 * Copyright IBM Corp. 2006, 2007
 *
 * Authors:
 *  Gareth Bestor <bestorga@us.ibm.com>
 *  Dan Smith <danms@us.ibm.com>
 *  Richard Maciel <richardm@br.ibm.com>
 */
/* DEFINITIONS SECTION */

%{
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "cmpidt.h"
#include "cmpift.h"

#include "eo_parser_xml.h"

    /* specify prototypes to get rid of warnings */
int eo_parse_lex (void);
void eo_parse_error(char *);

#define RC_OK 0
#define RC_EOF EOF
#define RC_INVALID_CLASS -1000
#define RC_ARR_CREAT_FAILED -2000

/* DEFINE ANY GLOBAL VARS HERE */
static const CMPIBroker * _BROKER;
static CMPIInstance ** _INSTANCE;
static const char * _NAMESPACE;
static CMPICount stringarraysize;
static char **stringarray;
static char *stringarraypropname;


#ifdef EODEBUG
#define EOTRACE(fmt, arg...) fprintf(stderr, fmt, ##arg)
#else
#define EOTRACE(fmt, arg...)
#endif


int eo_parse_parseinstance(const CMPIBroker *broker,
			   CMPIInstance **instance,
			   const char *ns);

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
%token INSTANCE OF ENDOFFILE OPENBRACKET CLOSEBRACKET COMMA

 /* Define lexical tokens that return a value and their return type */
%token <string> CLASS
%token <string> PROPERTYNAME
%token <string> STRING
%token <boolean> BOOLEAN
%token <sint64> INTEGER
%token CIMNULL

%%

/* Rules section */

instance:	/* empty */
	|	INSTANCE OF CLASS OPENBRACKET
			{
			EOTRACE("classname = %s\n",$3);
			CMPIObjectPath *op;
			op = CMNewObjectPath(_BROKER,
					     _NAMESPACE,
					     $3,
					     NULL);
			*_INSTANCE = CMNewInstance(_BROKER,
						   op,
						   NULL);
			if (*_INSTANCE == NULL)
				return RC_INVALID_CLASS;
			free($3);
			}
		properties CLOSEBRACKET ';'
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
                        EOTRACE("propertyname = %s\n", $1); 
                        int rc;
                        CMPIType t = set_int_prop($3, $1, *_INSTANCE);
                        EOTRACE("\ttype = %d\n"
                                "\tvalue = %lld\n", t, $3); 
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
        |       PROPERTYNAME '=' OPENBRACKET                         
                        {
                        EOTRACE("propertyname = %s\n"
				"\ttype = CMPI_charsA\n",
				$1);

                        stringarraysize = 0;
                        stringarraypropname = $1;
                        }
                arrayofstrings CLOSEBRACKET ';' 
                               
	|	PROPERTYNAME '=' CIMNULL ';'
			{
			EOTRACE("propertyname = %s\n"
				"\ttype = NULL\n", $1);
			free($1);
			}
	;

arrayofstrings: STRING 
                        {
                        EOTRACE("\t%s[%u]=%s\n", 
                                propertyname, 
                                stringarraysize, 
                                $1);

                        stringarraysize++;
                        stringarray = (char **)realloc(stringarray, 
                                                       sizeof(char *) * 
                                                       stringarraysize);
                        stringarray[stringarraysize-1] = $1;
                        }
                COMMA arrayofstrings

              
        |       STRING
                        {
                        CMPIArray *arr;
                        CMPICount i;
                        CMPIStatus s = {CMPI_RC_OK, NULL};
                        
                        EOTRACE("\t%s[%u]=%s\n", 
                                propertyname, 
                                stringarraysize, 
                                $1);
                        
                        stringarraysize++;

                        arr = CMNewArray(_BROKER, 
                                         stringarraysize, 
                                         CMPI_string, 
                                         &s);
                        if (s.rc != CMPI_RC_OK || CMIsNullObject(arr)) {
                                EOTRACE("Error creating array\n");
                                goto str_arr_out;
                        }

                        // Values to stringarraysize - 2 are in the
                        // temporary array
                        for (i = 0; i < stringarraysize - 1; i++) {
                                s = ins_chars_into_cmstr_arr(_BROKER,
                                                             arr,
                                                             i,
                                                             stringarray[i]);
                                if (s.rc != CMPI_RC_OK)
                                        goto str_arr_out;
                        }
                        
                        s = ins_chars_into_cmstr_arr(_BROKER, 
                                                     arr,  
                                                     stringarraysize - 1,
                                                     $1);
                        if (s.rc != CMPI_RC_OK)
                                goto str_arr_out;

                        CMSetProperty(*_INSTANCE, 
                                      stringarraypropname,
                                      &arr,
                                      CMPI_stringA);
                        
                       str_arr_out:
                        free(stringarraypropname);
                        for (i = 0; i < stringarraysize - 1; i++)
                                free(stringarray[i]);
                        free($1);

                        if (s.rc != CMPI_RC_OK) {
                                return RC_ARR_CREAT_FAILED; 
                        }
                         
                        }
        ;

/* END OF RULES SECTION */
%%

/* USER SUBROUTINE SECTION */

int eo_parse_parseinstance(const CMPIBroker *broker,
			   CMPIInstance **instance,
			   const char *ns)
{
   _BROKER = broker;
   _INSTANCE = instance;
   _NAMESPACE = ns;

   /* Parse the next instance */
   return(eo_parse_parse());
}

