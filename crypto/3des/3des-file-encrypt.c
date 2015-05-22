/* 3des-file-encrypt.c
 *
 * Copyright (C) 2006-2014 wolfSSL Inc.
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,USA
 */

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/des3.h>
#include <wolfssl/wolfcrypt/sha256.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/pwdbased.h>

#define DES3_BLOCK_SIZE 24               /* size of encryption blocks */
#define SALT_SIZE 8

/*
 * Makes a cyptographically secure key by stretching a user entered key
 */
int GenerateKey(RNG* rng, byte* key, int size, byte* salt, int pad)
{
    int ret;

    ret = wc_RNG_GenerateBlock(rng, salt, SALT_SIZE);
    if (ret != 0)
        return -1020;

    if (pad == 0)        /* sets first value of salt to check if the */
        salt[0] = 0;            /* message is padded */

    /* stretches key */
    ret = wc_PBKDF2(key, key, strlen((const char*)key), salt, SALT_SIZE, 4096, 
        size, SHA256);
    if (ret != 0)
        return -1030;

    return 0;
}

/*
 * Encrypts a file using 3DES 
 */
int Des3Encrypt(Des3* des3, byte* key, int size, FILE* inFile, FILE* outFile)
{
    RNG     rng;
    byte    iv[DES3_BLOCK_SIZE];
    byte*   input;
    byte*   output;
    byte    salt[SALT_SIZE] = {0};

    int     i = 0;
    int     ret = 0;
    int     inputLength;
    int     length;
    int     padCounter = 0;

    fseek(inFile, 0, SEEK_END);
    inputLength = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);

    length = inputLength;
    /* pads the length until it evenly matches a block / increases pad number*/
    while (length % DES3_BLOCK_SIZE != 0) {
        length++;
        padCounter++;
    }

    input = malloc(length);
    output = malloc(length);

    ret = wc_InitRng(&rng);
    if (ret != 0) {
        printf("Failed to initialize random number generator\n");
        return -1030;
    }

    /* reads from inFile and wrties whatever is there to the input array */
    ret = fread(input, 1, inputLength, inFile);
    if (ret == 0) {
        printf("Input file does not exist.\n");
        return -1010;
    }
    for (i = inputLength; i < length; i++) {
        /* padds the added characters with the number of pads */
        input[i] = padCounter;
    }

    ret = wc_RNG_GenerateBlock(&rng, iv, DES3_BLOCK_SIZE);
    if (ret != 0)
        return -1020;

    /* stretches key to fit size */
    ret = GenerateKey(&rng, key, size, salt, padCounter);
    if (ret != 0) 
        return -1040;

    /* sets key */
    ret = wc_Des3_SetKey(des3, key, iv, DES_ENCRYPTION);
    if (ret != 0)
        return -1001;

    /* encrypts the message to the ouput based on input length + padding */
    ret = wc_Des3_CbcEncrypt(des3, output, input, length);
    if (ret != 0)
        return -1005;

    /* writes to outFile */
    fwrite(salt, 1, SALT_SIZE, outFile);
    fwrite(iv, 1, DES3_BLOCK_SIZE, outFile);
    fwrite(output, 1, length, outFile);

    /* closes the opened files and frees the memory*/
    memset(input, 0, length);
    memset(output, 0, length);
    memset(key, 0, size);
    free(input);
    free(output);
    free(key);
    fclose(inFile);
    fclose(outFile);

    return 0;
}

/*
 * Decrypts a file using 3DES 
 */
