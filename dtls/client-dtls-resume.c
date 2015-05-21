#include <unistd.h>
#include <wolfssl/ssl.h>
#include <wolfssl/options.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE   4096
#define SERV_PORT 11111 

/* Send and receive function */
void DatagramClient (WOLFSSL* ssl) 
{
    int  n = 0;
    char sendLine[MAXLINE], recvLine[MAXLINE - 1];

    while (fgets(sendLine, MAXLINE, stdin) != NULL) {
        
       if ( ( wolfSSL_write(ssl, sendLine, strlen(sendLine))) != 
	      strlen(sendLine)) {
            printf("SSL_write failed");
        }

       n = wolfSSL_read(ssl, recvLine, sizeof(recvLine)-1);
       
       if (n < 0) {
            int readErr = wolfSSL_get_error(ssl, 0);
	        if(readErr != SSL_ERROR_WANT_READ)
		        printf("wolfSSL_read failed");
       }

        recvLine[n] = '\0';  
        fputs(recvLine, stdout);
    }
}

int main (int argc, char** argv) 
{
    int     		sockfd = 0;
    struct  		sockaddr_in servAddr;
    const char* 	host = argv[1];
    WOLFSSL* 		ssl = 0;
    WOLFSSL_CTX* 	ctx = 0;
    WOLFSSL* 		sslResume = 0;
    WOLFSSL_SESSION*	session = 0;
    char*    		srTest = "testing session resume";
    char            cert_array[] = "../certs/ca-cert.pem";
    char*           certs = cert_array;
    if (argc != 2) { 
        printf("usage: udpcli <IP address>\n");
        return 1;
    }

    wolfSSL_Init();
    /* wolfSSL_Debugging_ON(); */
   
    if ( (ctx = wolfSSL_CTX_new(wolfDTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new error.\n");
        return 1;
    }

    if (wolfSSL_CTX_load_verify_locations(ctx, certs, 0) != SSL_SUCCESS) {
        fprintf(stderr, "Error loading %s, please check the file.\n", certs);
        return 1;
    }

    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
    	printf("unable to get ssl object");
        return 1;
    }
    
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    if ( (inet_pton(AF_INET, host, &servAddr.sin_addr)) < 1) {
        printf("Error and/or invalid IP address");
        return 1;
    }

    wolfSSL_dtls_set_peer(ssl, &servAddr, sizeof(servAddr));
    
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
       printf("cannot create a socket."); 
       return 1;
    }
    
    wolfSSL_set_fd(ssl, sockfd);
    if (wolfSSL_connect(ssl) != SSL_SUCCESS) {
	    int err1 = wolfSSL_get_error(ssl, 0);
	    char buffer[80];
	    printf("err = %d, %s\n", err1, wolfSSL_ERR_error_string(err1, buffer));
	    printf("SSL_connect failed");
        return 1;
    }
    
    DatagramClient(ssl);
    wolfSSL_write(ssl, srTest, sizeof(srTest));
    session = wolfSSL_get_session(ssl);
    sslResume = wolfSSL_new(ctx);

    wolfSSL_shutdown(ssl);
    wolfSSL_free(ssl);
    close(sockfd);

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(SERV_PORT);
    if ( (inet_pton(AF_INET, host, &servAddr.sin_addr)) < 1) {
        printf("Error and/or invalid IP address");
        return 1;
    }

    wolfSSL_dtls_set_peer(sslResume, &servAddr, sizeof(servAddr));
   
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        printf("cannot create a socket.");
        return 1;
    } 
    
    wolfSSL_set_fd(sslResume, sockfd);
    wolfSSL_set_session(sslResume, session);

    if (wolfSSL_connect(sslResume) != SSL_SUCCESS) { 
	    printf("SSL_connect failed");
        return 1;
    }

    if(wolfSSL_session_reused(sslResume))
    	printf("reused session id\n");
    else
    	printf("didn't reuse session id!!!\n");
    
    DatagramClient(sslResume);
    
    wolfSSL_write(sslResume, srTest, sizeof(srTest));

    wolfSSL_shutdown(sslResume);
    wolfSSL_free(sslResume);
    
    close(sockfd);
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    return 0;
}

