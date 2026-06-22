// Stress / experiment client for the redis server.
//
// Run from the repo root (server should be listening on 127.0.0.1:6379):
//
//	go run ./stress concurrent   [clients] [pings-each]
//	go run ./stress large        [megabytes]
//	go run ./stress partial      [delay-ms-between-bytes]
//	go run ./stress pipeline     [num-commands]
//	go run ./stress all                              # run every scenario
//
// Address can be overridden:  ADDR=127.0.0.1:6379 go run ./stress ...
//
// IMPORTANT: this talks to the server over TCP only — it never touches the
// server process, so running it will not kill or restart your server.
package main

import (
	"bufio"
	"bytes"
	"fmt"
	"net"
	"os"
	"strconv"
	"sync"
	"sync/atomic"
	"time"
)

// addr returns the server to connect to. Set the ADDR env var to target a
// remote server, e.g. ADDR=1.2.3.4:6379 ; defaults to local.
func addr() string {
	if a := os.Getenv("ADDR"); a != "" {
		return a
	}
	return "161.35.161.112:6379"
	return "127.0.0.1:6379"
}

// RESP-encode a command as an array of bulk strings, e.g. ["PING"].
func respCmd(args ...string) []byte {
	var b bytes.Buffer
	fmt.Fprintf(&b, "*%d\r\n", len(args))
	for _, a := range args {
		fmt.Fprintf(&b, "$%d\r\n%s\r\n", len(a), a)
	}
	return b.Bytes()
}

// readReplies reads exactly `want` lines terminated by \r\n and returns them.
// Good enough to validate "+PONG\r\n"-style simple-string replies.
func readReplies(r *bufio.Reader, want int) ([]string, error) {
	out := make([]string, 0, want)
	for i := 0; i < want; i++ {
		line, err := r.ReadString('\n')
		if err != nil {
			return out, err
		}
		out = append(out, line)
	}
	return out, nil
}

func dial() (net.Conn, error) { return net.Dial("tcp", addr()) }

func arg(i int, def int) int {
	if len(os.Args) > i {
		if v, err := strconv.Atoi(os.Args[i]); err == nil {
			return v
		}
	}
	return def
}

// ---- scenario: many concurrent connections, each sending N PINGs ----
func concurrent() {
	clients := arg(2, 500)
	each := arg(3, 100)
	fmt.Printf("[concurrent] %d clients × %d PINGs = %d total\n", clients, each, clients*each)

	var wg sync.WaitGroup
	var okReplies, errs int64
	start := time.Now()

	for c := 0; c < clients; c++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			conn, err := dial()
			if err != nil {
				atomic.AddInt64(&errs, 1)
				return
			}
			defer conn.Close()
			r := bufio.NewReader(conn)
			for i := 0; i < each; i++ {
				if _, err := conn.Write(respCmd("PING")); err != nil {
					atomic.AddInt64(&errs, 1)
					return
				}
				line, err := r.ReadString('\n')
				if err != nil {
					atomic.AddInt64(&errs, 1)
					return
				}
				if line == "+PONG\r\n" {
					atomic.AddInt64(&okReplies, 1)
				} else {
					atomic.AddInt64(&errs, 1)
				}
			}
		}()
	}
	wg.Wait()
	dur := time.Since(start)
	total := int64(clients * each)
	fmt.Printf("  ok=%d  errors=%d  expected=%d\n", okReplies, errs, total)
	fmt.Printf("  %.0f req/s in %v\n", float64(total)/dur.Seconds(), dur.Round(time.Millisecond))
	pass(okReplies == total && errs == 0)
}

// ---- scenario: one large payload, watch the drain loop ----
// NOTE: sends a single huge bulk string. A *correct* RESP server replies ONCE.
// The current server replies per recv-chunk, so this reveals the framing bug.
func large() {
	mb := arg(2, 2)
	payload := bytes.Repeat([]byte("A"), mb*1024*1024)
	fmt.Printf("[large] sending one %d MB bulk string\n", mb)

	conn, err := dial()
	must(err)
	defer conn.Close()

	start := time.Now()
	if _, err := conn.Write(respCmd("ECHO", string(payload))); err != nil {
		fmt.Println("  write error:", err)
		return
	}
	// Read whatever comes back for 1s and count replies.
	conn.SetReadDeadline(time.Now().Add(1 * time.Second))
	buf := make([]byte, 64*1024)
	var got int
	var replies int
	for {
		n, err := conn.Read(buf)
		got += n
		replies += bytes.Count(buf[:n], []byte("\r\n"))
		if err != nil {
			break
		}
	}
	fmt.Printf("  sent=%d bytes in %v\n", mb*1024*1024, time.Since(start).Round(time.Millisecond))
	fmt.Printf("  received=%d bytes, ~%d reply-lines\n", got, replies)
	fmt.Printf(
		"  (a correct RESP server replies exactly 1 line here; >1 = per-chunk framing bug)\n",
	)
}

// ---- scenario: dribble one command in one byte at a time ----
func partial() {
	delayMs := arg(2, 50)
	cmd := respCmd("PING")
	fmt.Printf("[partial] sending %q one byte every %dms\n", cmd, delayMs)

	conn, err := dial()
	must(err)
	defer conn.Close()
	r := bufio.NewReader(conn)

	for _, b := range cmd {
		if _, err := conn.Write([]byte{b}); err != nil {
			fmt.Println("  write error:", err)
			return
		}
		time.Sleep(time.Duration(delayMs) * time.Millisecond)
	}
	conn.SetReadDeadline(time.Now().Add(1 * time.Second))
	line, err := r.ReadString('\n')
	if err != nil {
		fmt.Printf("  no clean reply: %v (got %q)\n", err, line)
		fmt.Printf("  (a server that accumulates correctly still returns exactly one +PONG)\n")
		return
	}
	fmt.Printf("  reply: %q\n", line)
	pass(line == "+PONG\r\n")
}

// ---- scenario: pipeline many commands in one write ----
func pipeline() {
	n := arg(2, 1000)
	var b bytes.Buffer
	for i := 0; i < n; i++ {
		b.Write(respCmd("PING"))
	}
	fmt.Printf("[pipeline] %d PINGs in a single write\n", n)

	conn, err := dial()
	must(err)
	defer conn.Close()
	if _, err := conn.Write(b.Bytes()); err != nil {
		fmt.Println("  write error:", err)
		return
	}
	r := bufio.NewReader(conn)
	conn.SetReadDeadline(time.Now().Add(2 * time.Second))
	replies, err := readReplies(r, n)
	good := 0
	for _, l := range replies {
		if l == "+PONG\r\n" {
			good++
		}
	}
	fmt.Printf("  expected=%d  got=%d  correct=%d  err=%v\n", n, len(replies), good, err)
	fmt.Printf("  (correct RESP framing => got==expected: one reply per command)\n")
	pass(good == n)
}

func must(err error) {
	if err != nil {
		fmt.Println("fatal:", err)
		os.Exit(1)
	}
}

func pass(ok bool) {
	if ok {
		fmt.Println("  PASS ✓")
	} else {
		fmt.Println("  FAIL ✗")
	}
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("usage: go run ./stress <concurrent|large|partial|pipeline|all> [args...]")
		os.Exit(2)
	}
	switch os.Args[1] {
	case "concurrent":
		concurrent()
	case "large":
		large()
	case "partial":
		partial()
	case "pipeline":
		pipeline()
	case "all":
		concurrent()
		fmt.Println()
		large()
		fmt.Println()
		partial()
		fmt.Println()
		pipeline()
	default:
		fmt.Println("unknown scenario:", os.Args[1])
		os.Exit(2)
	}
}
