
/* client-psk.c
 *
 or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <errno.h>
#include    <arpa/inet.h>
#include    <signal.h>
#include    <unistd.h>
#include    <wolfssl/ssl.h>  /* must include this to use wolfSSL security */

#define     MAXLINE 256      /* max text line length */
#define     SERV_PORT 11111  /* default port*/

/*
 *psk client set up.
 */
static inline unsigned int My_Psk_Client_Cb(WOLFSSL* ssl, const char* hint,
        char* identity, unsigned int id_max_len, unsigned char* key, 
        unsigned int key_max_len)
{
    (void)ssl;
    (void)hint;
    (void)key_max_len;

    /* identity is OpenSSL testing default for openssl s_client, keep same*/
    strncpy(identity, "Client_identity", id_max_len);

    /* test key n hex is 0x1a2b3c4d , in decimal 439,041,101, we're using
     * unsigned binary */
    key[0] = 26;
    key[1] = 43;
    key[2] = 60;
    key[3] = 77;

    return 4;
}

/*
 * this function will send the inputted string to the server and then 
 * recieve the string from the server outputing it to the termial
 */ 
int SendReceive(WOLFSSL* ssl)
{
    char sendline[MAXLINE]="Hello Server"; /* string to send to the server */
    char recvline[MAXLINE]; /* string received from the server */
        
	/* write string to the server */
	if (wolfSSL_write(ssl, sendline, MAXLINE) != sizeof(sendline)) {
		printf("Write Error to Server\n");
		return 1;
    }
        
	/* flags if the Server stopped before the client could end */
    if (wolfSSL_read(ssl, recvline, MAXLINE) < 0 ) {
    	printf("Client: Server Terminated Prematurely!\n");
        return 1;
    }

    /* show message from the server */
	printf("Server Message: %s\n", recvline);
        	
	return 0;
}

int main(int argc, char **argv)
{
    int ret, sockfd;
    WOLFSSL* ssl;
    WOLFSSL_CTX* ctx;
    struct sockaddr_in servaddr;;

    /* must include an ip address of this will flag */
    if (argc != 2) {
        printf("Usage: tcpClient <IPaddress>\n");
        return 1;
    }
    
    wolfSSL_Init();  /* initialize wolfSSL */
    
    /* create and initialize WOLFSSL_CTX structure */
    if ((ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
        fprintf(stderr, "SSL_CTX_new error.\n");
        return 1;
    }
                
    /* create a stream socket using tcp,internet protocal IPv4,
     * full-duplex stream */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    /* places n zero-valued bytes in the address servaddr */
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);

    /* converts IPv4 addresses from text to binary form */
    ret = inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    if (ret != 1) {
        printf("inet_pton error\n");    
		return 1;
    }
    
    /* set up pre shared keys */
    wolfSSL_CTX_set_psk_client_callback(ctx, My_Psk_Client_Cb);
	
    /* attempts to make a connection on a socket */
    ret = connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    
    if (ret != 0) {
        printf("Connection Error\n");
        return 1;
    }
    
    /* creat wolfssl object after each tcp connct */
    if ( (ssl = wolfSSL_new(ctx)) == NULL) {
        fprintf(stderr, "wolfSSL_new error.\n");
        return 1;
    }
	
    /* associate the file descriptor with the session */
    ret = wolfSSL_set_fd(ssl, sockfd);
	
    if (ret != SSL_SUCCESS){
        return 1;
    }
	
    /* takes inputting string and outputs it to the server */
	ret = SendReceive(ssl);
	if(ret != 0){
		return 1;
	}

    /* cleanup */
    wolfSSL_free(ssl);

    /* when completely done using SSL/TLS, free the 
     * wolfssl_ctx object */
    wolfSSL_CTX_free(ctx);
    wolfSSL_Cleanup();

    /* exit client */
    return ret;
}
