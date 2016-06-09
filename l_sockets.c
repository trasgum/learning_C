#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr

int xstrsearch ( char * s1, char * s2 )
{
    int i, j, k ;
    int l1 = strlen ( s1 ) ;
    int l2 = strlen ( s2 ) ;

    for ( i = 0 ; i <= l1 - l2 ; i++ )
    {
        j = 0 ;
        k = i ;
        while ( ( s1[k] == s2[j] ) && ( j < l2 ) )
        {
            k++ ;
            j++ ;
        }
        if ( j == l2 )
            return i ;
    }
    return -1 ;
}


char * http_tokenize(char * message)
{
    char *tkn_message = message;
    const char delim[8] = "\r\n";

    while (( tkn_message = strtok(tkn_message, delim)) != NULL)
    {
        printf("token: %s\n", tkn_message);
        tkn_message = NULL;
    }

    return tkn_message;

}

int main(int argc, char *argv[]) {
	int socket_desc, time_header_pos;
    char *message , server_reply[3000];
    char date[11];

	socket_desc = socket(AF_INET, SOCK_STREAM, 0);

	if (socket_desc == -1)
	{
		printf("Could not create a socket");
	}

	struct sockaddr_in server;

	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 8000 );

	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
    	{
        	puts("connect error");
        	return 1;
    	}
	
	//Send some data
	message = "GET / HTTP/1.0\r\n\r\n";
	if( send(socket_desc , message , strlen(message) , 0) < 0)
	{
	    puts("Send failed");
	    return 1;
	}
	puts("Data Send\n");

	//Receive a reply from the server
	if( recv(socket_desc, server_reply , 3000 , 0) < 0)
	{
	    puts("recv failed");
	}
	puts("Reply received\n");
	//puts(server_reply);

        // printf(http_tokenize(server_reply));
        // http_tokenize(server_reply);
    time_header_pos = xstrsearch(server_reply, "Posix-time:");
    printf("Postion: %d", time_header_pos);
    for(int i=0; i < 10; i++) {
        date[i] = server_reply[time_header_pos + 12 + i];
    }
 

    //date = strstr(server_reply, "\r\n\r\n");
    printf("\nDate from server: %s\n", date);
     
	return 0;

}
