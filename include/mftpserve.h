#ifndef __MFTP_SERVE_H__
#define __MFTP_SERVE_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define DEBUG
/* MAX LENGTH OF THE UNIX COMMANDLINE */
//#define TCP_NETWORK_BUFFER_LENGTH        1
#define UNIX_MAX_PATH_LENGTH             4096
//#define TCP_BUFFER_CAPACITY              UNIX_MAX_PATH_LENGTH
/* PORT MACROS */
#define PORT_DEFAULT                     49999
#define PORT_ANY                         0
/* READ/WRITE BUFFER CHUNK SIZE */
#define TCP_READ_CHUNK                   1
#define TCP_WRITE_CHUNK                  4096
/* READ/WRITE BUFFER SIZE*/
#define TCP_READBUFFER_LENGTH            4096
#define TCP_WRITEBUFFER_LENGTH           4096

#ifdef  DEBUG            
    #define DEBUG_PRINT( ... ) fprintf( stdout, __VA_ARGS__ ); 
#else
    #define DEBUG_PRINT( ... )
#endif

/*=====================================================================
   struct __tcp_server_host 
  =====================================================================*/
struct __tcp_server_host
{
    /* Connection Functions */
    void (*dctor            )( struct __tcp_server_host* this_ptr );
    void (*get_socket       )( struct __tcp_server_host* this_ptr );
    void (*bind_socket      )( struct __tcp_server_host* this_ptr );
    void (*set_queue        )( struct __tcp_server_host* this_ptr, int size );
    void (*connect          )( struct __tcp_server_host* this_ptr );
    pid_t(*spawn_handler    )( struct __tcp_server_host* this_ptr );
    void (*write            )( struct __tcp_server_host* this_ptr, char* format, ... );
    char*(*read             )( struct __tcp_server_host* this_ptr );
    void (*close            )( struct __tcp_server_host* this_ptr );
    
    /* Connection Data */
    struct sockaddr_in m_saddr; // Host socket address
    struct sockaddr_in m_caddr; // Client socket address
    struct hostent   * m_chost;
    struct in_addr   **m_pptr;
    int                m_port;
    int                m_socketfd;  
    int                m_connectionfd;
    /**/
    char*              m_clienthostname;
    char*              m_connectionip;
    char*              m_readbuffer;
    char*              m_writebuffer;
    int                m_readbuffersize;
    int                m_writebuffersize;
};
/*=====================================================================
  CreateTCPClient() - Creates a TCP connection Object
  =====================================================================
    -host : Name or ip of host in string format.
    -port : Port number in little endian.
*/
struct __tcp_server_host* CreateTCPHost( int port );

/*=====================================================================
  DestroyTCPHost() - Creates a TCP connection Object
  =====================================================================
    tcp_client : Client Object you want to close.
*/
void DestroyTCPHost( struct __tcp_server_host* tcp_host );

/*=====================================================================
   command ID Enumarator
  =====================================================================*/
enum __mftp_command_type 
{
    MCT_EXIT = 0,
    MCT_RCD,
    MCT_RLS,
    MCT_GET,
    MCT_PUT,
    MCT_DATA,
    MCT_UNKNOWN
};

/*=====================================================================
   MFTP command return value
  =====================================================================*/
enum __mftp_serve_command_status
{
    MSCS_SUCCESS     = 0,
    MSCS_ERROR       = 1,
    MSCS_EXIT        = 2
};

/*=====================================================================
   struct __mftp_command_serve
  =====================================================================*/
struct __mftp_command_serve
{
    /* Function Tupe */
    enum __mftp_serve_command_status (*command)( struct __tcp_server_host*, struct __tcp_server_host**, char* );
    /* Data */
    char m_commandVar;
};

/*=====================================================================
   CreateServerCommand
  =====================================================================*/
struct __mftp_command_serve* CreateServerCommand( char commandVar, enum __mftp_serve_command_status (*command)( struct  __tcp_server_host*, struct __tcp_server_host**, char* ) );

/*=====================================================================
   DestroyServerCommand
  =====================================================================*/
void DestroyServerCommand( struct __mftp_command_serve* );

/*=====================================================================
    GenerateServerCommandSet
  =====================================================================*/
struct __mftp_command_serve** GenerateServerCommandSet( void );

/*=====================================================================
    DestroyServerCommandSet
  =====================================================================*/
void DestroyServerCommandSet( struct __mftp_command_serve** object_list );
  
#endif