# database-server

mdb-lookup-server.c is a program that takes an input, connects to and downloads a database via a TCP socket, and then returns the records that contain the input. You use netcat to connect to the server and send it input.

It only works on databases in a certain custom format, but here's how to run it anyway:  
`./mdb-lookup-server <database file> <server port>`  

---------------

http-client.c is not related to the database server, but it is a first step towards a web server. It is a limited version of wget that can download a single HTML page.

To use it, run   
`./http-client <host> <port> <file path>` (ex: www.gnu.org 80 /software/make/manual/make.html)  

It exits if it receives a response code other than 200 from the webpage.