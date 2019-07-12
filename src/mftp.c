#include "..//include//mftp.h"

int main( int argc, char *argv[] )
{
    assert( "[ Assert ] Not enough arguments." && argv[1] && argc > 1 );

    struct __tcp_connect_client* tcp_client = CreateTCPClient( argv[1], PORT_DEFAULT );
    struct __mftp_command**   mftp_commands = GenerateCommandSet();    
    
    tcp_client->get_socket( tcp_client );
    tcp_client->connect   ( tcp_client );

    char * buffer = malloc( TCP_READBUFFER_LENGTH );
    for( enum __mftp_command_status mcs = MCS_SUCCESS; mcs != MCS_EXIT; )
    {
        fprintf( stdout, "%s", TCP_COMMANDLINE_INDICATOR );

        if( !fgets( buffer, UNIX_MAX_PATH_LENGTH, stdin ) )
            break;

        char * str = strtok( buffer, " \n" );
        char * arg = strtok( NULL  ,  "\n" );

        for(  struct __mftp_command **iter = mftp_commands; str != NULL; iter++ )
        {
            if( (*iter) == NULL )
            {
                fprintf( stderr, "[ Error ] Command not found. \n" );
                break;
            }
            else
            if( !strcmp( (*iter)->m_command, str ) )
            {
                mcs = (*iter)->command( tcp_client, arg );
                break;
            }
        }
        fflush( stdout );
    }
    free( buffer );

    tcp_client->close( tcp_client );
    tcp_client->dctor( tcp_client );
    DestroyCommandSet( mftp_commands );
    return 0;
}
/*=====================================================================
  __tcp_connect_client_get_socket() - Attempts to bind selected socket
  =====================================================================*/
void __tcp_connect_client_get_socket( struct __tcp_connect_client* this_ptr )
{
    if((this_ptr->m_socketfd = socket( AF_INET, SOCK_STREAM, 0 )) < 0 )
    {
        fprintf( stderr, "[ Error ] Failed to bind a client socket : %s\n", strerror( errno ) );
        exit(1);
    }
}
/*=====================================================================
  __tcp_connect_client_connect() - Connect to selected socket
  =====================================================================
*/
void __tcp_connect_client_connect( struct __tcp_connect_client* this_ptr )
{
    if( connect(this_ptr->m_socketfd, (struct sockaddr *) &this_ptr->m_servAddr, sizeof(this_ptr->m_servAddr)) < 0 ) {
        perror( "[ Error ] connection error " );
        exit(1);
    }
    inet_ntop( AF_INET, &this_ptr->m_servAddr, this_ptr->m_connectionip, INET_ADDRSTRLEN );
}
/*=====================================================================
  __tcp_connect_client_write() - Write to conection
  =====================================================================
*/
void __tcp_connect_client_write( struct __tcp_connect_client* this_ptr, char* format, ... )
{   
    va_list args;
    va_start( args, format );
    this_ptr->m_writebuffersize = vsprintf( this_ptr->m_writebuffer, format, args );
    va_end(args);

    if( this_ptr->m_readbuffersize > TCP_WRITE_CHUNK ) {
        fprintf( stderr, "[ Error ]TCPCLient::write(), Formatted string exceeds buffer size.\n" );
        return;
    }
    if( write( this_ptr->m_socketfd, this_ptr->m_writebuffer, this_ptr->m_writebuffersize ) < 0 )
    {
        fprintf( stderr, "[ Error ] TCPCLient::write() : %s\n", strerror( errno ));
        exit(1);
    }
}
/*=====================================================================
  __tcp_connect_client_read() - Read connection
  =====================================================================
*/
char* __tcp_connect_client_read( struct __tcp_connect_client* this_ptr )
{
    this_ptr->m_readbuffer[ 0 ] = '\0';
    this_ptr->m_readbuffersize  =    0;

    for( int n = 0 ; ; this_ptr->m_readbuffersize += n )
    {
        n = read( this_ptr->m_socketfd, &this_ptr->m_readbuffer[ this_ptr->m_readbuffersize ], TCP_READ_CHUNK ); 
        if( n < 0 )
        {
            fprintf( stderr, "[ Error ] TCPCLient::read() : %s\n", strerror( errno ) );
            return NULL;
        }
        if( this_ptr->m_readbuffer[ this_ptr->m_readbuffersize ] == '\n' )
            break;
    }
    this_ptr->m_readbuffer[ this_ptr->m_readbuffersize ] = '\0';

    DEBUG_PRINT( "[ Debug ] TCPClient::read() : %s, length : %d\n", this_ptr->m_readbuffer, this_ptr->m_readbuffersize );
    
    return this_ptr->m_readbuffer;
} 
/*=====================================================================
  __tcp_connect_client_close() - Close the socket 
  =====================================================================
*/
void __tcp_connect_client_close( struct __tcp_connect_client* this_ptr )
{   
    close( this_ptr->m_socketfd );
}
/*=====================================================================
  CreateTCPClient() - Creates a TCP connection Object
  =====================================================================
*/
struct __tcp_connect_client* CreateTCPClient( char* host, int port )
{
    struct __tcp_connect_client* ret = malloc( sizeof( struct __tcp_connect_client ) );

