# README. md


### To open server
    make all
    (sudo) ./server URL PORT
URL must be either IP address or hostname. 
If PORT is not specified it is defaulted to PORT 80 so must append sudo to front of command. If otherwise then must be a valid 4-digit number.
### cURL requests
    PUT: curl -T localfile localhost:8080 --request-target filename_27_characters
    GET: curl localhost:8080 --request-target filename_27_characters
    DELETE: curl -v -X DELETE localhost:8080 filename_27_characters 

Localfile can be any name not necessarily 27 characters. Filename_27_characters must be 27 characers. The last command is used to show 500 Internal Server Error. 

-v was used to check for response header from the server. and -H was used to hide Content-Length:

#### Makefile
    Includes the following commands:
    all,format,clean,spotless
### Bugs
Client does not close automatically when there is a PUT request using an empty file e.i Content length = 0.  
