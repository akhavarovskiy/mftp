#ifndef __MFTP_H__
#define __MFTP_H__

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
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

/* DEBUG FLAG */
//#define DEBUG
/* MAX LENGTH OF THE UNIX COMMANDLINE */
#define UNIX_MAX_PATH_LENGTH             4096
/* PORT MACROS */
#define PORT_DEFAULT                     49999
#define PORT_ANY                         0
/* TCP MACROS */
#define TCP_COMMANDLINE_INDICATOR        "$:"
#define TCP_COMMANDLINE_INDICATOR_LENGTH strlen(TCP_COMMANDLINE_INDICATOR)
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
   struct __tcp_connect_client 
  =====================================================================*/
struct __tcp_connect_client
{
    /* Connection Functions */
    void (*dctor      )( struct __tcp_connect_client* this_ptr );
    void (*get_socket )( struct __tcp_connect_client* this_ptr );
    void (*connect    )( struct __tcp_connect_client* this_ptr );
    void (*write      )( struct __tcp_connect_client* this_ptr, char* format, ... );
    char*(*read       )( struct __tcp_connect_client* this_ptr );
    void (*close      )( struct __tcp_connect_client* this_ptr );

    /* Connection Data */
    struct sockaddr_in m_servAddr;
    struct hostent   * m_hostEntry;
    struct in_addr   **m_pptr;
    char               m_host[UNIX_MAX_PATH_LENGTH];
    int                m_port;
    int                m_socketfd;
    /**/
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
struct __tcp_connect_client* CreateTCPClient( char* host, int port );

/*=====================================================================
  CloseTCPClient() - Creates a TCP connection Object
  =====================================================================
    tcp_client : Client Object you want to close.
*/
void DestroyTCPClient( struct __tcp_connect_client* tcp_client );

/*=====================================================================
   Command ID Enumarator
  =====================================================================*/
enum __mftp_command_type 
{
    MCT_EXIT    = 0,
    MCT_CD      = 1,
    MCT_RCD     = 2,
    MCT_LS      = 3,
    MCT_RLS     = 4,
    MCT_GET     = 5,
    MCT_SHOW    = 6,
    MCT_PUT     = 7,
    MCT_UNKNOWN = 8   
};
/*=====================================================================
   MFTP command return value
  =====================================================================*/
enum __mftp_command_status
{
    MCS_SUCCESS     = 0,
    MCS_ERROR       = 1,
    MCS_EXIT        = 2
};
/*=====================================================================
   struct __mftp_command 
  =====================================================================*/
struct __mftp_command 
{
    /* Functions */
    enum __mftp_command_status (*command)( struct  __tcp_connect_client* tcp_client, char* command_arguments );

    /* Data Variables*/
    enum __mftp_command_type m_commandID;
    char* m_command;
    char  m_symbol;
};
/*=====================================================================
    CreateMFTPCommand()
  =====================================================================*/
struct __mftp_command * CreateMFTPCommand( enum __mftp_command_type commandID, char* command, const char symbol, enum __mftp_command_status (*)( struct  __tcp_connect_client* tcp_client, char* command_arguments ));

/*=====================================================================
    DestroyMFTPCommand()
  =====================================================================*/
void DestroyMFTPCommand( struct __mftp_command * object );

/*=====================================================================
    GenerateCommandSet() - Create Command Set.
  =====================================================================*/
struct __mftp_command **GenerateCommandSet( void );

/*=====================================================================
  DestroyCommandSet() - Destroy Command Set
  =====================================================================*/
void DestroyCommandSet( struct __mftp_command** object_list );

#endif