    ret->m_readbuffer      = calloc( 1, TCP_READBUFFER_LENGTH  );
    ret->m_writebuffer     = calloc( 1, TCP_WRITEBUFFER_LENGTH );
    ret->m_connectionip    = calloc( 1, 24 );
    
    ret->dctor             = DestroyTCPClient;
    ret->get_socket        = __tcp_connect_client_get_socket;
    ret->connect           = __tcp_connect_client_connect;   
    ret->read              = __tcp_connect_client_read;
    ret->write             = __tcp_connect_client_write;
    ret->close             = __tcp_connect_client_close;

    memcpy( ret->m_host, host, strlen( host ) );
    ret->m_port                = port;

    memset( &ret->m_servAddr, 0, sizeof(ret->m_servAddr));
    ret->m_servAddr.sin_family = AF_INET;
    ret->m_servAddr.sin_port   = htons(port);

    ret->m_hostEntry           = gethostbyname( ret->m_host );
    
    if( !ret->m_hostEntry ) {
        herror( "[ Error ] " );
        exit(1);
    }
    DEBUG_PRINT( "[ Debug : Connection ] Host : %s | IP : %d\n", ret->m_host, port );

    ret->m_pptr = (struct in_addr **) ret->m_hostEntry->h_addr_list;
    memcpy(&ret->m_servAddr.sin_addr, *ret->m_pptr, sizeof(struct in_addr));
    return ret;
}
/*=====================================================================
  CloseTCPClient() - Destroys a TCP connection Object
  ====================================================================
    tcp_client : Client Object you want to close.
*/
void DestroyTCPClient( struct __tcp_connect_client* tcp_client )
{
    free( tcp_client->m_connectionip );
    free( tcp_client->m_readbuffer );
    free( tcp_client->m_writebuffer );
    free( tcp_client );
}
/*=====================================================================
  CreateMFTPCommand() - Create command object
  =====================================================================.
*/
struct __mftp_command * CreateMFTPCommand( enum __mftp_command_type commandID, char* command, const char symbol, enum __mftp_command_status (*func)( struct  __tcp_connect_client* tcp_client, char* command_arguments ))
{
    struct __mftp_command * ret = malloc( sizeof( struct __mftp_command  ) );
    ret->command     = func;
    ret->m_command   = command;
    ret->m_symbol    = symbol;
    ret->m_commandID = commandID;
    return ret;
}
/*=====================================================================
  DestroyMFTPCommand() - Destroy command object
  =====================================================================
*/
void DestroyMFTPCommand( struct __mftp_command * object )
{
    free( object );
}
/*=====================================================================
  __mftp_exit() - Exit the client
  =====================================================================*/
