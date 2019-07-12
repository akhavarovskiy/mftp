#include "..//include//mftpserve.h"

void* cleanup_thread(void* args)
{
    while( 1 ) {
        waitpid(-1, NULL, WNOHANG );
        sleep( 1 );
    }
}

int main( int argc, const char * argv[] )
{
    struct __tcp_server_host    * tcp_host     = CreateTCPHost( PORT_DEFAULT );
    struct __tcp_server_host    * tcp_data     = NULL;
    
    tcp_host->get_socket ( tcp_host );
    tcp_host->bind_socket( tcp_host );
    tcp_host->set_queue  ( tcp_host, 4 );

    pthread_t thread;
    if( !pthread_create( &thread, NULL, cleanup_thread, NULL ) == 0 ){
        fprintf( stderr, "[ Error ] Cant create zombie process cleanup thread.\n" );
        return 0;
    }
    
    while( 1 )
    {   
        tcp_host->connect( tcp_host );

        pid_t pid = tcp_host->spawn_handler( tcp_host );
        if( !pid ) 
        {
            struct __mftp_command_serve** mftp_command = GenerateServerCommandSet();
            for( enum __mftp_serve_command_status scs = MSCS_SUCCESS; scs != MSCS_EXIT; ) 
            { 
                char *cmd = tcp_host->read( tcp_host );
                if( cmd == NULL ) 
                    break; 

                for( struct __mftp_command_serve** iter = mftp_command;  iter != NULL; iter++ ) 
                {
                    if( (*iter) == NULL ) 
                    {
                        DEBUG_PRINT( "[ Debug ] Error : Unknown Command, \"%s\"\n", cmd );
                        tcp_host->write( tcp_host, "EUnknown Command\n" );
                        break;
                    }
                    else
                    if( (*iter)->m_commandVar == cmd[0] )
                    {
                        DEBUG_PRINT(  "[ Debug ] Server Received Command, \"%s\" \n", cmd );  
                        scs = (*iter)->command( tcp_host, &tcp_data, &cmd[1] );
                        break;
                    }
                }
            }
            tcp_host->close( tcp_host );
            tcp_host->dctor( tcp_host );
            DestroyServerCommandSet( mftp_command );
            return 0;
        }
    }
    return 0;
}
/*=====================================================================
  __tcp_server_host_get_socket() - Create a socket
  =====================================================================*/
void  __tcp_server_host_get_socket( struct __tcp_server_host* this_ptr )
{
    this_ptr->m_socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if( this_ptr->m_socketfd < 0 ) {
        perror( "[ Error ] Failed to create socket" ); 
        exit(1);
    }
}
/*=====================================================================
  __tcp_server_host_bind_socket() - Binds the socket to the selected port
  =====================================================================*/
void  __tcp_server_host_bind_socket( struct __tcp_server_host* this_ptr )
{
    socklen_t length = sizeof(struct sockaddr_in);

    struct timeval tv; 
    tv.tv_sec  = 180; lseek
    tv.tv_usec = 0; 

    struct sockaddr_in saddr; 
    memset( &saddr, 0, sizeof(saddr));
    
    memset( &this_ptr->m_saddr, 0, sizeof(this_ptr->m_saddr) );
    this_ptr->m_saddr.sin_family      = AF_INET;
    this_ptr->m_saddr.sin_port        = htons((this_ptr->m_port == 0) ? 0 : this_ptr->m_port );
    this_ptr->m_saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if ( bind( this_ptr->m_socketfd, (struct sockaddr*)&this_ptr->m_saddr, sizeof(this_ptr->m_saddr)) < 0 ) {
        perror("[ Error ] Failed to bind socket" );
        exit(1);
    }
    if( this_ptr->m_port > 0 ) {
        this_ptr->m_port = htons(this_ptr->m_saddr.sin_port);
    }
    else {
        if( getsockname( this_ptr->m_socketfd, (struct sockaddr*)&saddr, &length ) < 0 ) {
            perror("getsockname() failed");
            exit(1);
        }
        else this_ptr->m_port = htons(saddr.sin_port);

        setsockopt( this_ptr->m_connectionfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));        
    }
} 
/*=====================================================================
  _tcp_server_set_queue() - Sets the server queue capacity
  =====================================================================*/
