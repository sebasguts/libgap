/****************************************************************************
**
*F  records.c                   GAP source                   Martin Schoenert
**
*H  @(#)$Id$
**
*Y  Copyright (C)  1996,  Lehrstuhl D fuer Mathematik,  RWTH Aachen,  Germany
**
**  This file contains the functions of the generic record package.
**
**  This package  provides a uniform  interface to  the functions that access
**  records and the elements for the other packages in the GAP kernel.
*/
char *          Revision_records_c =
   "@(#)$Id$";

#include        "system.h"              /* Ints, UInts                     */
#include        "scanner.h"             /* Pr                              */
#include        "gasman.h"              /* NewBag, CHANGED_BAG             */

#include        "objects.h"             /* Obj, TYPE_OBJ, types            */
#include        "gvars.h"               /* AssGVar, GVarName               */

#include        "calls.h"               /* NewFunctionC                    */
#include        "opers.h"               /* NewOperationC, DoOperation2Args */

#define INCLUDE_DECLARATION_PART
#include        "records.h"             /* declaration part of the package */
#undef  INCLUDE_DECLARATION_PART

#include        "bool.h"                /* True, False                     */

#include        "plist.h"               /* NEW_PLIST, SET_LEN_PLIST, SET...*/
#include        "string.h"              /* NEW_STRING, CSTR_STRING         */

#include        "gap.h"                 /* Error                           */


/****************************************************************************
**
*F  CountRnam . . . . . . . . . . . . . . . . . . . .  number of record names
**
**  'CountRnam' is the number of record names.
*/
UInt            CountRNam;


/****************************************************************************
**
*F  NAME_RNAM(<rnam>) . . . . . . . . . . . . . . . .  name for a record name
**
**  'NAME_RNAM' returns the name (as a C string) for the record name <rnam>.
**
**  Note that 'NAME_RNAM' is a  macro, so do not call  it with arguments that
**  have sideeffects.
**
**  'NAME_RNAM' is defined in the declaration part of this package as follows
**
#define NAME_RNAM(rnam) CSTR_STRING( ELM_PLIST( NamesRNam, rnam ) )
*/
Obj             NamesRNam;


/****************************************************************************
**
*F  RNamName(<name>)  . . . . . . . . . . . . convert a name to a record name
**
**  'RNamName' returns  the record name with the  name  <name> (which is  a C
**  string).
*/
Obj             HashRNam;

UInt            SizeRNam;

UInt            RNamName (
    Char *              name )
{
    Obj                 rnam;           /* record name (as imm intobj)     */
    UInt                pos;            /* hash position                   */
    Char                namx [1024];    /* temporary copy of <name>        */
    Obj                 string;         /* temporary string object <name>  */
    Obj                 table;          /* temporary copy of <HashRNam>    */
    Obj                 rnam2;          /* one element of <table>          */
    Char *              p;              /* loop variable                   */
    UInt                i;              /* loop variable                   */

    /* start looking in the table at the following hash position           */
    pos = 0;
    for ( p = name; *p != '\0'; p++ ) {
        pos = 65599 * pos + *p;
    }
    pos = (pos % SizeRNam) + 1;

    /* look through the table until we find a free slot or the global      */
    while ( (rnam = ELM_PLIST( HashRNam, pos )) != 0
         && SyStrncmp( NAME_RNAM( INT_INTOBJ(rnam) ), name, 1023 ) ) {
        pos = (pos % SizeRNam) + 1;
    }

    /* if we did not find the global variable, make a new one and enter it */
    /* (copy the name first, to avoid a stale pointer in case of a GC)     */
    if ( rnam == 0 ) {
        CountRNam++;
        rnam = INTOBJ_INT(CountRNam);
        SET_ELM_PLIST( HashRNam, pos, rnam );
        namx[0] = '\0';
        SyStrncat( namx, name, 1023 );
        string = NEW_STRING( SyStrlen(namx) );
        SyStrncat( CSTR_STRING(string), namx, SyStrlen(namx) );
        GROW_PLIST(    NamesRNam,   CountRNam );
        SET_LEN_PLIST( NamesRNam,   CountRNam );
        SET_ELM_PLIST( NamesRNam,   CountRNam, string );
        CHANGED_BAG(   NamesRNam );
    }

    /* if the table is too crowed, make a larger one, rehash the names     */
    if ( SizeRNam < 3 * CountRNam / 2 ) {
        table = HashRNam;
        SizeRNam = 2 * SizeRNam + 1;
        HashRNam = NEW_PLIST( T_PLIST, SizeRNam );
        SET_LEN_PLIST( HashRNam, SizeRNam );
        for ( i = 1; i <= (SizeRNam-1)/2; i++ ) {
            rnam2 = ELM_PLIST( table, i );
            if ( rnam2 == 0 )  continue;
            pos = 0;
            for ( p = NAME_RNAM( INT_INTOBJ(rnam2) ); *p != '\0'; p++ ) {
                pos = 65599 * pos + *p;
            }
            pos = (pos % SizeRNam) + 1;
            while ( ELM_PLIST( HashRNam, pos ) != 0 ) {
                pos = (pos % SizeRNam) + 1;
            }
            SET_ELM_PLIST( HashRNam, pos, rnam2 );
        }
    }

    /* return the record name                                              */
    return INT_INTOBJ(rnam);
}