enum __mftp_command_status __mftp_exit( struct  __tcp_connect_client* tcp_client, char* command_arguments )
{
    tcp_client->write( tcp_client, "Q\n");
    char * responce = tcp_client->read( tcp_client );

    if( responce[0] == 'A' )
    {
        DEBUG_PRINT( "[ Debug ] Acknowledgement form server, exiting.\n" );    
        return MCS_EXIT;
    }
    else
    if( responce[0] == 'E' )
    {
        fprintf( stderr, "[ Error ] %s\n", responce );
        return MCS_EXIT;
    };
    fprintf( stderr, "[ Error ] Unknown server responce : %s, exiting.\n", &responce[1] );            
    return MCS_EXIT;
}
/*=====================================================================
  __mftp_cd() - Change the local directory
  =====================================================================*/
enum __mftp_command_status __mftp_cd( struct  __tcp_connect_client* tcp_client, char* command_arguments )
{
    if( command_arguments == NULL ) 
    {
        DEBUG_PRINT( "[ Debug ] cd : Needs an argument." );
        return MCS_ERROR;
    }
    if( chdir( command_arguments ) < 0 ) 
    {
        fprintf( stderr, " cd: \"%s\", %s \n", command_arguments, strerror( errno) );
        return MCS_ERROR;
    }
    return MCS_SUCCESS;
}
/*=====================================================================
  __mftp_rcd() - Change the local directory
  =====================================================================*/
enum __mftp_command_status  __mftp_rcd( struct  __tcp_connect_client* tcp_client, char* command_arguments )
{
    tcp_client->write( tcp_client, "C%s\n", (command_arguments == NULL) ? " " : command_arguments );
    char * responce = tcp_client->read( tcp_client );

    if( responce[0] == 'A' )
    {
        DEBUG_PRINT( "[ Debug ] Server Working Directory : %s\n", command_arguments );  
        return MCS_SUCCESS;
    }
    else
    if( responce[0] == 'E' )
    {
        fprintf( stderr, "rcd : %s\n", &responce[1] );
        return MCS_ERROR;   
    };
    fprintf( stderr, "[ Error ]  Unknown server responce : %s, exiting.\n", &responce[1] );            
    return MCS_EXIT;
}
/*=====================================================================
  __mftp_ls() - Change the local directory
  =====================================================================*/
enum __mftp_command_status __mftp_ls( struct  __tcp_connect_client* tcp_client, char* command_arguments )
{
    pid_t pid = fork();
    if( !pid ) 
    {       
        int fps[2];
        if( pipe( fps ) ) 
        {
            fprintf( stderr, "[ Error ] Cant make pipe %s, __mftp_ls. \n", strerror(errno) );
            exit(1);
        }
        pid = fork();
        if( !pid )
        {
            close( fps[0] );
            close( STDOUT_FILENO );
            dup  ( fps[1] );
            close( fps[1] );
            execlp( "ls", "ls", "-l", command_arguments, NULL );
            fprintf( stdout, "[ Error ] %s\n", strerror(errno) );
            return MCS_EXIT;
        }
        else
        {
            close( fps[1] );
            close( STDIN_FILENO );
            dup  ( fps[0] );
            close( fps[0] );
            execlp( "more", "more", "-20", NULL );
            fprintf( stderr, "[ Error ] %s\n", strerror(errno) );
            return MCS_EXIT;
        }
    }
    else 
    {
        wait( NULL );
        return MCS_SUCCESS;
    }
}
/*=====================================================================
  __mftp_rls() - Change the local directory
  =====================================================================*/
enum __mftp_command_status __mftp_rls( struct  __tcp_connect_client* tcp_client, char* command_arguments )
{
    struct __tcp_connect_client * tcp_data = NULL;

    DEBUG_PRINT( "[ Debug ] Sending a data connection request.\n" );

