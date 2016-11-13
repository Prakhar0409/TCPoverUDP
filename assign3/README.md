# Computer Networks Assignment 3

##What is the Problem Statement?

Please check the file `Problem.pdf` in the same repository to lookat th eproblem statement.

##How to get it running?

The `Makefile` provided with the assignment assists to get it up and running. To compile both the `sender` and the `receiver`, you just need to press `make` or `make all`

To individually compile server: `make sender`
To individually compile client: `make receiver`

To run the project. First run the server using `./receiver`. It creates a socket and binds it to the ip address of any of the available network interface cards. Then run the client using `./sender hostname port[8887] loss`

##Contributing

Please feel free to fork and send any pull requests in case you find any errors. Since this is only a basic project, we do not expect a lot of feature requests, but feel free to send them if you feel so.

##License
MIT
