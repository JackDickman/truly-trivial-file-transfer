# truly-trivial-file-transfer

AUTHOR:		Jack Dickman
PROJECT:	CSC424 Truly Trivial File Transfer
LAST UPDATED:	March 31, 2017


Description:	This program implements TFTP with only read requests (RRQ). It is initiated when the client
		sends a RRQ to a listen port number on the server containing a file name. The server responds 
		with a data packet containing bytes (up to 512) from the file and a block number from a new 
		port number and waits for an acknowledgement from the client. This exchange between the client 
		and the new port number on the server machine continues until the entirety of the file has 
		been sent and received. The server resends a data packet after waiting for a positive
		acknowledgement for four seconds and times out after six retries. The client lingers for three
		seconds after the final data packet is received in case of any resends.

Targets:	build, basic-test, ext-test, clean, submit.

Usage:		ttftp [-vL] [-h host -f filename] port

Server:		ttftp [-vL] port
		The -v and -L flags are optional. The -v runs the server in verbose mode, printing everything
		that is sent and received in its entirety. The -L tells the server to service only one RRQ, 
		looping indefinitely otherwise. The 'port' is a port number for the server to listen for a RRQ.

Client:		ttftp [-v] -h host -f filename port
       		The -v flag is optional. It runs the client in verbose mode, printing everything that is sent
		and received in its entirety. The 'host' is the name of the server machine to send the 
		RRQ to. The 'filename' indicates the name of the file in the server directory to be requested. 
		The 'port' is the listen port number on the server and must match the number given in the 
		server command.