    tcp_client->write( tcp_client, "D\n" );
    char * responce =  tcp_client->read( tcp_client );

    DEBUG_PRINT( "[ Debug ] Server responce : %s\n", responce );

    if( responce[0] == 'A' )
    {
        DEBUG_PRINT( "[ Debug ] Data Command Responce %c, Port: %d\n", responce[0], atoi( &responce[1] ) );

        int port         = atoi( &responce[1] );
        tcp_data         = CreateTCPClient( tcp_client->m_hostEntry->h_name, port );

        tcp_data->get_socket( tcp_data );
        tcp_data->connect   ( tcp_data );

        DEBUG_PRINT( "[ Debug ] Sending LS command to the server. \n" );

        tcp_client->write( tcp_client, "L\n" );
        responce = tcp_client->read( tcp_client );

        DEBUG_PRINT( "[ Debug ] LS command responce, %s \n", responce );

        if( responce[0] == 'A' )
        {
            pid_t pid = fork();   
            if( !pid )
            {
                close( STDIN_FILENO );
                dup  ( tcp_data->m_socketfd );
                close( tcp_data->m_socketfd );
                execlp( "more", "more", "-20", NULL );
                fprintf( stderr, "[ Error ] execlp(\"more\") %s\n", strerror( errno ) );
                tcp_data->close ( tcp_data );
                DestroyTCPClient( tcp_data ); 
                return MCS_EXIT;
            }
            else 
            {
                tcp_data->close ( tcp_data );
                DestroyTCPClient( tcp_data ); 
                wait( NULL );
                return MCS_SUCCESS;
            }
        }
        else
        if( responce[0] == 'E' )
        {
            fprintf( stderr, "[ Error ] %s\n", &responce[1] );
            tcp_data->close ( tcp_data );
            DestroyTCPClient( tcp_data ); 
            return MCS_ERROR;
        }
    }
    else 
    if( responce[0] == 'E' )
    {
        fprintf( stderr, "[ Error ] %s\n", &responce[1] );
        return MCS_EXIT;
    }
    fprintf( stderr, "[ Error ] Unknown server responce : %s, exiting.\n", responce );
    return MCS_EXIT;
}
/*=====================================================================
  __mftp_get() 
  =====================================================================*/
enum __mftp_command_status __mftp_get( struct  __tcp_connect_client* tcp_client, char* command_arguments )
{        
    /* Create data connection object */
    struct __tcp_connect_client * tcp_data = NULL;

    DEBUG_PRINT( "[ Debug ] Sending a data connection request.\n" );

    tcp_client->write( tcp_client, "D\n" );
    char * responce =  tcp_client->read( tcp_client );

    if( responce[0] == 'A' )
    {
        DEBUG_PRINT( "[ Debug ] Data Command Responce %c, Port: %d\n", responce[0], atoi( &responce[1] ) );
        
        int port         = atoi( &responce[1] );
        tcp_data         = CreateTCPClient( tcp_client->m_host, port );

        tcp_data->get_socket( tcp_data );
        tcp_data->connect   ( tcp_data );

        DEBUG_PRINT( "[ Debug ] Sending G%s command.\n", command_arguments );

        tcp_client->write( tcp_client, "G%s\n", command_arguments );
        responce = tcp_client->read( tcp_client );

        DEBUG_PRINT( "[ Debug ] Responce %c\n", responce[0] );

        if( responce[0] == 'A' )
        {
            int fd = open( command_arguments, O_WRONLY | O_TRUNC | O_EXCL | O_CREAT, 0644 );
            if( fd < 0 )
            {
                fprintf( stderr, "[ Error ] %s \n", strerror( errno ) );
                tcp_data->close( tcp_data );
                tcp_data->dctor( tcp_data );
                return MCS_ERROR;
            }
            char buffer[ 512 ]; // HERE
            for(int n = read( tcp_data->m_socketfd, buffer, 512 ); n > 0; n = read( tcp_data->m_socketfd, buffer, 512 )) {
                write( fd, buffer, n );
            }
            close( fd );
            tcp_data->close( tcp_data );
            tcp_data->dctor( tcp_data );
            return MCS_SUCCESS;
        }
        else
        if( responce[0] == 'E' )
        {
            fprintf( stderr, "[ Error ] %s\n", &responce[1] );
            tcp_data->close( tcp_data );
            tcp_data->dctor( tcp_data );
            return MCS_ERROR;
        }
    }
    else
    if( responce[0] == 'E' )
    {
        fprintf( stderr, "[ Error ] %s\n", &responce[1] );
        return MCS_ERROR;
    }
    fprintf( stderr, "[ Error ] Unknown server responce : %s, exiting.\n", strerror( errno ) );
    return MCS_EXIT;
}
/*=====================================================================
  __mftp_show() - Change the local directory
  =====================================================================*/
