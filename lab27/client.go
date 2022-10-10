package main

import (
	"fmt"
	"net"
	"os"
	"time"
)

func main() {
	conn, err := net.Dial("tcp", "127.0.0.1:8080")
	if err != nil {
		fmt.Printf("Dial failed, error: %d", err)
		os.Exit(1)
	}

	ans := make([]byte, 255)
	msg := os.Args[1]
	for i := 0; i < 5; i++ {
		conn.Write([]byte(msg))
		time.Sleep(5 * time.Second)
		n, _ := conn.Read(ans)
		fmt.Printf("%s", ans[:n])
	}
	conn.Close()
}
