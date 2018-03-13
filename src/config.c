#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "messages.h"

static CHAR* trim( CHAR* src )
{
    CHAR* end = strchr( src, '\0' );
    while (isspace(*src)) src++;
    while ((isspace(*(end-1)) && (end > src))) *--end = '\0';
    return src;
}

static BOOL ParseDirective( CONFIG* config, CHAR* name, CHAR* value, BOOL inHeader )
{
    /* Quick way to reference the current image */
    IMAGE* img = config->Images;

    if (stricmp(name, "Timeout") == 0)
    {
        ULONG timeout;
        CHAR* endptr;

        if (inHeader)
        {
            PrintMessage( MSG_CONF_DIR_INSIDE_SECTION, name );
            return FALSE;
        }

        timeout = strtoul( value, &endptr, 0 );
        if (endptr == value)
        {
            PrintMessage( MSG_CONF_INVALID_INTEGER, name );
            return FALSE;
        }

        config->Timeout = timeout;
    }
    else if (!inHeader)
    {
        PrintError( MSG_CONF_DIR_OUTSIDE_SECTION, name );
        return FALSE;
    }
    else if (stricmp(name, "Address") == 0)
    {
        CHAR* endptr;

        img->Address = strtoul( value, &endptr, 0 );
        if (endptr == value)
        {
            img->Address = 0;
        }
    }
    else if (stricmp(name, "Command") == 0)
    {
        CHAR* copy = malloc( strlen( value ) + 1 );
        if (copy == NULL)
        {
            PrintMessage( MSG_OUT_OF_MEMORY );
            return FALSE;
        }
        strcpy( copy, value );

        img->Command = copy;
    }
    else if (stricmp(name, "Drive") == 0)
    {
        CHAR* endptr;

        img->Drive = strtoul( value, &endptr, 0 );
        if ((endptr == value) || (img->Drive > 0xFF))
        {
            /*
             * Default to none if invalid drive is given.
             * This will get filled in by the bootstrap code to the drive it
             * was loaded from.
             */
            img->Drive = 0xFFFFFFFF;
        }
    }
    else if (stricmp(name, "Module") == 0)
    {
        CHAR* str = malloc( strlen(value) + 1 );
        if (str == NULL)
        {
            PrintMessage( MSG_OUT_OF_MEMORY );
            return FALSE;
        }
        strcpy( str, value );

        if (img->nModules % 8 == 0)
        {
            /* Expand array */
            MODULE* tmp = realloc( img->Modules, (img->nModules + 8) * sizeof(MODULE) );
            if (tmp == NULL)
            {
                PrintMessage( MSG_OUT_OF_MEMORY );
                return FALSE;
            }
            img->Modules = tmp;
        }

        /*
         * Note that the string member should not include the file path.
         * This will get removed when the file has been loaded.
         */
        img->Modules[ img->nModules ].ModStart = 0;
        img->Modules[ img->nModules ].ModEnd   = 0;
        img->Modules[ img->nModules ].String   = str;
        img->Modules[ img->nModules ].Reserved = 0;
        img->nModules++;
    }
    else if (stricmp(name, "Type") == 0)
    {
        if      (stricmp(value, "Binary")      == 0) img->Type = IT_BINARY;
        else if (stricmp(value, "Multiboot")   == 0) img->Type = IT_MULTIBOOT;
        else if (stricmp(value, "Bootsector")  == 0) img->Type = IT_BOOTSECTOR;
        else if (stricmp(value, "Relocatable") == 0) img->Type = IT_RELOCATABLE;
    }
    else if (stricmp(name, "Default") == 0)
    {
        CHAR* endptr;
        ULONG val;

        val = strtoul( value, &endptr, 10 );
        if (endptr == value)
        {
            val = (stricmp( value, "Yes" ) == 0);
        }

        if (val != 0)
        {
            if (config->Default != NULL)
            {
                PrintMessage( MSG_CONF_MULTIPLE_DEFAULTS );
                return FALSE;
            }

            config->Default = img;
        }
    }

    return TRUE;
}