void __tcp_server_set_queue( struct __tcp_server_host* this_ptr, int size )
{
    if( listen(this_ptr->m_socketfd, size) < 0 ) {
        perror("[ Error ] Failed to setup listening queue");
        exit(1);
    }
}
/*=====================================================================
  __tcp_server_host_connect() - Waits for an incomming connection
  =====================================================================*/
void __tcp_server_host_connect( struct __tcp_server_host* this_ptr )
{
    socklen_t length         = sizeof(struct sockaddr_in);
    this_ptr->m_connectionfd = accept( this_ptr->m_socketfd, (struct sockaddr*)&this_ptr->m_caddr, &length );
}
/*=====================================================================
  __tcp_server_host_spawn_handler() -Spawns a handler process
  ====================================================================*/
pid_t __tcp_server_host_spawn_handler( struct __tcp_server_host* this_ptr )
{
    pid_t process_id = fork();
    if( process_id > 0 )
    {
        close( this_ptr->m_connectionfd );
        return process_id;
    }
    else
    if( process_id < 0 )
    {
        perror( "[ Error ] Failed to fork new process for client connection" );
        exit(1);
    }
    else
    {
        close( this_ptr->m_socketfd );
        

        inet_ntop( AF_INET, &this_ptr->m_caddr, this_ptr->m_connectionip, INET_ADDRSTRLEN );

        this_ptr->m_chost = gethostbyaddr(&(this_ptr->m_caddr.sin_addr.s_addr), sizeof(struct in_addr), AF_INET);
        
        if( !this_ptr->m_chost && h_errno != HOST_NOT_FOUND ) {
            herror( "[ Error ] gethostbyaddr() " );
            exit(1); 
        }
        else 
        if( h_errno == HOST_NOT_FOUND ) {
            this_ptr->m_clienthostname = "Unknown client hostname";
        }
        else { 
            this_ptr->m_clienthostname = this_ptr->m_chost->h_name;
        }
        DEBUG_PRINT( "[ Debug ] Connection : Opened |  %s | IP: %s | Port : %d \n", this_ptr->m_clienthostname, this_ptr->m_connectionip, this_ptr->m_port );
        return process_id;
    }
}
/*=====================================================================
  __tcp_server_host_write() - Write to the connection
  =====================================================================
    this_ptr : Pointer to object. 
    format   : String format token
    ...      : token arguments
*/
void __tcp_server_host_write( struct __tcp_server_host* this_ptr, char* format, ... )
{
    va_list args;
    va_start( args, format );
    this_ptr->m_writebuffersize = vsprintf( this_ptr->m_writebuffer, format, args );
    va_end  ( args );

    if( write( this_ptr->m_connectionfd, this_ptr->m_writebuffer, this_ptr->m_writebuffersize ) < 0 )
    {
        fprintf( stderr, "[ Error ] TCPCServer::write() : %s\n", strerror( errno ));
        exit(1);
    }
}
/*=====================================================================
  __tcp_server_host_read() - Read from the connection
  =====================================================================
    this_ptr : Pointer to object. 
    return   : Null terminated string read form client.*/
