package main

import (
	"fmt"
	"net"
	"strconv"
)

func main() {
	listenSock, err := net.Listen("tcp", "127.0.0.1:8888")
	if err != nil {
		fmt.Printf("Listen failed\n")
	}
	number := 0
	for {
		newConn, err := listenSock.Accept()
		if err != nil {
			fmt.Printf("Accept failed\n")
		}
		fmt.Printf("New connection!\n")
		go connection(newConn, number)
		number++
	}
}

func connection(conn net.Conn, num int) {
	fmt.Printf("Connection read\n")
	for {
		msg := make([]byte, 255)
		n, err := conn.Read(msg)
		if n == 0 {
			fmt.Printf("Closing connection\n")
			conn.Close()
			break
		}
		if err != nil {
			fmt.Printf("read failed %d, %d\n", n, err)
			break
		}

		fmt.Printf("%s\n", msg)
		conn.Write([]byte(strconv.Itoa(num) + ": server received message: " + string(msg[:n]) + "\n"))
	}
}
