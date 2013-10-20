#include "musicProtocol.h"

#define RCVBUFSIZE 512

/**
* encode
*
* Format: request|data|len
*/
size_t Encode ( const msg_t *msg, uint8_t *outBuf, const size_t bufSize )
{
    int i;
    int count = sprintf ( outBuf, "%s\t", msg->request );

    /* If not valid, pad with ellipses */
    if ( msg->len == 0 )
        count += sprintf ( outBuf + count, "..." );

    for ( i = 0; i < msg->len; i++ )
    {
        count += sprintf ( outBuf + count, "%s|", msg->filenames[i] );
    }

    sprintf ( outBuf + count, "\t%d", msg->len );
    return 1;
}

bool Decode ( uint8_t *inBuf, const size_t mSize, msg_t *msg )
{
    char tmp[256];
    int i;
    memset ( tmp, 0, sizeof ( tmp ) );

    /* Parse request */
    char *token = strtok (  inBuf, DELIM_INFO );
    if ( token == NULL || ( strcmp ( token, LIST ) != 0 && strcmp ( token, DIFF ) != 0 && strcmp ( token, PULL ) != 0 && strcmp ( token, LEAVE ) != 0 ) )
    {
        printf ( "ERROR: Command not recognized\n" );
        return false;
    }
    strcpy ( msg->request, token );

    /* Parse data to tmp */
    token = strtok ( NULL, DELIM_INFO );
    strcpy ( tmp, token );

    /* Parse len */
    token = strtok ( NULL, DELIM_INFO );
    msg->len = atoi ( token );

    /* Parse tmp tokens */
    token = strtok ( tmp, "|" );
    strcpy ( msg->filenames[0], token );
    for ( i = 1; i < msg->len; i++ )
    {
        token = strtok ( NULL, "|" );
        strcpy ( msg->filenames[i], token );
    }

    return true;
}