/****************************************************************************
**
*F  RNamIntg(<intg>)  . . . . . . . . . . convert an integer to a record name
**
**  'RNamIntg' returns the record name corresponding to the integer <intg>.
*/
UInt            RNamIntg (
    Int                 intg )
{
    Char                name [16];      /* integer converted to a string   */
    Char *              p;              /* loop variable                   */

    /* convert the integer to a string                                     */
    p = name + sizeof(name);  *--p = '\0';
    do {
        *--p = '0' + intg % 10;
    } while ( (intg /= 10) != 0 );

    /* return the name                                                     */
    return RNamName( p );
}


/****************************************************************************
**
*F  RNamObj(<obj>)  . . . . . . . . . . .  convert an object to a record name
**
**  'RNamObj' returns the record name  corresponding  to  the  object  <obj>,
**  which currently must be a string or an integer.
*/
UInt            RNamObj (
    Obj                 obj )
{
    /* convert integer object                                              */
    if ( IS_INTOBJ(obj) ) {
        return RNamIntg( INT_INTOBJ(obj) );
    }

    /* convert string object (empty string may have type T_PLIST)          */
    else if ( IsStringConv(obj) && MUTABLE_TYPE(TYPE_OBJ(obj))==T_STRING ) {
        return RNamName( CSTR_STRING(obj) );
    }

    /* otherwise fail                                                      */
    else {
        obj = ErrorReturnObj(
            "Record: '<rec>.(<obj>)' <obj> must be a string or an integer",
            0L, 0L,
            "you can return a string or an integer for <obj>" );
        return RNamObj( obj );
    }
}


/****************************************************************************
**
*F  RNamObjHandler(<self>,<obj>)  . . . .  convert an object to a record name
**
**  'RNamObjHandler' implements the internal function 'RNamObj'.
**
**  'RNamObj( <obj> )'
**
**  'RNamObj' returns the record name  corresponding  to  the  object  <obj>,
**  which currently must be a string or an integer.
*/
Obj             RNamObjFunc;

Obj             RNamObjHandler (
    Obj                 self,
    Obj                 obj )
{
    return INTOBJ_INT( RNamObj( obj ) );
}


/****************************************************************************
**
*F  NameRNamHandler(<self>,<rnam>)  . . . . convert a record name to a string
**
**  'NameRNamHandler' implements the internal function 'NameRName'.
**
**  'NameRName( <rnam> )'
**
**  'NameRName' returns the string corresponding to the record name <rnam>.
*/
Obj             NameRNamFunc;

Obj             NameRNamHandler (
    Obj                 self,
    Obj                 rnam )
{
    Obj                 name;
    while ( ! IS_INTOBJ(rnam)
         || INT_INTOBJ(rnam) <= 0
        || CountRNam < INT_INTOBJ(rnam) ) {
        rnam = ErrorReturnObj(
            "NameRName: <rnam> must be a record name (not a %s)",
            (Int)(InfoBags[TYPE_OBJ(rnam)].name), 0L,
            "you can return a record name for <rnam>" );
    }
    name = NEW_STRING( SyStrlen( NAME_RNAM( INT_INTOBJ(rnam) ) ) );
    SyStrncat( CSTR_STRING(name),
               NAME_RNAM( INT_INTOBJ(rnam) ),
               SyStrlen( NAME_RNAM( INT_INTOBJ(rnam) ) ) );
    return name;
}


