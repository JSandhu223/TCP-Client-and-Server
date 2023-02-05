#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

// #define PORT 3496     // the port the client will connect to

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: ./server PORT\n");
        exit(-1);
    }

    const int PORT = atoi(argv[1]);

    int sock;   // Client socket
    int status; // Used for error checking

    // Address initialization
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = inet_addr("127.0.0.1"); // "127.0.0.1" is the loopback address

    // Socket creation
    // AF_INET means we are using the IPv4 protocol
    // SOCK_STREAM is for TCP socket stream
    // 0 means we let the bastard figure out the protocol based on the second argument
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        printf("client: error creating socket\n");
        return 1;
    }

    // Connect to server
    // We have to cast the superior sockaddr_in to the inferior sockaddr unfortunately.
    status = connect(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_in));
    if (status == -1)
    {
        printf("client: failed to connect\n");
        return 2;
    }

    // Prompt the user for their UCID
    std::cout << "Enter your UCID (enter \"q!\" to quit): ";
    char ucid[100];
    // fgets(ucid, MAXDATASIZE, stdin);
    fgets(ucid, sizeof(ucid), stdin);
    size_t ucid_length = strlen(ucid);
    ucid[ucid_length - 1] = '\0'; // Set the null terminator just to be safe ;)

    // Send the UCID to the server
    status = send(sock, ucid, strlen(ucid) + 1, 0);
    if (status == -1)
    {
        printf("Client: failed to send UCID\n");
        return 3;
    }

    if (strcmp(ucid, "q!") == 0)
    {
        close(sock);
        exit(0);
    }

    char date_and_time[25];
    // Receive date and time from server
    status = recv(sock, date_and_time, 100, 0);
    if (status == -1)
    {
        printf("client: Failed to receive date and time from server\n");
        return 4;
    }
    std::cout << "Received date and time: " << date_and_time << std::endl;

    // The passcode is generated by both the client and server!
    char client_passcode[5];
    char ucid_last_four[5] = {ucid[4], ucid[5], ucid[6], ucid[7], '\0'}; // Extract the last four digits from the UCID
    int ucid_to_int = atoi(ucid_last_four);                              // Convert the last four UCID digits to an int
    char seconds[3] = {date_and_time[17], date_and_time[18], '\0'};      // Extract seconds from date_and_time as string
    int seconds_to_int = atoi(seconds);                                  // Convert seconds to int
    int passcode_as_int = ucid_to_int + seconds_to_int;                  // Add the ucid and passcodes that were converted to ints
    sprintf(client_passcode, "%d", passcode_as_int);                     // Convert the passcode from int to string

    // Now we send the passcode to the server
    status = send(sock, client_passcode, 7, 0);
    if (status == -1)
    {
        printf("client: failed to send PASSCODE\n");
        return 5;
    }
    std::cout << "Sending PASSCODE..." << std::endl;

    // Receive file data and write to output file
    char received_data[2048];
    FILE *output_file = fopen("output.txt", "w");

    status = recv(sock, received_data, sizeof(received_data), 0);
    if (status == -1)
    {
        printf("client: Failed to receive file data from server\n");
        return 6;
    }

    printf("Succesfully received file data from server: %s\n", received_data);
    fwrite(received_data, 1, status, output_file);

    fclose(output_file);

    close(sock);

    return 0;
}