enum __mftp_command_status __mftp_show( struct  __tcp_connect_client* tcp_client, char* command_arguments )
{
    DEBUG_PRINT( "[ Debug ] Sending a data connection request.\n" );
    
    /* Create data connection object */
    struct __tcp_connect_client * tcp_data = NULL;

    tcp_client->write( tcp_client, "D\n" );
    char * responce = tcp_client->read( tcp_client );

    if( responce[0] == 'A' )
    {
        DEBUG_PRINT( "[ Debug ] Data Command Responce %c, Port: %d\n", responce[0], atoi( &responce[1] ) );
        
        char* port_str   = strtok( &responce[1], "\n" );
        int port         = atoi( port_str );
        tcp_data         = CreateTCPClient( tcp_client->m_hostEntry->h_name, port );

        tcp_data->get_socket( tcp_data );
        tcp_data->connect   ( tcp_data );

        DEBUG_PRINT( "[ Debug ] Sending Show command to the server. \n" );

        tcp_client->write( tcp_client,"G%s\n", command_arguments );
        responce = tcp_client->read( tcp_client );

        DEBUG_PRINT( "[ Debug ] Show command responce, %s \n", responce );

        if( responce[0] == 'A' )
        {
            pid_t pid = fork();
            if( !pid )
            {
                close( STDIN_FILENO );
                dup  ( tcp_data->m_socketfd );
                close( tcp_data->m_socketfd );
                execlp( "more", "more", "-20", NULL );
                fprintf( stderr, "[ Error ] execlp(\"more\") %s\n", strerror( errno ) );
                tcp_data->close( tcp_data );
                tcp_data->dctor( tcp_data );     
                return MCS_EXIT;       
            }
            else 
            {
                tcp_data->close( tcp_data );
                tcp_data->dctor( tcp_data );
                wait( NULL );            
                return MCS_SUCCESS;
            }
        }
        else
        if( responce[0] == 'E' )
        {
            fprintf( stderr, "[ Error ] %s\n", &responce[1] );
            tcp_data->close( tcp_data );
            tcp_data->dctor( tcp_data );
            return MCS_ERROR;
        }
    }
    else
    if( responce[0] == 'E' )
    {
        fprintf( stderr, "[ Error ] %s\n", &responce[1] );
        return MCS_ERROR;
    }
    fprintf( stderr, "[ Error ] Unknown server responce : %s, exiting.\n", responce );
    return MCS_EXIT;
}
/*=====================================================================
  __mftp_put() - Change the local directory
 =====================================================================*/
enum __mftp_command_status __mftp_put( struct  __tcp_connect_client* tcp_client, char* command_arguments )
{    
    struct __tcp_connect_client * tcp_data = NULL;

    int fd = open( command_arguments, O_RDONLY, 0644 );
    if( fd < 0 ) 
    {
        fprintf( stderr, "[ Error ] Put : open(), %s\n", strerror( errno ) );
        return MCS_ERROR;
    }
    tcp_client->write( tcp_client, "D\n" );
    char * responce = tcp_client->read( tcp_client );