char* __tcp_server_host_read( struct __tcp_server_host* this_ptr )
{
    this_ptr->m_readbuffer[ 0 ] = '\0';
    this_ptr->m_readbuffersize  =    0;

    for( int n = 0 ; ; this_ptr->m_readbuffersize += n )
    {
        n = read( this_ptr->m_connectionfd, &this_ptr->m_readbuffer[ this_ptr->m_readbuffersize ], TCP_READ_CHUNK ); 
        if( n < 0 )
        {
            fprintf( stderr, "[ Error ] TCPServer::read() : \"%s\" \n", strerror( errno ) );
            return NULL;
        }
        if( this_ptr->m_readbuffer[ this_ptr->m_readbuffersize ] == '\n' )
            break;
    }
    this_ptr->m_readbuffer[ this_ptr->m_readbuffersize ] = '\0';

    DEBUG_PRINT( "[ Debug ] TCPServer::read() : \"%s\", length : %d\n", this_ptr->m_readbuffer, this_ptr->m_readbuffersize );

    return this_ptr->m_readbuffer;
}
/*=====================================================================
  __tcp_server_host_close() - Close the connection
  =====================================================================
    this_ptr : Pointer to object. 
    return   : Null terminated string read form client.
*/
void __tcp_server_host_close( struct __tcp_server_host* this_ptr )
{
    DEBUG_PRINT( "[ Debug ] Connection : Closed | Port : %d\n",  this_ptr->m_port );
    close( this_ptr->m_connectionfd );
    close( this_ptr->m_socketfd );

}
/*=====================================================================
  DestroyTCPHost() - Creates a TCP connection Object
  =====================================================================
    tcp_client : Client Object you want to close.
*/
void DestroyTCPHost( struct __tcp_server_host* tcp_host )
{
    free( tcp_host->m_readbuffer  );
    free( tcp_host->m_writebuffer );
    free( tcp_host->m_connectionip );
    free( tcp_host );
}
/*=====================================================================
  CreateTCPHost() - Creates a TCP connection Object
  =====================================================================
    -host : Name or ip of host in string format.
    -port : Port number in little endian.
*/
struct __tcp_server_host* CreateTCPHost( int port )
{
    struct __tcp_server_host* ret 
                           = calloc( 1, sizeof( struct __tcp_server_host ) );
    ret->m_readbuffer      = calloc( 1, TCP_READBUFFER_LENGTH  ); 
    ret->m_writebuffer     = calloc( 1, TCP_WRITEBUFFER_LENGTH ); 
    ret->m_connectionip    = malloc( 18 );

    ret->dctor             = DestroyTCPHost;
    ret->get_socket        = __tcp_server_host_get_socket;
    ret->bind_socket       = __tcp_server_host_bind_socket;
    ret->set_queue         = __tcp_server_set_queue;
    ret->connect           = __tcp_server_host_connect;
    ret->spawn_handler     = __tcp_server_host_spawn_handler;
    ret->read              = __tcp_server_host_read;
    ret->write             = __tcp_server_host_write;
    ret->close             = __tcp_server_host_close;
    ret->m_port            = port;

    return ret;
}
/*=====================================================================
   CreateServerCommand
  =====================================================================*/
struct __mftp_command_serve* CreateServerCommand( char commandVar, enum __mftp_serve_command_status (*command)(  struct __tcp_server_host*, struct __tcp_server_host**,  char* ) )
{
    struct __mftp_command_serve* ret = malloc( sizeof( struct __mftp_command_serve ) );
    ret->command      = command;
    ret->m_commandVar = commandVar;
    return ret;
}
/*=====================================================================
   DestroyServerCommand
  =====================================================================*/
void DestroyServerCommand( struct __mftp_command_serve* object )
{
    free( object );
}

enum __mftp_serve_command_status __mftp_serve_exit( struct __tcp_server_host* tcp_host, struct __tcp_server_host** tcp_data, char* command_arguments )
{
    DEBUG_PRINT( "[ Debug ] Client Closing connection.\n" );
    tcp_host->write( tcp_host, "A\n" );
    return MSCS_EXIT;
}