int Des3Decrypt(Des3* des3, byte* key, int size, FILE* inFile, FILE* outFile)
{
    RNG     rng;
    byte    iv[DES3_BLOCK_SIZE];
    byte*   input;
    byte*   output;
    byte    salt[SALT_SIZE] = {0};

    int     i = 0;
    int     ret = 0;
    int     length;
    int     aSize;

    fseek(inFile, 0, SEEK_END);
    length = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);
    aSize = length;

    input = malloc(aSize);
    output = malloc(aSize);

    wc_InitRng(&rng);

    /* reads from inFile and wrties whatever is there to the input array */
    ret = fread(input, 1, length, inFile);
    if (ret == 0) {
        printf("Input file does not exist.\n");
        return -1010;
    }
    for (i = 0; i < SALT_SIZE; i++) {
        /* finds salt from input message */
        salt[i] = input[i];
    }
    for (i = SALT_SIZE; i < DES3_BLOCK_SIZE + SALT_SIZE; i++) {
        /* finds iv from input message */
        iv[i - SALT_SIZE] = input[i];
    }

    /* replicates old key if keys match */
    ret = wc_PBKDF2(key, key, strlen((const char*)key), salt, SALT_SIZE, 4096, 
        size, SHA256);
    if (ret != 0)
        return -1050;

    /* sets key */
    ret = wc_Des3_SetKey(des3, key, iv, DES_DECRYPTION);
    if (ret != 0)
        return -1002;

    /* change length to remove salt/iv block from being decrypted */
    length -= (DES3_BLOCK_SIZE + SALT_SIZE);
    for (i = 0; i < length; i++) {
        /* shifts message: ignores salt/iv on message*/
        input[i] = input[i + (DES3_BLOCK_SIZE + SALT_SIZE)];
    }
    /* decrypts the message to output based on input length + padding */
    ret = wc_Des3_CbcDecrypt(des3, output, input, length);
    if (ret != 0)
        return -1006;

    if (salt[0] != 0) {
        /* reduces length based on number of padded elements  */
        length -= output[length-1];
    }
    /* writes output to the outFile based on shortened length */
    fwrite(output, 1, length, outFile);

    /* closes the opened files and frees the memory*/
    memset(input, 0, aSize);
    memset(output, 0, aSize);
    memset(key, 0, size);
    free(input);
    free(output);
    free(key);
    fclose(inFile);
    fclose(outFile);

    return 0;
}

/*
 * help message
 */
void help()
{
    printf("\n~~~~~~~~~~~~~~~~~~~~|Help|~~~~~~~~~~~~~~~~~~~~~\n\n");
    printf("Usage: ./3des-encrypt <-option> <KeySize> <file.in> "
        "<file.out>\n\n");
    printf("Options\n");
    printf("-d    Decryption\n-e    Encryption\n-h    Help\n");
}

/*
 * temporarily deisables echoing in terminal for secure key input
 */
int NoEcho(char* key, int size)
{
    struct termios oflags, nflags;

    /* disabling echo */
    tcgetattr(fileno(stdin), &oflags);
    nflags = oflags;
    nflags.c_lflag &= ~ECHO;
    nflags.c_lflag |= ECHONL;

    if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
        printf("Error\n");
        return -1060;
    }

    printf("Key: ");
    fgets(key, size, stdin);
    key[strlen(key) - 1] = 0;

    /* restore terminal */
    if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
        printf("Error\n");
        return -1070;
    }
    return 0;
}

int SizeCheck(int size)
{
    int ret = 0;

    if (size != 56 && size != 112 && size != 168) {
        /* if the entered size does not match acceptable size */
        printf("Invalid 3DES key size\n");
        ret = -1080;
    }

    return ret;
}

int main(int argc, char** argv)
{
    Des3   des3;
    byte*  key;       /* user entered key */
    FILE*  inFile;
    FILE*  outFile;

    const char* in;
    const char* out;

    int    option;    /* choice of how to run program */
    int    ret = 0;   /* return value */
    int    inCheck = 0;
    int    outCheck = 0;
    int    size = 0;
    char   choice = 'n';

    while ((option = getopt(argc, argv, "d:e:i:o:h")) != -1) {
        switch (option) {
            case 'd': /* if entered decrypt */
                size = atoi(optarg);
                ret = SizeCheck(size);
                choice = 'd';
                break;
            case 'e': /* if entered encrypt */
                size = atoi(optarg);
                ret = SizeCheck(size);
                choice = 'e';
                break;
            case 'h': /* if entered 'help' */
                help();
                break;
            case 'i': /* input file */
                in = optarg;
                inCheck = 1;
                inFile = fopen(in, "r");
                break;
            case 'o': /* output file */
                out = optarg;
                outCheck = 1;
                outFile = fopen(out, "w");
                break;
            case '?':
                if (optopt) {
                    printf("Ending Session\n");
                    return -111;
                }
            default:
                abort();
        }
    }

    if (inCheck == 0 || outCheck == 0) {
            printf("Must have both input and output file");
            printf(": -i filename -o filename\n");
    }

    else if (ret == 0 && choice != 'n') {
        key = malloc(size);    /* sets size memory of key */
        ret = NoEcho((char*)key, size);
        if (choice == 'e')
            Des3Encrypt(&des3, key, size, inFile, outFile);
        else if (choice == 'd')
            Des3Decrypt(&des3, key, size, inFile, outFile);
    }
    else if (choice == 'n') {
        printf("Must select either -e or -d for encryption and decryption\n");
    }

    return ret;
}