/****************************************************************************
**
*F  IS_REC(<obj>) . . . . . . . . . . . . . . . . . . . is an object a record
*V  IsRecFuncs[<type>]  . . . . . . . . . . . . . . . . table of record tests
**
**  'IS_REC' returns a nonzero value if the object <obj> is a  record  and  0
**  otherwise.
**
**  Note that 'IS_REC' is a record, so do not call  it  with  arguments  that
**  sideeffects.
**
**  'IS_REC' is defined in the declaration part of this package as follows
**
#define IS_REC(obj)     ((*IsRecFuncs[ TYPE_OBJ(obj) ])( obj ))
*/
Int             (*IsRecFuncs[LAST_REAL_TYPE+1]) ( Obj obj );

Obj             IsRecFilt;

Obj             IsRecHandler (
    Obj                 self,
    Obj                 obj )
{
    return (IS_REC(obj) ? True : False);
}

Int             IsRecNot (
    Obj                 obj )
{
    return 0L;
}

Int             IsRecYes (
    Obj                 obj )
{
    return 1L;
}

Int             IsRecObject (
    Obj                 obj )
{
    return (DoFilter( IsRecFilt, obj ) == True);
}


/****************************************************************************
**
*F  ELM_REC(<rec>,<rnam>) . . . . . . . . . . select an element from a record
**
**  'ELM_REC' returns the element, i.e., the value of the component, with the
**  record name <rnam> in  the record <rec>.   An error is signalled if <rec>
**  is not a record or if <rec> has no component with the record name <rnam>.
**
**  Note that 'ELM_REC' is  a macro, so do   not call it with arguments  that
**  have sideeffects.
**
**  'ELM_REC' is defined in the declaration part of this package as follows
**
#define ELM_REC(rec,rnam) \
                        ((*ElmRecFuncs[ TYPE_OBJ(rec) ])( rec, rnam ))
*/
Obj             (*ElmRecFuncs[LAST_REAL_TYPE+1]) ( Obj rec, UInt rnam );

Obj             ElmRecOper;

Obj             ElmRecHandler (
    Obj                 self,
    Obj                 rec,
    Obj                 rnam )
{
    return ELM_REC( rec, INT_INTOBJ(rnam) );
}

Obj             ElmRecError (
    Obj                 rec,
    UInt                rnam )
{
    rec = ErrorReturnObj(
        "Record Element: <rec> must be a record (not a %s)",
        (Int)(InfoBags[TYPE_OBJ(rec)].name), 0L,
        "you can return a record for <rec>" );
    return ELM_REC( rec, rnam );
}

Obj             ElmRecObject (
    Obj                 obj,
    UInt                rnam )
{
    return DoOperation2Args( ElmRecOper, obj, INTOBJ_INT(rnam) );
}


/****************************************************************************
**
*F  ISB_REC(<rec>,<rnam>) . . . . . . . . . test for an element from a record
**
**  'ISB_REC' returns 1 if the record <rec> has a component with  the  record
**  name <rnam> and 0 otherwise.  An error is signalled if  <rec>  is  not  a
**  record.
**
**  Note  that 'ISB_REC'  is a macro,  so do not call  it with arguments that
**  have sideeffects.
**
**  'ISB_REC' is defined in the declaration part of this package as follows
**
#define ISB_REC(rec,rnam) \
                        ((*IsbRecFuncs[ TYPE_OBJ(rec) ])( rec, rnam ))
*/
Int             (*IsbRecFuncs[LAST_REAL_TYPE+1]) ( Obj rec, UInt rnam );

Obj             IsbRecOper;

Obj             IsbRecHandler (
    Obj                 self,
    Obj                 rec,
    Obj                 rnam )
{
    return (ISB_REC( rec, INT_INTOBJ(rnam) ) ? True : False);
}

Int             IsbRecError (
    Obj                 rec,
    UInt                rnam )
{
    rec = ErrorReturnObj(
        "IsBound: <rec> must be a record (not a %s)",
        (Int)(InfoBags[TYPE_OBJ(rec)].name), 0L,
        "you can return a record for <rec>" );
    return ISB_REC( rec, rnam );
}

Int             IsbRecObject (
    Obj                 obj,
    UInt                rnam )
{
    return (DoOperation2Args( IsbRecOper, obj, INTOBJ_INT(rnam) ) == True);
}