enum __mftp_serve_command_status __mftp_serve_rcd( struct __tcp_server_host* tcp_host, struct __tcp_server_host** tcp_data,  char* command_arguments  )
{
    DEBUG_PRINT( "[ Debug ] Server attempting to change server directory to %s\n", command_arguments );
    if( chdir( command_arguments ) < 0 ) {
        DEBUG_PRINT( "[ Debug ] Server sending E command.\n" );
        tcp_host->write( tcp_host, "E%s\n", strerror(errno)); 
        return MSCS_ERROR;
    }
    DEBUG_PRINT(  "[ Debug ] Server sending A command.\n" );
    tcp_host->write( tcp_host, "A\n" ); 
    return MSCS_SUCCESS;    
}

enum __mftp_serve_command_status __mftp_serve_rls( struct __tcp_server_host* tcp_host, struct __tcp_server_host** tcp_data,  char* command_arguments  )
{
    if( tcp_data == NULL )
    {
        fprintf( stderr, "[ Error ] __mftp_serve_rls() : tcp_data object pointer is NULL, exiting.\n" );
        return MSCS_EXIT;
    }
    if( (*tcp_data) == NULL )
    {
        tcp_host->write( tcp_host, "E%s\n", "No active data connection." );
        return MSCS_EXIT;
    }
    tcp_host->write( tcp_host, "A\n" );

    pid_t pid = fork() ;
    if( !pid )
    {
        close ( STDOUT_FILENO );
        dup   ( (*tcp_data)->m_connectionfd );
        close ( (*tcp_data)->m_connectionfd );
        execlp( "ls", "ls", "-l", NULL );
        fprintf( stderr, "[ Error ] execlp(\"more\") %s\n", strerror( errno ) );
        (*tcp_data)->close( (*tcp_data) );     
        (*tcp_data)->dctor( (*tcp_data) );
        return MSCS_EXIT;     
    }
    else {
        wait( NULL );
        (*tcp_data)->close( (*tcp_data) ); 
        (*tcp_data)->dctor( (*tcp_data) );
        return MSCS_SUCCESS;
    }
}

enum __mftp_serve_command_status __mftp_serve_get( struct __tcp_server_host* tcp_host, struct __tcp_server_host** tcp_data,  char* command_arguments  )
{
    if( tcp_data == NULL )
    {
        fprintf( stderr, "[ Error ] __mftp_serve_rls() : tcp_data object pointer is NULL, exiting.\n" );
        return MSCS_EXIT;
    }
    if( (*tcp_data) == NULL )
    {
        tcp_host->write( tcp_host, "E%s\n", "No active data connection." );
        return MSCS_EXIT;
    }

    int fd = open( command_arguments, O_RDONLY, 0644 );
    if( fd < 0 )
    {
        fprintf( stderr, "[ Error ] open() : get, show, %s\n", strerror( errno ) );        
        tcp_host->write( tcp_host, "E%s\n", strerror( errno ) );
        (*tcp_data)->close( (*tcp_data) );
        (*tcp_data)->dctor( (*tcp_data) );
        return MSCS_ERROR;
    }
    tcp_host->write( tcp_host, "A\n" );
    DEBUG_PRINT( "[ Debug ] Server sending  :  A\n" );
    DEBUG_PRINT( "[ Debug ] Started Writing %s\n", command_arguments );    

    char buffer[ TCP_WRITE_CHUNK ];        
    for( int n = read( fd, buffer, TCP_WRITE_CHUNK ); 
             n > 0;
             n = read( fd, buffer, TCP_WRITE_CHUNK )) 
    {
        if( write((*tcp_data)->m_connectionfd, buffer, n ) < 0  ) {
            fprintf( stderr, "[ Error ] %s\n", strerror( errno ) );
            break;
        }
    }
    DEBUG_PRINT( "[ Debug ] Server done writing.\n" );    
    (*tcp_data)->close( (*tcp_data) );
    (*tcp_data)->dctor( (*tcp_data) );
    close( fd );    
    return MSCS_SUCCESS;
}