    if( responce[0] == 'A' )
    {
        DEBUG_PRINT( "[ Debug ] Data Command Responce %c, Port: %d\n", responce[0], atoi( &responce[1] ) );
        
        int port         = atoi( &responce[1] );
        tcp_data         = CreateTCPClient( tcp_client->m_host, port );

        tcp_data->get_socket( tcp_data );
        tcp_data->connect   ( tcp_data );

        tcp_client->write( tcp_client, "P%s\n", command_arguments );
        responce = tcp_client->read( tcp_client );

        if( responce[0] == 'A' )
        {
            char buffer[TCP_READBUFFER_LENGTH];
            for( int n = read( fd, buffer, TCP_READBUFFER_LENGTH ); n != 0; n = read( fd, buffer, TCP_READBUFFER_LENGTH ) )
                write( tcp_data->m_socketfd, buffer, n );

            close( fd );
            tcp_data->close( tcp_data );
            tcp_data->dctor( tcp_data );
            return MCS_SUCCESS;
        }
        else
        if( responce[0] == 'E' )
        {
            fprintf( stderr, "[ Error ] %s\n", &responce[1] );
            tcp_data->close( tcp_data );
            tcp_data->dctor( tcp_data );
            return MCS_ERROR;
        }
    }
    else
    if( responce[0] == 'E' )
    {
        fprintf( stderr, "[ Error ] %s\n", &responce[1] );
        return MCS_ERROR;
    }
    fprintf( stderr, "[ Error ] Unknown server responce : %s, exiting.\n", strerror( errno ) );
    return MCS_EXIT;
}
/*=====================================================================
  GenerateCommandSet() - Generate Command Set
  =====================================================================*/
struct __mftp_command ** GenerateCommandSet( void )
{
    struct  __mftp_command ** ret = malloc( sizeof( struct __mftp_command ) * MCT_UNKNOWN );
    ret[MCT_EXIT]    = CreateMFTPCommand( MCT_EXIT, "exit", 'Q' , __mftp_exit );
    ret[MCT_CD  ]    = CreateMFTPCommand( MCT_CD  ,   "cd", '\0', __mftp_cd   );
    ret[MCT_RCD ]    = CreateMFTPCommand( MCT_RCD ,  "rcd", 'C', __mftp_rcd  );
    ret[MCT_LS  ]    = CreateMFTPCommand( MCT_LS  ,   "ls", '\0', __mftp_ls   );
    ret[MCT_RLS ]    = CreateMFTPCommand( MCT_RLS ,  "rls", 'L', __mftp_rls  );
    ret[MCT_GET ]    = CreateMFTPCommand( MCT_GET ,  "get", 'G', __mftp_get  );
    ret[MCT_SHOW]    = CreateMFTPCommand( MCT_EXIT, "show", 'S', __mftp_show );
    ret[MCT_PUT ]    = CreateMFTPCommand( MCT_PUT,   "put", 'P', __mftp_put  );
    ret[MCT_UNKNOWN] = NULL;
    return ret;
}
/*=====================================================================
  DestroyCommandSet() - Destroy Command Set 
  =====================================================================*/
void DestroyCommandSet( struct __mftp_command** object_list )
{   
    DestroyMFTPCommand( object_list[MCT_EXIT] );
    DestroyMFTPCommand( object_list[MCT_CD  ] );
    DestroyMFTPCommand( object_list[MCT_RCD ] );
    DestroyMFTPCommand( object_list[MCT_LS  ] );
    DestroyMFTPCommand( object_list[MCT_RLS ] );
    DestroyMFTPCommand( object_list[MCT_GET ] );
    DestroyMFTPCommand( object_list[MCT_SHOW] );
    DestroyMFTPCommand( object_list[MCT_PUT ] );
    free( object_list );
}