/****************************************************************************
**
*F  ASS_REC(<rec>,<rnam>,<obj>) . . . . . . . . . . . . .  assign to a record
**
**  'ASS_REC' assigns the object <obj>  to  the  record  component  with  the
**  record name <rnam> in the record <rec>.  An error is signalled  if  <rec>
**  is not a record.
**
**  'ASS_REC' is defined in the declaration part of this package as follows
**
#define ASS_REC(rec,rnam,obj) \
                        ((*AssRecFuncs[ TYPE_OBJ(rec) ])( rec, rnam, obj ))
*/
void            (*AssRecFuncs[LAST_REAL_TYPE+1]) ( Obj rec, UInt rnam, Obj obj );

Obj             AssRecOper;

Obj             AssRecHandler (
    Obj                 self,
    Obj                 rec,
    Obj                 rnam,
    Obj                 obj )
{
    ASS_REC( rec, INT_INTOBJ(rnam), obj );
    return 0;
}

void            AssRecError (
    Obj                 rec,
    UInt                rnam,
    Obj                 obj )
{
    rec = ErrorReturnObj(
        "Record Assignment: <rec> must be a record (not a %s)",
        (Int)(InfoBags[TYPE_OBJ(rec)].name), 0L,
        "you can return a record for <rec>" );
    ASS_REC( rec, rnam, obj );
}

void            AssRecObject (
    Obj                 obj,
    UInt                rnam,
    Obj                 val )
{
    DoOperation3Args( AssRecOper, obj, INTOBJ_INT(rnam), val );
}


/****************************************************************************
**
*F  UNB_REC(<rec>,<rnam>) . . . . . . unbind a record component from a record
**
**  'UNB_REC' removes the record component  with the record name <rnam>  from
**  the record <rec>.
**
**  Note that 'UNB_REC' is  a macro, so  do  not call it with  arguments that
**  have sideeffects.
**
**  'UNB_REC' is defined in the declaration part of this package as follows
**
#define UNB_REC(rec,rnam) \
                        ((*UnbRecFuncs[ TYPE_OBJ(rec) ])( rec, rnam ))
*/
void            (*UnbRecFuncs[LAST_REAL_TYPE+1]) ( Obj rec, UInt rnam );

Obj             UnbRecOper;

Obj             UnbRecHandler (
    Obj                 self,
    Obj                 rec,
    Obj                 rnam )
{
    UNB_REC( rec, INT_INTOBJ(rnam) );
    return 0;
}

void            UnbRecError (
    Obj                 rec,
    UInt                rnam )
{
    rec = ErrorReturnObj(
        "Unbind: <rec> must be a record (not a %s)",
        (Int)(InfoBags[TYPE_OBJ(rec)].name), 0L,
        "you can return a record for <rec>" );
    UNB_REC( rec, rnam );
}
        
void            UnbRecObject (
    Obj                 obj,
    UInt                rnam )
{
    DoOperation2Args( UnbRecOper, obj, INTOBJ_INT(rnam) );
}


/****************************************************************************
**
*F  iscomplete( <name>, <len> ) . . . . . . . .  find the completions of name
*F  completion( <name>, <len> ) . . . . . . . .  find the completions of name
*/
UInt            iscomplete_rnam (
    Char *              name,
    UInt                len )
{
    Char *              curr;
    UInt                i, k;

    for ( i = 1; i <= CountRNam; i++ ) {
        curr = NAME_RNAM( i );
        for ( k = 0; name[k] != 0 && curr[k] == name[k]; k++ ) ;
        if ( k == len && curr[k] == '\0' )  return 1;
    }
    return 0;
}

UInt            completion_rnam (
    Char *              name,
    UInt                len )
{
    Char *              curr;
    Char *              next;
    UInt                i, k;

    next = 0;
    for ( i = 1; i <= CountRNam; i++ ) {
        curr = NAME_RNAM( i );
        for ( k = 0; name[k] != 0 && curr[k] == name[k]; k++ ) ;
        if ( k < len || curr[k] <= name[k] )  continue;
        if ( next != 0 ) {
            for ( k = 0; curr[k] != '\0' && curr[k] == next[k]; k++ ) ;
            if ( k < len || next[k] < curr[k] )  continue;
        }
        next = curr;
    }

    if ( next != 0 ) {
        for ( k = 0; next[k] != '\0'; k++ )
            name[k] = next[k];
        name[k] = '\0';
    }

    return next != 0;
}