enum __mftp_serve_command_status __mftp_serve_put( struct __tcp_server_host* tcp_host, struct __tcp_server_host** tcp_data,  char* command_arguments  )
{
    if( tcp_data == NULL )
    {
        fprintf( stderr, "[ Error ] __mftp_serve_rls() : tcp_data object pointer is NULL, exiting.\n" );
        return MSCS_EXIT;
    }
    if( (*tcp_data) == NULL )
    {
        tcp_host->write( tcp_host, "E%s\n", "No active data connection." );
        return MSCS_EXIT;
    }

    int fd = open( command_arguments, O_WRONLY | O_TRUNC | O_EXCL | O_CREAT, 0644 );
    if( fd < 0 )
    {
        tcp_host->write( tcp_host, "E%s\n", strerror( errno ) );
        fprintf( stderr, "[ Error ] open() : put, %s\n", strerror( errno ) );
        return MSCS_ERROR;
    }
    tcp_host->write( tcp_host, "A\n" );

    char buffer[ TCP_WRITE_CHUNK ];
    for( int n = read( (*tcp_data)->m_connectionfd, buffer, TCP_WRITE_CHUNK ); 
             n > 0;
             n = read( (*tcp_data)->m_connectionfd, buffer, TCP_WRITE_CHUNK ) ) 
    {
        if( write( fd, buffer, n ) < 0 )
        {
            fprintf( stderr, "[ Error ] %s\n", strerror( errno ) );
            break;   
        }
    }
    close( fd );
    (*tcp_data)->close( (*tcp_data) );
    (*tcp_data)->dctor( (*tcp_data) );
    return MSCS_SUCCESS;
}

enum __mftp_serve_command_status __mftp_serve_data( struct __tcp_server_host* tcp_host, struct __tcp_server_host** tcp_data,  char* command_arguments  )
{
    (*tcp_data) = CreateTCPHost( PORT_ANY );
    (*tcp_data)->get_socket ( (*tcp_data) );
    (*tcp_data)->bind_socket( (*tcp_data) );
    (*tcp_data)->set_queue  ( (*tcp_data), 1 );

    DEBUG_PRINT( "[ Debug ] Data Connection | Client request : Opening Port %d\n", (*tcp_data)->m_port );
    tcp_host->write( tcp_host, "A%d\n", (*tcp_data)->m_port );

    (*tcp_data)->connect( (*tcp_data) );
    return MSCS_SUCCESS;
}
/*=====================================================================
    GenerateServerCommandSet
  =====================================================================*/
struct __mftp_command_serve** GenerateServerCommandSet( void )
{
    struct __mftp_command_serve** ret = malloc( sizeof( struct __mftp_command_serve* ) * (MCT_UNKNOWN + 1) );
    ret[ MCT_EXIT    ] = CreateServerCommand( 'Q', __mftp_serve_exit  );
    ret[ MCT_RCD     ] = CreateServerCommand( 'C', __mftp_serve_rcd   );
    ret[ MCT_RLS     ] = CreateServerCommand( 'L', __mftp_serve_rls   );
    ret[ MCT_GET     ] = CreateServerCommand( 'G', __mftp_serve_get   );
    ret[ MCT_PUT     ] = CreateServerCommand( 'P', __mftp_serve_put   );
    ret[ MCT_DATA    ] = CreateServerCommand( 'D', __mftp_serve_data  );
    ret[ MCT_UNKNOWN ] = NULL;
    return ret;
}
/*=====================================================================
    DestroyServerCommandSet
  =====================================================================*/
void DestroyServerCommandSet( struct __mftp_command_serve** object_list )
{
    DestroyServerCommand( object_list[ MCT_EXIT ] );
    DestroyServerCommand( object_list[ MCT_RCD  ] );
    DestroyServerCommand( object_list[ MCT_RLS  ] );
    DestroyServerCommand( object_list[ MCT_GET  ] );
    DestroyServerCommand( object_list[ MCT_PUT  ] );
    DestroyServerCommand( object_list[ MCT_DATA ] );
    free( object_list );
}