static BOOL ValidateConfig( CONFIG* config )
{
    IMAGE* img = config->Images;

    while (img != NULL)
    {
        /* Check for missing required options */
        if (img->Command == NULL)
        {
            PrintMessage( MSG_CONF_MISSING_PROPERTY, img->Name, "Command" );
            return FALSE;
        }

        if ((img->Type == IT_BINARY) && (img->Address == 0))
        {
            PrintMessage( MSG_CONF_MISSING_PROPERTY, img->Name, "Address" );
            return FALSE;
        }

        img = img->Next;
    }

    if (config->Default == NULL)
    {
        /* Make the first entry the default one if none is given */
        config->Default = config->Images;
        while (config->Default->Next != NULL)
        {
            config->Default = config->Default->Next;
        }
    }

    return TRUE;
}

static IMAGE* CreateImage( CHAR* name )
{
    IMAGE* image = malloc( sizeof(IMAGE) );
    if (image == NULL)
    {
        return NULL;
    }

    /* Defaults */
    image->Type        = IT_AUTO;
    image->Address     = 0;
    image->Drive       = 0xFFFFFFFF;
    image->Command     = NULL;
    image->Modules     = NULL;
    image->nModules    = 0;
    image->Name        = malloc( strlen( name ) + 1 );
    if (image == NULL)
    {
        free( image );
        return NULL;
    }
    strcpy( image->Name, name );

    return image;
}

BOOL ParseConfigFile( FILE* file, CONFIG* config )
{
    ULONGLONG size     = GetFileSize( file );
    CHAR*     buffer   = malloc( size + 1 );
    BOOL      inHeader = FALSE;
    ULONG     line     = 1;
    CHAR*     src;
    CHAR*     nl;

    if (buffer == NULL)
    {
        PrintMessage( MSG_OUT_OF_MEMORY );
        return FALSE;
    }

    config->nImages = 0;
    config->Default = NULL;
    config->Images  = NULL;
    config->Timeout = 120;      /* Default to two minutes */

    size = ReadFile( file, buffer, size );
    if (size == 0)
    {
        return FALSE;
    }

    /* Remove NULs */
    nl = buffer;
    while ((nl = memchr( nl, '\0', buffer + size - nl )) != NULL)
    {
        memmove( nl, nl + 1, buffer + size - nl );
        size--;
    }
    buffer[ size ] = '\0';

    /* Replace \r\n and \r with \n */
    nl = buffer;
    while ((nl = memchr( nl, '\r', buffer + size - nl )) != NULL)
    {
        if (nl[1] == '\n')
        {
            memmove( nl + 1, nl + 2, buffer + size - nl - 1 );
            size--;
        }
        *nl = '\n';
    }

    src  = buffer;
    do
    {
        CHAR* sc;

        nl = strchr( src, '\n' );
        if (nl != NULL) *nl = '\0';

        /* Ignore comments */
        sc = strchr( src, ';' );
        if (src != NULL) *sc = '\0';

        src = trim( src );

        /* Ignore empty lines */
        if (*src != '\0')
        {
            if (*src == '[')
            {
                /* Section Header */
                CHAR* name;
                name = strchr( src, ']' );
                if (name == NULL)
                {
                    PrintMessage( MSG_CONF_ERROR_AT_LINE, line );
                    free( buffer );
                    return FALSE;
                }
                *name = '\0';
                name  = trim( src + 1 );

                /* Create a new image structure */
                IMAGE* image = CreateImage( name );
                if (image == NULL)
                {
                    PrintMessage( MSG_OUT_OF_MEMORY );
                    return FALSE;
                }

                /* Add to list */
                image->Next    = config->Images;
                config->Images = image;
                config->nImages++;

                inHeader = TRUE;
            }
            else
            {
                /* Normal line */
                CHAR* eq    = strchr( src, '=' );
                CHAR* name  = src;
                CHAR* value = eq + 1;
                if (eq == NULL)
                {
                    PrintMessage( MSG_CONF_ERROR_AT_LINE, line );
                    free( buffer );
                    return FALSE;
                }
                *eq = '\0';

                name  = trim( name );
                value = trim( value );

                if (!ParseDirective( config, name, value, inHeader ))
                {
                    free( buffer );
                    return FALSE;
                }
            }
        }

        line = line + 1;
        src  = nl   + 1;
    } while (nl != NULL);

    free( buffer );

    return ValidateConfig( config );
}