/****************************************************************************
**
*F  InitRecords() . . . . . . . . . .  initialize the generic records package
**
**  'InitRecords' initializes the generic records package.
*/
void            InitRecords ( void )
{
    UInt                type;           /* loop variable                   */

    /* make the list of names of record names                              */
    CountRNam = 0;
    InitGlobalBag( &NamesRNam );
    NamesRNam = NEW_PLIST( T_PLIST, 0 );
    SET_LEN_PLIST( NamesRNam, 0 );

    /* make the hash list of record names                                  */
    SizeRNam = 997;
    InitGlobalBag( &HashRNam );
    HashRNam = NEW_PLIST( T_PLIST, SizeRNam );
    SET_LEN_PLIST( HashRNam, SizeRNam );

    /* make and install the 'RNamObj' and 'NameRName' functions            */
    RNamObjFunc = NewFunctionC( "RNamObj", 1L, "obj", RNamObjHandler );
    AssGVar( GVarName( "RNamObj" ), RNamObjFunc );
    NameRNamFunc = NewFunctionC( "NameRNam", 1L, "rnam", NameRNamHandler );
    AssGVar( GVarName( "NameRNam" ), NameRNamFunc );

    /* make and install the 'IS_REC' filter                                */
    IsRecFilt = NewFilterC( "IS_REC", 1L, "obj", IsRecHandler );
    AssGVar( GVarName( "IS_REC" ), IsRecFilt );
    for ( type = FIRST_REAL_TYPE; type <= LAST_REAL_TYPE; type++ ) {
        IsRecFuncs[ type ] = IsRecNot;
    }
    for ( type = FIRST_RECORD_TYPE; type <= LAST_RECORD_TYPE; type++ ) {
        IsRecFuncs[ type ] = IsRecYes;
    }
    for ( type = FIRST_EXTERNAL_TYPE; type <= LAST_EXTERNAL_TYPE; type++ ) {
        IsRecFuncs[ type ] = IsRecObject;
    }

    /* make and install the 'ELM_REC' operations                           */
    ElmRecOper = NewOperationC( "ELM_REC", 2L, "obj, rnam",
                                ElmRecHandler );
    AssGVar( GVarName( "ELM_REC" ), ElmRecOper );
    for ( type = FIRST_REAL_TYPE; type <= LAST_REAL_TYPE; type++ ) {
        ElmRecFuncs[ type ] = ElmRecError;
    }
    for ( type = FIRST_EXTERNAL_TYPE; type <= LAST_EXTERNAL_TYPE; type++ ) {
        ElmRecFuncs[ type ] = ElmRecObject;
    }

    /* make and install the 'ISB_REC' operation                            */
    IsbRecOper = NewOperationC( "ISB_REC", 2L, "obj, rnam",
                                IsbRecHandler );
    AssGVar( GVarName( "ISB_REC" ), IsbRecOper );
    for ( type = FIRST_REAL_TYPE; type <= LAST_REAL_TYPE; type++ ) {
        IsbRecFuncs[ type ] = IsbRecError;
    }
    for ( type = FIRST_EXTERNAL_TYPE; type <= LAST_EXTERNAL_TYPE; type++ ) {
        IsbRecFuncs[ type ] = IsbRecObject;
    }

    /* make and install the 'ASS_REC' operation                            */
    AssRecOper = NewOperationC( "ASS_REC", 3L, "obj, rnam, val",
                                AssRecHandler );
    AssGVar( GVarName( "ASS_REC" ), AssRecOper );
    for ( type = FIRST_REAL_TYPE; type <= LAST_REAL_TYPE; type++ ) {
        AssRecFuncs[ type ] = AssRecError;
    }
    for ( type = FIRST_EXTERNAL_TYPE; type <= LAST_EXTERNAL_TYPE; type++ ) {
        AssRecFuncs[ type ] = AssRecObject;
    }

    /* make and install the 'UNB_REC' operation                            */
    UnbRecOper = NewOperationC( "UNB_REC", 2L, "obj, rnam",
                                UnbRecHandler );
    AssGVar( GVarName( "UNB_REC" ), UnbRecOper );
    for ( type = FIRST_REAL_TYPE; type <= LAST_REAL_TYPE; type++ ) {
        UnbRecFuncs[ type ] = UnbRecError;
    }
    for ( type = FIRST_EXTERNAL_TYPE; type <= LAST_EXTERNAL_TYPE; type++ ) {
        UnbRecFuncs[ type ] = UnbRecObject;
    }

}



